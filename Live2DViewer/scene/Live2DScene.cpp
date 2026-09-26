#include "Live2DScene.hpp"

#include <QDateTime>
#include <QMouseEvent>

static bool gladLoaded = false;
static int fps = 40;


Live2DScene::Live2DScene(QWidget* parent)
    : QOpenGLWidget(parent)
    , lastUpdateTime(-1)
    , paramValues()
    , autoBlink(true)
    , autoBreath(true)
    , autoPhysics(true)
    , vbo(-1)
    , selectedDrawableIndex(-1)
    , program(nullptr)
    , modelScale(1.0f)
    , modelOffsetX(0.0f)
    , modelOffsetY(0.0f)
{
    menu = new QMenu(this);
    menu->addAction("清除选中", [this]() {
        selectedDrawableIndex = -1;
        emit clearSelection();
    });
    setFocusPolicy(Qt::ClickFocus);
}

Live2DScene::~Live2DScene()
{
    if (holder.version == Version::V2) {
        delete holder.model2;
    } else if (holder.version == Version::V3) {
        delete holder.model3;
    }
    delete program;
}

void Live2DScene::LoadModel(const QString& filePath)
{
    if (filePath.endsWith("model3.json")) {
        holder.version = Version::V3;
        holder.model3 = new V3::Model();
        holder.model3->LoadModelJson(filePath.toStdString().c_str());
    } else if (filePath.endsWith("model.json")) {
        holder.version = Version::V2;
        holder.model2 = new V2::Model();
        holder.model2->loadModelJson(filePath.toStdString().c_str(), false);
    }
}

ModelHolder& Live2DScene::GetModel()
{
    return holder;
}

QVector<ParamValue>* Live2DScene::GetParamValues()
{
    return &paramValues;
}

void Live2DScene::selectDrawable(int index)
{
    selectedDrawableIndex = index;
}

void Live2DScene::setAutoBlink(bool value)
{
    autoBlink = value;
}

void Live2DScene::setAutoBreath(bool value)
{
    autoBreath = value;
}

void Live2DScene::setAutoPhysics(bool value)
{
    autoPhysics = value;
}

void Live2DScene::timerEvent(QTimerEvent* event)
{
    update();
}

void Live2DScene::initializeGL()
{
    initializeOpenGLFunctions();

    if (!gladLoaded) {
        gladLoadGL();
        gladLoaded = true;
    }

    if (holder.version == Version::V3) {
        holder.model3->CreateRenderer(2);
    } else if (holder.version == Version::V2) {
        holder.model2->CreateRenderer();
    }

    lastUpdateTime = QDateTime::currentMSecsSinceEpoch();

    program = new QOpenGLShaderProgram(this);
    program->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
        #version 330 core
        layout (location = 0) in vec2 position;
        uniform mat4 mvp;
        void main()
        {
            gl_Position = mvp * vec4(position.x, position.y, 0.0, 1.0);
        })");
    program->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
        #version 330 core
        out vec4 color;
        void main()
        {
            color = vec4(0.8, 0.2, 0.2, 0.5);
        })");
    program->link();

    glGenBuffers(1, &vbo);

    startTimer(1000 / fps);
}

void Live2DScene::paintGL()
{
    if (lastUpdateTime < 0) {
        return;
    }

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    long long currentTime = QDateTime::currentMSecsSinceEpoch();
    float deltaTime =
        float(((double)QDateTime::currentMSecsSinceEpoch() - (double)lastUpdateTime) / 1000.0);
    lastUpdateTime = currentTime;

    if (holder.version == Version::V3) {
        bool motionUpdated = false;
        auto model = holder.model3;
        model->LoadParameters();
        if (!model->IsMotionFinished()) {
            motionUpdated = model->UpdateMotion(deltaTime);
        }
        for (auto& param : paramValues) {
            model->SetParameterValue(param.index, param.value);
        }

        model->SaveParameters();

        if (!motionUpdated && autoBlink) {
            model->UpdateBlink(deltaTime);
        }
        model->UpdateExpression(deltaTime);

        model->UpdateDrag(deltaTime);

        if (autoBreath) {
            model->UpdateBreath(deltaTime);
        }

        paramValues.clear();

        if (autoPhysics) {
            model->UpdatePhysics(deltaTime);
        }

        model->UpdatePose(deltaTime);
        model->Draw();

        if (selectedDrawableIndex != -1) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            program->bind();
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER,
                         sizeof(float) * 2 * model->GetDrawableVertexCount(selectedDrawableIndex),
                         model->GetDrawableVertices(selectedDrawableIndex),
                         GL_STATIC_DRAW);
            QMatrix4x4 mvp(model->GetMvp());
            program->setUniformValue("mvp", mvp);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);
            glDrawElements(GL_TRIANGLES,
                           model->GetDrawableVertexIndexCount(selectedDrawableIndex),
                           GL_UNSIGNED_SHORT,
                           model->GetDrawableIndices(selectedDrawableIndex));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            program->release();
        }
    } else if (holder.version == Version::V2) {
        auto model = holder.model2;
        model->update();
        model->draw();
    }

    emit paramValuesUpdated();
}

void Live2DScene::resizeGL(int w, int h)
{
    if (holder.version == Version::V3) {
        holder.model3->Resize(w, h);
    } else if (holder.version == Version::V2) {
        holder.model2->resize(w, h);
    }
}

void Live2DScene::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        if (holder.version == Version::V3) {
            holder.model3->Drag((float)event->x(), (float)event->y());
        } else if (holder.version == Version::V2) {
            holder.model2->drag((float)event->x(), (float)event->y());
        }
    }
}

void Live2DScene::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        menu->exec(event->globalPos());
    }
}

void Live2DScene::mouseReleaseEvent(QMouseEvent* event)
{
    // 复位
    if (event->button() == Qt::LeftButton) {
        if (holder.version == Version::V3) {
            holder.model3->Drag(width() / 2, height() / 2);
        } else if (holder.version == Version::V2) {
            holder.model2->drag(width() / 2, height() / 2);
        }
    }
}

void Live2DScene::keyPressEvent(QKeyEvent* event)
{
    const float step = 0.1f;
    auto setOffset = [this](float a, float b) {
        if (holder.version == Version::V2) {
            holder.model2->setOffset(a, b);
        } else if (holder.version == Version::V3) {
            holder.model3->SetOffset(a, b);
        }
    };
    auto setScale = [this](float s) {
        if (holder.version == Version::V2) {
            holder.model2->setScale(s);
        } else if (holder.version == Version::V3) {
            holder.model3->SetScale(s);
        }
    };
    switch (event->key()) {
        case Qt::Key_Equal:
            modelScale += step;
            setScale(modelScale);
            break;
        case Qt::Key_Minus:
            modelScale -= step;
            setScale(modelScale);
            break;
        case Qt::Key_Up:
            modelOffsetY += step;
            setScale(modelScale);
            break;
        case Qt::Key_Down:
            modelOffsetY -= step;
            setOffset(modelOffsetX, modelOffsetY);
            break;
        case Qt::Key_Left:
            modelOffsetX -= step;
            setOffset(modelOffsetX, modelOffsetY);
            break;
        case Qt::Key_Right:
            modelOffsetX += step;
            setOffset(modelOffsetX, modelOffsetY);
            break;
    }
}

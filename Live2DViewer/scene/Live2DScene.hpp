#pragma once
// make sure OpenGL header is included first by glew 
#include <GL/glew.h>
#include <IModel.hpp>

#include <QMenu>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <optional>


using namespace Live2D;


struct ParamValue {
    int index;
    float value;
};

// 统一模型接口: V2/V3 模型都通过 IModel 访问（版本用 IModel::IsV2()/IsV3()）
struct ModelHolder {
    IModel* model = nullptr;
};


class Live2DScene : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

signals:
    void paramValuesUpdated();
    void clearSelection();

public slots:
    void setAutoBlink(bool value);
    void setAutoBreath(bool value);
    void setAutoPhysics(bool value);

protected:
    void timerEvent(QTimerEvent* event) override;
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

public:
    Live2DScene(QWidget* parent = nullptr);
    ~Live2DScene();

    void LoadModel(const QString& filePath);

    ModelHolder& GetModel();

    QVector<ParamValue>* GetParamValues();

    void selectDrawable(int index);

private:
    ModelHolder holder;

    long long lastUpdateTime;

    QVector<ParamValue> paramValues;

    bool autoBlink;
    bool autoBreath;
    bool autoPhysics;

    QOpenGLShaderProgram* program;
    GLuint vbo;
    int selectedDrawableIndex;

    float modelScale;
    float modelOffsetX;
    float modelOffsetY;

    QMenu* menu;
};

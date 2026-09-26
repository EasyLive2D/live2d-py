#pragma once
#include <V2/Model.hpp>
#include <V3/Model.hpp>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QMenu>
#include <optional>


using namespace Live2D;


struct ParamValue
{
    int index;
    float value;
};

enum Version {
    V2 = 2,
    V3 = 3
};

union ModelHolder {
    Version version;
    V2::Model* model2;
    V3::Model* model3;
};


class Live2DScene : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

signals:
    void paramValuesUpdated();
    void clearSelection();

public slots:
    void setAutoBlink(bool value);
    void setAutoBreath(bool value);
    void setAutoPhysics(bool value);

protected:
    void timerEvent(QTimerEvent *event) override;
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;
public:
    Live2DScene(QWidget *parent = nullptr);
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

    QOpenGLShaderProgram *program;
    GLuint vbo;
    int selectedDrawableIndex;

    float modelScale;
    float modelOffsetX;
    float modelOffsetY;

    QMenu* menu;
};
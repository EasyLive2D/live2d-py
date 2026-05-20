#pragma once
#include <Model.hpp>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QMenu>


struct ParamValue
{
    int index;
    float value;
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

    Model *GetModel();

    QVector<ParamValue>* GetParamValues();

    void selectDrawable(int index);

private:
    Model *model;

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
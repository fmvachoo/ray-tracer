#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMouseEvent>
#include <QWheelEvent>
#include "Scene.h"
#include "PathTracer.h"

class Viewport : public QOpenGLWidget, protected QOpenGLFunctions_4_3_Core {
    Q_OBJECT
public:
    explicit Viewport(QWidget *parent = nullptr);
    ~Viewport() override;

    void setScene(Scene *scene);
    void renderPathTraced(int spp);
    void setPreviewMode();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

signals:
    void initialized();

private:
    void drawPreview();
    void drawLights();
    void rebuildLightBuffers();

    Scene *m_scene = nullptr;

    // Preview shader + objects
    QOpenGLShaderProgram *m_previewProgram = nullptr;
    PathTracer m_pathTracer;

    // Light shader + buffers
    QOpenGLShaderProgram *m_lightProgram = nullptr;
    QOpenGLVertexArrayObject m_lightVAO;
    QOpenGLBuffer m_lightVBO;
    int m_lightVertexCount = 0;

    bool m_showRender = false;
    bool m_dragging = false;
    bool m_panning = false;
    QPoint m_lastPos;

    bool m_lightBuffersDirty = true;
};

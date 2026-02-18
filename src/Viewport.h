#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLShaderProgram>
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
    QImage grabRenderResult();

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

    Scene *m_scene = nullptr;
    QOpenGLShaderProgram *m_previewProgram = nullptr;
    PathTracer m_pathTracer;

    bool m_showRender = false;
    bool m_dragging = false;
    bool m_panning = false;
    QPoint m_lastPos;
};

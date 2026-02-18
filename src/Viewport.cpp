#include "Viewport.h"
#include <QFile>
#include <QDebug>

Viewport::Viewport(QWidget *parent) : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

Viewport::~Viewport()
{
    makeCurrent();
    m_pathTracer.destroy();
    delete m_previewProgram;
    doneCurrent();
}

void Viewport::setScene(Scene *scene)
{
    m_scene = scene;
    m_showRender = false;

    makeCurrent();
    for (auto &obj : m_scene->objects())
        obj->initGL();
    doneCurrent();

    update();
}

void Viewport::renderPathTraced(int spp)
{
    if (!m_scene) return;

    makeCurrent();
    m_pathTracer.render(*m_scene, spp);
    m_showRender = true;
    doneCurrent();
    update();
}

void Viewport::setPreviewMode()
{
    m_showRender = false;
    update();
}

void Viewport::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // Preview shader
    m_previewProgram = new QOpenGLShaderProgram();
    {
        QFile vf(":/shaders/preview.vert"), ff(":/shaders/preview.frag");
        vf.open(QIODevice::ReadOnly);
        ff.open(QIODevice::ReadOnly);
        m_previewProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vf.readAll());
        m_previewProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, ff.readAll());
        if (!m_previewProgram->link())
            qWarning() << "Preview shader link error:" << m_previewProgram->log();
    }

    m_pathTracer.init(this);

    emit initialized();
}

void Viewport::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    m_pathTracer.resize(w, h);
}

void Viewport::paintGL()
{
    if (m_showRender) {
        glDisable(GL_DEPTH_TEST);
        m_pathTracer.displayResult();
        glEnable(GL_DEPTH_TEST);
    } else {
        drawPreview();
    }
}

void Viewport::drawPreview()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_scene || !m_previewProgram) return;

    m_previewProgram->bind();

    float aspect = float(width()) / float(height());
    QMatrix4x4 view = m_scene->camera().viewMatrix();
    QMatrix4x4 proj = m_scene->camera().projectionMatrix(aspect);

    m_previewProgram->setUniformValue("u_view", view);
    m_previewProgram->setUniformValue("u_projection", proj);
    m_previewProgram->setUniformValue("u_lightDir", QVector3D(0.5f, 1.0f, 0.3f).normalized());
    m_previewProgram->setUniformValue("u_viewPos", m_scene->camera().position());

    for (const auto &obj : m_scene->objects()) {
        QMatrix4x4 model;
        m_previewProgram->setUniformValue("u_model", model);
        m_previewProgram->setUniformValue("u_color", obj->material().color);
        m_previewProgram->setUniformValue("u_roughness", obj->material().roughness);
        obj->draw();
    }

    m_previewProgram->release();
    if (m_scene) {
        //
    }
}

void Viewport::mousePressEvent(QMouseEvent *event)
{
    m_lastPos = event->pos();
    if (event->button() == Qt::LeftButton)
        m_dragging = true;
    else if (event->button() == Qt::MiddleButton)
        m_panning = true;

    if (m_showRender) setPreviewMode();
}

void Viewport::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_scene) return;

    int dx = event->pos().x() - m_lastPos.x();
    int dy = event->pos().y() - m_lastPos.y();
    m_lastPos = event->pos();

    if (m_dragging) {
        m_scene->camera().rotate(dx, -dy);
        update();
    } else if (m_panning) {
        m_scene->camera().pan(-dx, dy);
        update();
    }
}

void Viewport::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_dragging = false;
    m_panning = false;
}

void Viewport::wheelEvent(QWheelEvent *event)
{
    if (!m_scene) return;
    float delta = event->angleDelta().y() / 120.0f;
    m_scene->camera().zoom(delta);
    if (m_showRender) setPreviewMode();
    update();
}

QImage Viewport::grabRenderResult()
{
    makeCurrent();

    glDisable(GL_DEPTH_TEST);
    m_pathTracer.displayResult();
    glEnable(GL_DEPTH_TEST);

    QImage img(width(), height(), QImage::Format_RGB888);
    glReadPixels(0, 0, width(), height(), GL_RGB, GL_UNSIGNED_BYTE, img.bits());

    doneCurrent();

    return img.mirrored(false, true);
}

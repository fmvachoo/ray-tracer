#include "Viewport.h"
#include <QDebug>
#include <cmath>

Viewport::Viewport(QWidget *parent)
    : QOpenGLWidget(parent), m_lightVBO(QOpenGLBuffer::VertexBuffer)
{
}

Viewport::~Viewport()
{
    makeCurrent();
    m_lightVAO.destroy();
    m_lightVBO.destroy();
    delete m_previewProgram;
    delete m_lightProgram;
    doneCurrent();
}

void Viewport::setScene(Scene *scene)
{
    m_scene = scene;
    m_showRender = false;
    m_lightBuffersDirty = true;
    update();
}

void Viewport::setPreviewMode()
{
    m_showRender = false;
    m_lightBuffersDirty = true;
    update();
}

void Viewport::renderPathTraced(int spp)
{
    if (!m_scene) return;
    makeCurrent();
    m_pathTracer.render(*m_scene, width(), height(), spp);
    m_showRender = true;
    doneCurrent();
    update();
}

void Viewport::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Preview shader (for scene objects)
    m_previewProgram = new QOpenGLShaderProgram;
    m_previewProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/preview.vert");
    m_previewProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/preview.frag");
    m_previewProgram->link();

    // Light shader
    m_lightProgram = new QOpenGLShaderProgram;
    m_lightProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/light.vert");
    m_lightProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/light.frag");
    m_lightProgram->link();

    // Light VAO/VBO
    m_lightVAO.create();
    m_lightVBO.create();

    m_pathTracer.init(this);

    emit initialized();
}

void Viewport::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void Viewport::paintGL()
{
    if (m_showRender) {
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        m_pathTracer.displayResult();
        glEnable(GL_DEPTH_TEST);
        return;
    }

    drawPreview();
}

void Viewport::drawPreview()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_scene || !m_previewProgram) return;

    float aspect = float(width()) / float(height());
    QMatrix4x4 view = m_scene->camera().viewMatrix();
    QMatrix4x4 proj = m_scene->camera().projectionMatrix(aspect);

    // Draw scene objects
    m_previewProgram->bind();
    m_previewProgram->setUniformValue("uView", view);
    m_previewProgram->setUniformValue("uProjection", proj);

    QMatrix4x4 model;
    m_previewProgram->setUniformValue("uModel", model);

    QVector3D camPos = m_scene->camera().position();
    m_previewProgram->setUniformValue("uViewPos", camPos);

    // Simple light for preview — use first scene light if available
    if (!m_scene->lights().isEmpty()) {
        m_previewProgram->setUniformValue("uLightPos", m_scene->lights()[0].position);
        QVector3D lc = m_scene->lights()[0].color * std::min(m_scene->lights()[0].intensity / 5.0f, 1.0f);
        m_previewProgram->setUniformValue("uLightColor", lc);
    } else {
        m_previewProgram->setUniformValue("uLightPos", QVector3D(0, 3, 0));
        m_previewProgram->setUniformValue("uLightColor", QVector3D(1, 1, 1));
    }

    for (const auto &obj : m_scene->objects()) {
        if (!obj->isLoaded()) continue;

        if (!obj->isGLInitialized()) {
            obj->initGL();
        }

        m_previewProgram->setUniformValue("uColor", obj->material().color);
        obj->draw();
    }

    m_previewProgram->release();

    // Draw lights
    drawLights();
}

void Viewport::rebuildLightBuffers()
{
    if (!m_scene) return;

    QVector<float> vertices;

    for (const auto &light : m_scene->lights()) {
        QVector3D v0, v1, v2, v3;
        light.getCorners(v0, v1, v2, v3);

        // Two triangles: v0-v1-v2 and v0-v2-v3
        auto addVert = [&](const QVector3D &v) {
            vertices.append(v.x());
            vertices.append(v.y());
            vertices.append(v.z());
        };

        addVert(v0); addVert(v1); addVert(v2);
        addVert(v0); addVert(v2); addVert(v3);

        // Back face (so visible from both sides)
        addVert(v0); addVert(v2); addVert(v1);
        addVert(v0); addVert(v3); addVert(v2);
    }

    m_lightVertexCount = vertices.size() / 3;

    m_lightVAO.bind();
    m_lightVBO.bind();
    m_lightVBO.allocate(vertices.constData(), vertices.size() * sizeof(float));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    m_lightVBO.release();
    m_lightVAO.release();

    m_lightBuffersDirty = false;
}

void Viewport::drawLights()
{
    if (!m_scene || m_scene->lights().isEmpty() || !m_lightProgram) return;

    if (m_lightBuffersDirty) {
        rebuildLightBuffers();
    }

    if (m_lightVertexCount == 0) return;

    float aspect = float(width()) / float(height());
    QMatrix4x4 view = m_scene->camera().viewMatrix();
    QMatrix4x4 proj = m_scene->camera().projectionMatrix(aspect);
    QMatrix4x4 mvp = proj * view;

    m_lightProgram->bind();
    m_lightProgram->setUniformValue("uMVP", mvp);

    m_lightVAO.bind();

    int offset = 0;
    for (const auto &light : m_scene->lights()) {
        m_lightProgram->setUniformValue("uColor", light.color);
        m_lightProgram->setUniformValue("uIntensity", light.intensity);

        glDrawArrays(GL_TRIANGLES, offset, 12);
        offset += 12;
    }

    m_lightVAO.release();
    m_lightProgram->release();
}

// ======================== Mouse Controls ========================

void Viewport::mousePressEvent(QMouseEvent *event)
{
    m_lastPos = event->pos();

    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
    } else if (event->button() == Qt::MiddleButton) {
        m_panning = true;
    }
}

void Viewport::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_scene) return;

    QPoint delta = event->pos() - m_lastPos;
    m_lastPos = event->pos();

    if (m_dragging) {
        m_scene->camera().orbit(delta.x() * 0.5f, delta.y() * 0.5f);
        m_lightBuffersDirty = true;
        update();
    } else if (m_panning) {
        m_scene->camera().pan(delta.x() * 0.01f, delta.y() * 0.01f);
        m_lightBuffersDirty = true;
        update();
    }
}

void Viewport::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) m_dragging = false;
    if (event->button() == Qt::MiddleButton) m_panning = false;
}

void Viewport::wheelEvent(QWheelEvent *event)
{
    if (!m_scene) return;
    float delta = event->angleDelta().y() / 120.0f;
    m_scene->camera().zoom(delta);
    update();
}

#include "SceneObject.h"
#include <QOpenGLFunctions>
#include <QOpenGLContext>

SceneObject::SceneObject(const QString &name, const QString &objPath)
    : m_name(name), m_objPath(objPath)
{
}

bool SceneObject::loadMesh()
{
    return ObjLoader::load(m_objPath, m_mesh);
}

void SceneObject::initGL()
{
    if (m_mesh.vertices.isEmpty()) return;

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    m_vao.create();
    m_vao.bind();

    // interleave: pos(3) + normal(3) per vertex
    QVector<float> data;
    data.reserve(m_mesh.vertices.size() * 6);
    for (int i = 0; i < m_mesh.vertices.size(); ++i) {
        data.append(m_mesh.vertices[i].x());
        data.append(m_mesh.vertices[i].y());
        data.append(m_mesh.vertices[i].z());
        data.append(m_mesh.normals[i].x());
        data.append(m_mesh.normals[i].y());
        data.append(m_mesh.normals[i].z());
    }

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(data.constData(), data.size() * sizeof(float));

    // position
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    // normal
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                             reinterpret_cast<void *>(3 * sizeof(float)));

    m_ebo.create();
    m_ebo.bind();
    m_ebo.allocate(m_mesh.indices.constData(), m_mesh.indices.size() * sizeof(unsigned int));
    m_indexCount = m_mesh.indices.size();

    m_vao.release();
    m_glInitialized = true;
}

void SceneObject::draw()
{
    if (!m_glInitialized) return;

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    m_vao.bind();
    f->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    m_vao.release();
}

void SceneObject::destroyGL()
{
    if (!m_glInitialized) return;
    m_ebo.destroy();
    m_vbo.destroy();
    m_vao.destroy();
    m_glInitialized = false;
}

QJsonObject SceneObject::toJson() const
{
    QJsonObject obj;
    obj["name"] = m_name;
    obj["objPath"] = m_objPath;
    obj["material"] = m_material.toJson();
    return obj;
}

void SceneObject::fromJson(const QJsonObject &obj)
{
    m_name = obj["name"].toString();
    m_objPath = obj["objPath"].toString();
    m_material.fromJson(obj["material"].toObject());
}
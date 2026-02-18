#pragma once

#include <QString>
#include <QJsonObject>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include "Material.h"
#include "ObjLoader.h"

class SceneObject {
public:
    SceneObject() = default;
    SceneObject(const QString &name, const QString &objPath);

    QString name() const { return m_name; }
    QString objPath() const { return m_objPath; }

    Material &material() { return m_material; }
    const Material &material() const { return m_material; }
    const Mesh &mesh() const { return m_mesh; }

    bool loadMesh();
    void initGL();
    void draw();
    void destroyGL();

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &obj);

private:
    QString m_name;
    QString m_objPath;
    Material m_material;
    Mesh m_mesh;

    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer m_ebo{QOpenGLBuffer::IndexBuffer};
    int m_indexCount = 0;
    bool m_glInitialized = false;
};
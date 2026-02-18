#pragma once

#include <QString>
#include <QVector>
#include <QVector3D>

struct Mesh {
    QVector<QVector3D> vertices;
    QVector<QVector3D> normals;
    QVector<unsigned int> indices;
};

class ObjLoader {
public:
    static bool load(const QString &path, Mesh &mesh);
};
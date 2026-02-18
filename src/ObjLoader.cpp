#include "ObjLoader.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

bool ObjLoader::load(const QString &path, Mesh &mesh)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open OBJ file:" << path;
        return false;
    }

    QVector<QVector3D> positions;
    QVector<QVector3D> normals;

    struct VertexKey {
        int posIdx;
        int normIdx;
    };

    QVector<VertexKey> faceVertices;
    QVector<unsigned int> faceIndices;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith('#') || line.isEmpty())
            continue;

        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.isEmpty()) continue;

        if (parts[0] == "v" && parts.size() >= 4) {
            positions.append(QVector3D(
                parts[1].toFloat(),
                parts[2].toFloat(),
                parts[3].toFloat()));
        } else if (parts[0] == "vn" && parts.size() >= 4) {
            normals.append(QVector3D(
                parts[1].toFloat(),
                parts[2].toFloat(),
                parts[3].toFloat()));
        } else if (parts[0] == "f") {
            QVector<int> polyIndices;
            for (int i = 1; i < parts.size(); ++i) {
                QStringList sub = parts[i].split('/');
                int pi = sub[0].toInt() - 1;
                int ni = -1;
                if (sub.size() >= 3 && !sub[2].isEmpty())
                    ni = sub[2].toInt() - 1;

                // find or create vertex
                int idx = faceVertices.size();
                faceVertices.append({pi, ni});
                polyIndices.append(idx);
            }
            // triangulate polygon (fan)
            for (int i = 1; i + 1 < polyIndices.size(); ++i) {
                faceIndices.append(polyIndices[0]);
                faceIndices.append(polyIndices[i]);
                faceIndices.append(polyIndices[i + 1]);
            }
        }
    }

    mesh.vertices.resize(faceVertices.size());
    mesh.normals.resize(faceVertices.size());

    for (int i = 0; i < faceVertices.size(); ++i) {
        const auto &v = faceVertices[i];
        mesh.vertices[i] = (v.posIdx >= 0 && v.posIdx < positions.size())
                               ? positions[v.posIdx]
                               : QVector3D();
        if (v.normIdx >= 0 && v.normIdx < normals.size()) {
            mesh.normals[i] = normals[v.normIdx];
        } else {
            mesh.normals[i] = QVector3D(0, 1, 0);
        }
    }

    mesh.indices.resize(faceIndices.size());
    for (int i = 0; i < faceIndices.size(); ++i)
        mesh.indices[i] = faceIndices[i];

    // compute normals if missing
    if (normals.isEmpty()) {
        for (int i = 0; i + 2 < mesh.indices.size(); i += 3) {
            QVector3D &v0 = mesh.vertices[mesh.indices[i]];
            QVector3D &v1 = mesh.vertices[mesh.indices[i + 1]];
            QVector3D &v2 = mesh.vertices[mesh.indices[i + 2]];
            QVector3D n = QVector3D::crossProduct(v1 - v0, v2 - v0).normalized();
            mesh.normals[mesh.indices[i]] = n;
            mesh.normals[mesh.indices[i + 1]] = n;
            mesh.normals[mesh.indices[i + 2]] = n;
        }
    }

    qDebug() << "Loaded OBJ:" << path
             << "verts:" << mesh.vertices.size()
             << "tris:" << mesh.indices.size() / 3;
    return true;
}
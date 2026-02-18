#pragma once

#include <QVector3D>
#include <QString>

struct Light {
    QString name;
    QVector3D position{0.0f, 2.5f, 0.0f};
    QVector3D color{1.0f, 1.0f, 1.0f};
    float intensity = 5.0f;
    float width = 1.0f;
    float height = 1.0f;
    QVector3D rotation{0.0f, 0.0f, 0.0f}; // euler degrees (pitch, yaw, roll)

    // Generate 4 corners of the light plane in world space
    void getCorners(QVector3D &v0, QVector3D &v1, QVector3D &v2, QVector3D &v3) const {
        float hw = width * 0.5f;
        float hh = height * 0.5f;

        // Local corners (facing down by default, -Y normal)
        QVector3D c0(-hw, 0, -hh);
        QVector3D c1( hw, 0, -hh);
        QVector3D c2( hw, 0,  hh);
        QVector3D c3(-hw, 0,  hh);

        // Apply rotation
        QMatrix4x4 rot;
        rot.rotate(rotation.x(), 1, 0, 0);
        rot.rotate(rotation.y(), 0, 1, 0);
        rot.rotate(rotation.z(), 0, 0, 1);

        v0 = position + rot.map(c0);
        v1 = position + rot.map(c1);
        v2 = position + rot.map(c2);
        v3 = position + rot.map(c3);
    }

    QVector3D normal() const {
        QMatrix4x4 rot;
        rot.rotate(rotation.x(), 1, 0, 0);
        rot.rotate(rotation.y(), 0, 1, 0);
        rot.rotate(rotation.z(), 0, 0, 1);
        return rot.map(QVector3D(0, -1, 0)).normalized();
    }
};
#pragma once

#include <QVector3D>
#include <QJsonObject>

struct Material {
    QVector3D color{0.8f, 0.8f, 0.8f};
    float roughness = 0.5f;
    float transparency = 0.0f;
    float ior = 1.5f;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &obj);
};

#include "Material.h"

QJsonObject Material::toJson() const
{
    QJsonObject obj;
    obj["r"] = color.x();
    obj["g"] = color.y();
    obj["b"] = color.z();
    obj["roughness"] = roughness;
    obj["transparency"] = transparency;
    return obj;
}

void Material::fromJson(const QJsonObject &obj)
{
    color.setX(obj["r"].toDouble(0.8));
    color.setY(obj["g"].toDouble(0.8));
    color.setZ(obj["b"].toDouble(0.8));
    roughness = obj["roughness"].toDouble(0.5);
    transparency = obj["transparency"].toDouble(0.0);
}
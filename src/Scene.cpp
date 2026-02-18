#include "Scene.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QCoreApplication>

void Scene::clear()
{
    m_objects.clear();
    m_lights.clear();
}

void Scene::addLight(const Light &light)
{
    m_lights.append(light);
}

void Scene::removeLight(int index)
{
    if (index >= 0 && index < m_lights.size())
        m_lights.removeAt(index);
}

void Scene::createDefault()
{
    clear();
    m_camera.reset();

    QString basePath = QCoreApplication::applicationDirPath();
    qDebug() << "App dir:" << basePath;

    struct ObjDef {
        QString name;
        QString path;
        QVector3D color;
        float roughness;
    };

    QVector<ObjDef> defs = {
                            {"cornell_box", basePath + "/models/cornell_box.obj", {0.8f, 0.8f, 0.8f}, 0.9f},
                            {"obj1",        basePath + "/models/obj1.obj",        {0.9f, 0.2f, 0.2f}, 0.3f},
                            {"obj2",        basePath + "/models/obj2.obj",        {0.2f, 0.9f, 0.2f}, 0.5f},
                            {"obj3",        basePath + "/models/obj3.obj",        {0.2f, 0.2f, 0.9f}, 0.1f},
                            };

    for (const auto &d : defs) {
        auto obj = std::make_shared<SceneObject>(d.name, d.path);
        obj->material().color = d.color;
        obj->material().roughness = d.roughness;
        obj->material().transparency = 0.0f;

        if (!obj->loadMesh()) {
            qWarning() << "FAILED to load:" << d.path;
        } else {
            qDebug() << "OK:" << d.path << "tris:" << obj->mesh().indices.size() / 3;
        }
        m_objects.append(obj);
    }

    // Default light
    Light defaultLight;
    defaultLight.name = "Light 1";
    defaultLight.position = QVector3D(0.0f, 2.9f, 0.0f);
    defaultLight.color = QVector3D(1.0f, 0.95f, 0.9f);
    defaultLight.intensity = 8.0f;
    defaultLight.width = 1.5f;
    defaultLight.height = 1.5f;
    defaultLight.rotation = QVector3D(0, 0, 0);
    m_lights.append(defaultLight);
}

bool Scene::load(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return false;

    QJsonObject root = doc.object();
    clear();

    QString basePath = QCoreApplication::applicationDirPath();

    QJsonArray objArr = root["objects"].toArray();
    for (const auto &val : objArr) {
        QJsonObject jo = val.toObject();
        QString name = jo["name"].toString();
        QString objPath = jo["path"].toString();

        if (!QFile::exists(objPath))
            objPath = basePath + "/" + objPath;

        auto obj = std::make_shared<SceneObject>(name, objPath);

        QJsonObject matObj = jo["material"].toObject();
        obj->material().color = QVector3D(
            matObj["r"].toDouble(0.8),
            matObj["g"].toDouble(0.8),
            matObj["b"].toDouble(0.8));
        obj->material().roughness = matObj["roughness"].toDouble(0.5);
        obj->material().transparency = matObj["transparency"].toDouble(0.0);
        obj->material().ior = matObj["ior"].toDouble(1.5);

        obj->loadMesh();
        m_objects.append(obj);
    }

    QJsonArray lightArr = root["lights"].toArray();
    for (const auto &val : lightArr) {
        QJsonObject lo = val.toObject();
        Light light;
        light.name = lo["name"].toString("Light");
        light.position = QVector3D(lo["px"].toDouble(), lo["py"].toDouble(), lo["pz"].toDouble());
        light.color = QVector3D(lo["cr"].toDouble(1), lo["cg"].toDouble(1), lo["cb"].toDouble(1));
        light.intensity = lo["intensity"].toDouble(5);
        light.width = lo["width"].toDouble(1);
        light.height = lo["height"].toDouble(1);
        light.rotation = QVector3D(lo["rx"].toDouble(), lo["ry"].toDouble(), lo["rz"].toDouble());
        m_lights.append(light);
    }

    return true;
}

bool Scene::save(const QString &path) const
{
    QJsonArray objArr;
    for (const auto &obj : m_objects) {
        QJsonObject jo;
        jo["name"] = obj->name();
        jo["path"] = obj->path();

        QJsonObject matObj;
        matObj["r"] = obj->material().color.x();
        matObj["g"] = obj->material().color.y();
        matObj["b"] = obj->material().color.z();
        matObj["roughness"] = obj->material().roughness;
        matObj["transparency"] = obj->material().transparency;
        matObj["ior"] = obj->material().ior;
        jo["material"] = matObj;

        objArr.append(jo);
    }

    QJsonArray lightArr;
    for (const auto &light : m_lights) {
        QJsonObject lo;
        lo["name"] = light.name;
        lo["px"] = light.position.x();
        lo["py"] = light.position.y();
        lo["pz"] = light.position.z();
        lo["cr"] = light.color.x();
        lo["cg"] = light.color.y();
        lo["cb"] = light.color.z();
        lo["intensity"] = light.intensity;
        lo["width"] = light.width;
        lo["height"] = light.height;
        lo["rx"] = light.rotation.x();
        lo["ry"] = light.rotation.y();
        lo["rz"] = light.rotation.z();
        lightArr.append(lo);
    }

    QJsonObject root;
    root["objects"] = objArr;
    root["lights"] = lightArr;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson());
    return true;
}

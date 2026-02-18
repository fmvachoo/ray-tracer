#pragma once

#include <QVector>
#include <QString>
#include <memory>
#include "SceneObject.h"
#include "Camera.h"
#include "Light.h"

class Scene {
public:
    void createDefault();
    void clear();

    bool load(const QString &path);
    bool save(const QString &path) const;

    QVector<std::shared_ptr<SceneObject>> &objects() { return m_objects; }
    const QVector<std::shared_ptr<SceneObject>> &objects() const { return m_objects; }

    QVector<Light> &lights() { return m_lights; }
    const QVector<Light> &lights() const { return m_lights; }

    void addLight(const Light &light);
    void removeLight(int index);

    Camera &camera() { return m_camera; }
    const Camera &camera() const { return m_camera; }

private:
    QVector<std::shared_ptr<SceneObject>> m_objects;
    QVector<Light> m_lights;
    Camera m_camera;
};

#pragma once

#include <QVector3D>
#include <QMatrix4x4>
#include <QJsonObject>

class Camera {
public:
    Camera();

    void reset();
    void rotate(float dx, float dy);
    void pan(float dx, float dy);
    void zoom(float delta);

    QMatrix4x4 viewMatrix() const;
    QMatrix4x4 projectionMatrix(float aspect) const;

    QVector3D position() const;
    QVector3D target() const { return m_target; }
    QVector3D front() const;
    QVector3D up() const;
    QVector3D right() const;

    float fov() const { return m_fov; }

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &obj);

private:
    void updateVectors();

    float m_yaw = -90.0f;
    float m_pitch = 0.0f;
    float m_distance = 5.0f;
    QVector3D m_target{0.0f, 1.0f, 0.0f};
    QVector3D m_front;
    QVector3D m_up;
    QVector3D m_right;
    float m_fov = 45.0f;
};

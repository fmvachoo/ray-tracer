#include "Camera.h"
#include <QtMath>

Camera::Camera()
{
    updateVectors();
}

void Camera::reset()
{
    m_yaw = -90.0f;
    m_pitch = 20.0f;
    m_distance = 5.0f;
    m_target = QVector3D(0.0f, 1.0f, 0.0f);
    m_fov = 45.0f;
    updateVectors();
}

void Camera::rotate(float dx, float dy)
{
    m_yaw += dx * 0.3f;
    m_pitch += dy * 0.3f;
    m_pitch = qBound(-89.0f, m_pitch, 89.0f);
    updateVectors();
}

void Camera::pan(float dx, float dy)
{
    m_target += m_right * dx * 0.005f * m_distance;
    m_target += m_up * dy * 0.005f * m_distance;
    updateVectors();
}

void Camera::zoom(float delta)
{
    m_distance -= delta * 0.3f;
    m_distance = qMax(0.5f, m_distance);
    updateVectors();
}

QMatrix4x4 Camera::viewMatrix() const
{
    QMatrix4x4 view;
    view.lookAt(position(), m_target, QVector3D(0, 1, 0));
    return view;
}

QMatrix4x4 Camera::projectionMatrix(float aspect) const
{
    QMatrix4x4 proj;
    proj.perspective(m_fov, aspect, 0.1f, 100.0f);
    return proj;
}

QVector3D Camera::position() const
{
    return m_target - m_front * m_distance;
}

QVector3D Camera::front() const { return m_front; }
QVector3D Camera::up() const { return m_up; }
QVector3D Camera::right() const { return m_right; }

void Camera::updateVectors()
{
    float yawRad = qDegreesToRadians(m_yaw);
    float pitchRad = qDegreesToRadians(m_pitch);

    m_front.setX(cosf(pitchRad) * cosf(yawRad));
    m_front.setY(sinf(pitchRad));
    m_front.setZ(cosf(pitchRad) * sinf(yawRad));
    m_front.normalize();

    m_right = QVector3D::crossProduct(m_front, QVector3D(0, 1, 0)).normalized();
    m_up = QVector3D::crossProduct(m_right, m_front).normalized();
}

QJsonObject Camera::toJson() const
{
    QJsonObject obj;
    obj["yaw"] = m_yaw;
    obj["pitch"] = m_pitch;
    obj["distance"] = m_distance;
    obj["targetX"] = m_target.x();
    obj["targetY"] = m_target.y();
    obj["targetZ"] = m_target.z();
    obj["fov"] = m_fov;
    return obj;
}

void Camera::fromJson(const QJsonObject &obj)
{
    m_yaw = obj["yaw"].toDouble(-90);
    m_pitch = obj["pitch"].toDouble(20);
    m_distance = obj["distance"].toDouble(5);
    m_target.setX(obj["targetX"].toDouble(0));
    m_target.setY(obj["targetY"].toDouble(1));
    m_target.setZ(obj["targetZ"].toDouble(0));
    m_fov = obj["fov"].toDouble(45);
    updateVectors();
}
#include "RenderWindow.h"
#include "BVH.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QDebug>
#include <cmath>
#include <cstdlib>

// ============ RenderWorker ============

RenderWorker::RenderWorker(Scene *scene, int width, int height, int totalSpp)
    : m_scene(scene), m_width(width), m_height(height), m_totalSpp(totalSpp)
{
}

void RenderWorker::process()
{
    QImage image(m_width, m_height, QImage::Format_RGB888);
    image.fill(Qt::black);

    std::vector<float> accum(m_width * m_height * 3, 0.0f);

    Camera &cam = m_scene->camera();
    QVector3D eye = cam.position();
    float fov = cam.fov();
    float aspect = float(m_width) / float(m_height);
    float tanHalf = std::tan(fov * 0.5f * 3.14159265f / 180.0f);

    QVector3D forward = (cam.target() - eye).normalized();
    QVector3D worldUp(0, 1, 0);
    QVector3D right = QVector3D::crossProduct(forward, worldUp).normalized();
    QVector3D up = QVector3D::crossProduct(right, forward).normalized();

    // Collect triangles
    QVector<RenderTriangle> triangles;
    for (const auto &obj : m_scene->objects()) {
        const auto &mesh = obj->mesh();
        const auto &mat = obj->material();
        bool isEmissive = obj->name().contains("light", Qt::CaseInsensitive);

        for (int i = 0; i + 2 < mesh.indices.size(); i += 3) {
            unsigned int idx0 = mesh.indices[i];
            unsigned int idx1 = mesh.indices[i + 1];
            unsigned int idx2 = mesh.indices[i + 2];

            if (idx0 >= (unsigned int)mesh.vertices.size() ||
                idx1 >= (unsigned int)mesh.vertices.size() ||
                idx2 >= (unsigned int)mesh.vertices.size())
                continue;

            RenderTriangle tri;
            tri.v0 = mesh.vertices[idx0];
            tri.v1 = mesh.vertices[idx1];
            tri.v2 = mesh.vertices[idx2];

            QVector3D e1 = tri.v1 - tri.v0;
            QVector3D e2 = tri.v2 - tri.v0;
            tri.normal = QVector3D::crossProduct(e1, e2).normalized();
            tri.color = mat.color;
            tri.emissive = isEmissive;
            triangles.append(tri);
        }
    }

    for (const auto &light : m_scene->lights()) {
        QVector3D v0, v1, v2, v3;
        light.getCorners(v0, v1, v2, v3);

        RenderTriangle t1;
        t1.v0 = v0; t1.v1 = v1; t1.v2 = v2;
        t1.normal = light.normal();
        t1.color = light.color * light.intensity;
        t1.emissive = true;
        triangles.append(t1);

        RenderTriangle t2;
        t2.v0 = v0; t2.v1 = v2; t2.v2 = v3;
        t2.normal = light.normal();
        t2.color = light.color * light.intensity;
        t2.emissive = true;
        triangles.append(t2);
    }

    qDebug() << "Total triangles:" << triangles.size();
    qDebug() << "Camera pos:" << eye << "target:" << cam.target();

    if (triangles.isEmpty()) {
        qWarning() << "No triangles!";
        emit finished(image);
        return;
    }

    // Build BVH
    QElapsedTimer bvhTimer;
    bvhTimer.start();
    BVH bvh;
    bvh.build(triangles);
    qDebug() << "BVH build time:" << bvhTimer.elapsed() << "ms";

    auto randf = []() -> float {
        return float(rand()) / float(RAND_MAX);
    };

    auto randomHemisphere = [&](const QVector3D &normal) -> QVector3D {
        float r1 = randf();
        float r2 = randf();
        float sinTheta = std::sqrt(1.0f - r1 * r1);
        float phi = 2.0f * 3.14159265f * r2;

        QVector3D w = normal.normalized();
        QVector3D a = (std::abs(w.x()) > 0.9f) ? QVector3D(0, 1, 0) : QVector3D(1, 0, 0);
        QVector3D u = QVector3D::crossProduct(a, w).normalized();
        QVector3D v = QVector3D::crossProduct(w, u);

        return (u * (sinTheta * std::cos(phi)) +
                v * (sinTheta * std::sin(phi)) +
                w * r1).normalized();
    };

    const auto &bvhTris = bvh.triangles();

    auto tracePath = [&](QVector3D orig, QVector3D dir) -> QVector3D {
        QVector3D throughput(1, 1, 1);
        QVector3D radiance(0, 0, 0);

        for (int bounce = 0; bounce < 4; ++bounce) {
            float t;
            int hitIdx = bvh.intersect(orig, dir, t);

            if (hitIdx < 0) {
                float sky_t = 0.5f * (dir.y() + 1.0f);
                QVector3D sky = (1.0f - sky_t) * QVector3D(0.2f, 0.2f, 0.25f) +
                                sky_t * QVector3D(0.4f, 0.5f, 0.7f);
                radiance += throughput * sky;
                break;
            }

            const RenderTriangle &tri = bvhTris[hitIdx];
            QVector3D hitPoint = orig + t * dir;
            QVector3D normal = tri.normal;

            if (QVector3D::dotProduct(normal, dir) > 0)
                normal = -normal;

            if (tri.emissive) {
                radiance += throughput * tri.color;
                break;
            }

            throughput *= tri.color;

            if (bounce > 1) {
                float p = std::max({throughput.x(), throughput.y(), throughput.z()});
                if (randf() > p) break;
                throughput /= p;
            }

            orig = hitPoint + normal * 0.001f;
            dir = randomHemisphere(normal);
        }

        return radiance;
    };

    QElapsedTimer timer;
    timer.start();

    for (int s = 0; s < m_totalSpp; ++s) {
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                float u = (2.0f * (x + randf()) / m_width - 1.0f) * aspect * tanHalf;
                float v = (1.0f - 2.0f * (y + randf()) / m_height) * tanHalf;

                QVector3D dir = (forward + right * u + up * v).normalized();
                QVector3D color = tracePath(eye, dir);

                int idx = (y * m_width + x) * 3;
                accum[idx + 0] += color.x();
                accum[idx + 1] += color.y();
                accum[idx + 2] += color.z();
            }
        }

        // Update preview
        float invS = 1.0f / (s + 1);
        for (int y = 0; y < m_height; ++y) {
            uchar *line = image.scanLine(y);
            for (int x = 0; x < m_width; ++x) {
                int idx = (y * m_width + x) * 3;
                float r = accum[idx + 0] * invS;
                float g = accum[idx + 1] * invS;
                float b = accum[idx + 2] * invS;

                line[x * 3 + 0] = (uchar)std::min(255, (int)(std::pow(std::clamp(r, 0.0f, 1.0f), 1.0f / 2.2f) * 255));
                line[x * 3 + 1] = (uchar)std::min(255, (int)(std::pow(std::clamp(g, 0.0f, 1.0f), 1.0f / 2.2f) * 255));
                line[x * 3 + 2] = (uchar)std::min(255, (int)(std::pow(std::clamp(b, 0.0f, 1.0f), 1.0f / 2.2f) * 255));
            }
        }

        float elapsed = timer.elapsed() / 1000.0f;
        qDebug() << QString("Sample %1/%2 - %3s").arg(s + 1).arg(m_totalSpp).arg(elapsed, 0, 'f', 1);
        emit progressUpdated(s + 1, m_totalSpp, image.copy());
    }

    emit finished(image.copy());
}
// ============ RenderWindow ============

RenderWindow::RenderWindow(Scene *scene, int width, int height, int spp, QWidget *parent)
    : QDialog(parent), m_width(width), m_height(height)
{
    setWindowTitle("Render");
    setMinimumSize(400, 300);
    resize(width + 40, height + 100);

    auto *layout = new QVBoxLayout(this);

    m_imageLabel = new QLabel;
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setMinimumSize(200, 150);
    m_imageLabel->setStyleSheet("background-color: black;");
    layout->addWidget(m_imageLabel, 1);

    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, spp);
    m_progressBar->setValue(0);
    layout->addWidget(m_progressBar);

    m_statusLabel = new QLabel("Starting render...");
    layout->addWidget(m_statusLabel);

    auto *btnLayout = new QHBoxLayout;
    m_saveButton = new QPushButton("Save Image...");
    m_saveButton->setEnabled(false);
    m_cancelButton = new QPushButton("Cancel");
    btnLayout->addStretch();
    btnLayout->addWidget(m_saveButton);
    btnLayout->addWidget(m_cancelButton);
    layout->addLayout(btnLayout);

    connect(m_saveButton, &QPushButton::clicked, this, &RenderWindow::saveImage);
    connect(m_cancelButton, &QPushButton::clicked, this, [this]() {
        if (m_thread && m_thread->isRunning()) {
            m_thread->terminate();
            m_thread->wait();
        }
        reject();
    });

    m_worker = new RenderWorker(scene, width, height, spp);
    m_thread = new QThread;
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_worker, &RenderWorker::process);
    connect(m_worker, &RenderWorker::progressUpdated, this, &RenderWindow::onProgressUpdated);
    connect(m_worker, &RenderWorker::finished, this, &RenderWindow::onFinished);
    connect(m_worker, &RenderWorker::finished, m_thread, &QThread::quit);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
}

RenderWindow::~RenderWindow()
{
    if (m_thread && m_thread->isRunning()) {
        m_thread->terminate();
        m_thread->wait();
    }
    delete m_thread;
}

void RenderWindow::startRender()
{
    m_thread->start();
}

void RenderWindow::onProgressUpdated(int current, int total, QImage image)
{
    m_progressBar->setValue(current);

    float percent = 100.0f * current / total;
    m_statusLabel->setText(QString("Sample %1 / %2  (%3%)")
                               .arg(current).arg(total)
                               .arg(percent, 0, 'f', 1));

    QPixmap pix = QPixmap::fromImage(image).scaled(
        m_imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_imageLabel->setPixmap(pix);
}

void RenderWindow::onFinished(QImage finalImage)
{
    m_finalImage = finalImage;
    m_saveButton->setEnabled(true);
    m_cancelButton->setText("Close");

    m_statusLabel->setText(QString("Done! %1 samples, %2x%3")
                               .arg(m_progressBar->maximum())
                               .arg(m_width).arg(m_height));

    QPixmap pix = QPixmap::fromImage(m_finalImage).scaled(
        m_imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_imageLabel->setPixmap(pix);
}

void RenderWindow::saveImage()
{
    QString path = QFileDialog::getSaveFileName(this, "Save Render",
                                                "render.png",
                                                "PNG Image (*.png);;JPEG Image (*.jpg)");
    if (path.isEmpty()) return;

    if (m_finalImage.save(path)) {
        m_statusLabel->setText("Saved: " + path);
    } else {
        QMessageBox::warning(this, "Error", "Failed to save image.");
    }
}

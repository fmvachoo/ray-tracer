#pragma once

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QImage>
#include <QThread>
#include "Scene.h"
#include "PathTracer.h"

class RenderWorker : public QObject {
    Q_OBJECT
public:
    RenderWorker(Scene *scene, int width, int height, int totalSpp);

public slots:
    void process();

signals:
    void progressUpdated(int currentSample, int totalSamples, QImage image);
    void finished(QImage finalImage);

private:
    Scene *m_scene;
    int m_width;
    int m_height;
    int m_totalSpp;
};

class RenderWindow : public QDialog {
    Q_OBJECT
public:
    RenderWindow(Scene *scene, int width, int height, int spp, QWidget *parent = nullptr);
    ~RenderWindow() override;

    void startRender();

private slots:
    void onProgressUpdated(int current, int total, QImage image);
    void onFinished(QImage finalImage);
    void saveImage();

private:
    QLabel *m_imageLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_cancelButton = nullptr;

    QThread *m_thread = nullptr;
    RenderWorker *m_worker = nullptr;

    QImage m_finalImage;
    int m_width;
    int m_height;
};
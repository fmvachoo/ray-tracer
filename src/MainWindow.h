#pragma once

#include <QMainWindow>
#include "Scene.h"
#include "Viewport.h"
#include "PropertiesPanel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();
    void showViewport();
    void showRenderPreview();
    void startRender();
    void onViewportInitialized();

private:
    void setupUI();
    void setupMenuBar();

    Viewport *m_viewport = nullptr;
    PropertiesPanel *m_propertiesPanel = nullptr;
    Scene m_scene;
    QString m_currentFilePath;
    bool m_viewportReady = false;
    bool m_pendingNewFile = false;
};
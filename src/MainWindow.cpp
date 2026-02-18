#include "MainWindow.h"
#include "RenderWindow.h"
#include <QApplication>
#include <QMenuBar>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUI();
    setupMenuBar();
    m_pendingNewFile = true;
    resize(1200, 700);
}

void MainWindow::setupMenuBar()
{
    QMenuBar *mb = menuBar();

    // File menu
    QMenu *fileMenu = mb->addMenu("File");
    fileMenu->addAction("New", this, &MainWindow::newFile, QKeySequence::New);
    fileMenu->addAction("Open...", this, &MainWindow::openFile, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("Save", this, &MainWindow::saveFile, QKeySequence::Save);
    fileMenu->addAction("Save As...", this, &MainWindow::saveFileAs, QKeySequence("Ctrl+Shift+S"));

    // View menu
    QMenu *viewMenu = mb->addMenu("View");
    viewMenu->addAction("Viewport", this, &MainWindow::showViewport, QKeySequence("F5"));
    viewMenu->addAction("Render Preview", this, &MainWindow::showRenderPreview, QKeySequence("F6"));
    viewMenu->addSeparator();
    viewMenu->addAction("Render", this, &MainWindow::startRender, QKeySequence("F7"));
}

void MainWindow::setupUI()
{
    auto *central = new QWidget;
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_viewport = new Viewport;
    m_propertiesPanel = new PropertiesPanel;

    layout->addWidget(m_viewport, 1);
    layout->addWidget(m_propertiesPanel);

    setCentralWidget(central);
    statusBar()->showMessage("Ready");

    connect(m_propertiesPanel, &PropertiesPanel::sceneChanged, m_viewport, [this]() {
        m_viewport->setPreviewMode();
        m_viewport->update();
    });

    connect(m_propertiesPanel, &PropertiesPanel::renderRequested,
            this, &MainWindow::startRender);

    connect(m_viewport, &Viewport::initialized,
            this, &MainWindow::onViewportInitialized);
}

void MainWindow::onViewportInitialized()
{
    m_viewportReady = true;
    if (m_pendingNewFile) {
        m_pendingNewFile = false;
        newFile();
    }
}

void MainWindow::newFile()
{
    if (!m_viewportReady) {
        m_pendingNewFile = true;
        return;
    }

    m_scene.createDefault();
    m_viewport->setScene(&m_scene);
    m_propertiesPanel->setScene(&m_scene);
    m_currentFilePath.clear();
    statusBar()->showMessage("New scene created");
}

void MainWindow::openFile()
{
    QString path = QFileDialog::getOpenFileName(this, "Open Scene", QString(), "Scene Files (*.scene)");
    if (path.isEmpty()) return;

    if (!m_scene.load(path)) {
        QMessageBox::warning(this, "Error", "Failed to load scene file.");
        return;
    }

    m_viewport->setScene(&m_scene);
    m_propertiesPanel->setScene(&m_scene);
    m_currentFilePath = path;
    statusBar()->showMessage("Loaded: " + path);
}

void MainWindow::saveFile()
{
    if (m_currentFilePath.isEmpty()) {
        saveFileAs();
        return;
    }

    if (!m_scene.save(m_currentFilePath)) {
        QMessageBox::warning(this, "Error", "Failed to save scene file.");
        return;
    }
    statusBar()->showMessage("Saved: " + m_currentFilePath);
}

void MainWindow::saveFileAs()
{
    QString path = QFileDialog::getSaveFileName(this, "Save Scene", QString(), "Scene Files (*.scene)");
    if (path.isEmpty()) return;

    if (!path.endsWith(".scene"))
        path += ".scene";

    m_currentFilePath = path;
    saveFile();
}

void MainWindow::showViewport()
{
    m_viewport->setPreviewMode();
    m_viewport->update();
    statusBar()->showMessage("Viewport mode");
}

void MainWindow::showRenderPreview()
{
    int spp = m_propertiesPanel->viewportSamples();
    statusBar()->showMessage(QString("Viewport render preview (%1 spp)...").arg(spp));
    QApplication::processEvents();

    m_viewport->renderPathTraced(spp);
    statusBar()->showMessage("Preview complete");
}

void MainWindow::startRender()
{
    int spp = m_propertiesPanel->renderSamples();

    // Get render resolution from properties panel
    QSpinBox *wSpin = m_propertiesPanel->findChild<QSpinBox *>("", Qt::FindDirectChildrenOnly);

    // Use default or get from panel
    int w = 800, h = 600;

    // Find width/height spinboxes
    QList<QSpinBox *> spins = m_propertiesPanel->findChildren<QSpinBox *>();
    for (auto *spin : spins) {
        if (spin->maximum() == 4096) {
            if (w == 800) { w = spin->value(); }
            else { h = spin->value(); break; }
        }
    }

    statusBar()->showMessage(QString("Rendering %1x%2 @ %3 spp...").arg(w).arg(h).arg(spp));

    auto *renderWin = new RenderWindow(&m_scene, w, h, spp, this);
    renderWin->setAttribute(Qt::WA_DeleteOnClose);
    renderWin->show();
    renderWin->startRender();
}
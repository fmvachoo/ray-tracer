#include "PropertiesPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QLabel>
#include <QInputDialog>

PropertiesPanel::PropertiesPanel(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(300);
    buildUI();
}

void PropertiesPanel::buildUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    m_tabs = new QTabWidget;
    mainLayout->addWidget(m_tabs);

    auto *objectTab = new QWidget;
    auto *lightTab = new QWidget;
    auto *renderTab = new QWidget;

    buildObjectTab(objectTab);
    buildLightTab(lightTab);
    buildRenderTab(renderTab);

    m_tabs->addTab(objectTab, "Objects");
    m_tabs->addTab(lightTab, "Lights");
    m_tabs->addTab(renderTab, "Render");
}

// ======================== OBJECT TAB ========================

void PropertiesPanel::buildObjectTab(QWidget *tab)
{
    auto *layout = new QVBoxLayout(tab);

    m_objectCombo = new QComboBox;
    layout->addWidget(m_objectCombo);

    connect(m_objectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertiesPanel::onObjectSelected);

    auto *matGroup = new QGroupBox("Material");
    auto *matLayout = new QFormLayout(matGroup);

    m_colorPreview = new QLabel;
    m_colorPreview->setFixedSize(40, 20);
    m_colorPreview->setAutoFillBackground(true);
    matLayout->addRow("Color:", m_colorPreview);

    m_redSlider = new QSlider(Qt::Horizontal);
    m_redSlider->setRange(0, 255);
    matLayout->addRow("R:", m_redSlider);

    m_greenSlider = new QSlider(Qt::Horizontal);
    m_greenSlider->setRange(0, 255);
    matLayout->addRow("G:", m_greenSlider);

    m_blueSlider = new QSlider(Qt::Horizontal);
    m_blueSlider->setRange(0, 255);
    matLayout->addRow("B:", m_blueSlider);

    m_roughnessSlider = new QSlider(Qt::Horizontal);
    m_roughnessSlider->setRange(0, 100);
    m_roughnessLabel = new QLabel("0.50");
    matLayout->addRow("Roughness:", m_roughnessSlider);
    matLayout->addRow("", m_roughnessLabel);

    m_transparencySlider = new QSlider(Qt::Horizontal);
    m_transparencySlider->setRange(0, 100);
    m_transparencyLabel = new QLabel("0.00");
    matLayout->addRow("Transparency:", m_transparencySlider);
    matLayout->addRow("", m_transparencyLabel);

    m_iorSlider = new QSlider(Qt::Horizontal);
    m_iorSlider->setRange(100, 250);
    m_iorLabel = new QLabel("1.50");
    matLayout->addRow("IOR:", m_iorSlider);
    matLayout->addRow("", m_iorLabel);

    layout->addWidget(matGroup);
    layout->addStretch();

    auto onMaterialChanged = [this]() {
        int idx = m_objectCombo->currentIndex();
        if (!m_scene || idx < 0 || idx >= m_scene->objects().size()) return;

        auto &mat = m_scene->objects()[idx]->material();
        mat.color = QVector3D(m_redSlider->value() / 255.0f,
                              m_greenSlider->value() / 255.0f,
                              m_blueSlider->value() / 255.0f);
        mat.roughness = m_roughnessSlider->value() / 100.0f;
        mat.transparency = m_transparencySlider->value() / 100.0f;
        mat.ior = m_iorSlider->value() / 100.0f;

        m_roughnessLabel->setText(QString::number(mat.roughness, 'f', 2));
        m_transparencyLabel->setText(QString::number(mat.transparency, 'f', 2));
        m_iorLabel->setText(QString::number(mat.ior, 'f', 2));

        QPalette pal = m_colorPreview->palette();
        pal.setColor(QPalette::Window, QColor(m_redSlider->value(),
                                              m_greenSlider->value(),
                                              m_blueSlider->value()));
        m_colorPreview->setPalette(pal);

        emit sceneChanged();
    };

    connect(m_redSlider, &QSlider::valueChanged, this, onMaterialChanged);
    connect(m_greenSlider, &QSlider::valueChanged, this, onMaterialChanged);
    connect(m_blueSlider, &QSlider::valueChanged, this, onMaterialChanged);
    connect(m_roughnessSlider, &QSlider::valueChanged, this, onMaterialChanged);
    connect(m_transparencySlider, &QSlider::valueChanged, this, onMaterialChanged);
    connect(m_iorSlider, &QSlider::valueChanged, this, onMaterialChanged);
}

void PropertiesPanel::onObjectSelected(int index)
{
    if (!m_scene || index < 0 || index >= m_scene->objects().size()) return;
    updateMaterialUI();
}

void PropertiesPanel::updateMaterialUI()
{
    int idx = m_objectCombo->currentIndex();
    if (!m_scene || idx < 0 || idx >= m_scene->objects().size()) return;

    const auto &mat = m_scene->objects()[idx]->material();

    m_redSlider->blockSignals(true);
    m_greenSlider->blockSignals(true);
    m_blueSlider->blockSignals(true);
    m_roughnessSlider->blockSignals(true);
    m_transparencySlider->blockSignals(true);
    m_iorSlider->blockSignals(true);

    m_redSlider->setValue(int(mat.color.x() * 255));
    m_greenSlider->setValue(int(mat.color.y() * 255));
    m_blueSlider->setValue(int(mat.color.z() * 255));
    m_roughnessSlider->setValue(int(mat.roughness * 100));
    m_transparencySlider->setValue(int(mat.transparency * 100));
    m_iorSlider->setValue(int(mat.ior * 100));

    m_roughnessLabel->setText(QString::number(mat.roughness, 'f', 2));
    m_transparencyLabel->setText(QString::number(mat.transparency, 'f', 2));
    m_iorLabel->setText(QString::number(mat.ior, 'f', 2));

    QPalette pal = m_colorPreview->palette();
    pal.setColor(QPalette::Window, QColor(int(mat.color.x() * 255),
                                          int(mat.color.y() * 255),
                                          int(mat.color.z() * 255)));
    m_colorPreview->setPalette(pal);

    m_redSlider->blockSignals(false);
    m_greenSlider->blockSignals(false);
    m_blueSlider->blockSignals(false);
    m_roughnessSlider->blockSignals(false);
    m_transparencySlider->blockSignals(false);
    m_iorSlider->blockSignals(false);
}

// ======================== LIGHT TAB ========================

void PropertiesPanel::buildLightTab(QWidget *tab)
{
    auto *layout = new QVBoxLayout(tab);

    // List + buttons
    m_lightList = new QListWidget;
    layout->addWidget(m_lightList);

    auto *btnRow = new QHBoxLayout;
    m_addLightBtn = new QPushButton("+");
    m_removeLightBtn = new QPushButton("-");
    m_addLightBtn->setFixedWidth(40);
    m_removeLightBtn->setFixedWidth(40);
    btnRow->addWidget(m_addLightBtn);
    btnRow->addWidget(m_removeLightBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    // Properties
    auto *propGroup = new QGroupBox("Light Properties");
    auto *propLayout = new QFormLayout(propGroup);

    // Position
    m_lightPosX = new QDoubleSpinBox; m_lightPosX->setRange(-50, 50); m_lightPosX->setSingleStep(0.1); m_lightPosX->setDecimals(2);
    m_lightPosY = new QDoubleSpinBox; m_lightPosY->setRange(-50, 50); m_lightPosY->setSingleStep(0.1); m_lightPosY->setDecimals(2);
    m_lightPosZ = new QDoubleSpinBox; m_lightPosZ->setRange(-50, 50); m_lightPosZ->setSingleStep(0.1); m_lightPosZ->setDecimals(2);
    propLayout->addRow("Pos X:", m_lightPosX);
    propLayout->addRow("Pos Y:", m_lightPosY);
    propLayout->addRow("Pos Z:", m_lightPosZ);

    // Rotation
    m_lightRotX = new QDoubleSpinBox; m_lightRotX->setRange(-180, 180); m_lightRotX->setSingleStep(5);
    m_lightRotY = new QDoubleSpinBox; m_lightRotY->setRange(-180, 180); m_lightRotY->setSingleStep(5);
    m_lightRotZ = new QDoubleSpinBox; m_lightRotZ->setRange(-180, 180); m_lightRotZ->setSingleStep(5);
    propLayout->addRow("Rot X:", m_lightRotX);
    propLayout->addRow("Rot Y:", m_lightRotY);
    propLayout->addRow("Rot Z:", m_lightRotZ);

    // Color
    m_lightColorPreview = new QLabel;
    m_lightColorPreview->setFixedSize(40, 20);
    m_lightColorPreview->setAutoFillBackground(true);
    propLayout->addRow("Color:", m_lightColorPreview);

    m_lightRed = new QSlider(Qt::Horizontal); m_lightRed->setRange(0, 255);
    m_lightGreen = new QSlider(Qt::Horizontal); m_lightGreen->setRange(0, 255);
    m_lightBlue = new QSlider(Qt::Horizontal); m_lightBlue->setRange(0, 255);
    propLayout->addRow("R:", m_lightRed);
    propLayout->addRow("G:", m_lightGreen);
    propLayout->addRow("B:", m_lightBlue);

    // Intensity, size
    m_lightIntensity = new QDoubleSpinBox; m_lightIntensity->setRange(0.1, 100); m_lightIntensity->setSingleStep(0.5); m_lightIntensity->setDecimals(1);
    m_lightWidth = new QDoubleSpinBox; m_lightWidth->setRange(0.1, 20); m_lightWidth->setSingleStep(0.1); m_lightWidth->setDecimals(2);
    m_lightHeight = new QDoubleSpinBox; m_lightHeight->setRange(0.1, 20); m_lightHeight->setSingleStep(0.1); m_lightHeight->setDecimals(2);
    propLayout->addRow("Intensity:", m_lightIntensity);
    propLayout->addRow("Width:", m_lightWidth);
    propLayout->addRow("Height:", m_lightHeight);

    layout->addWidget(propGroup);
    layout->addStretch();

    // Connections
    connect(m_lightList, &QListWidget::currentRowChanged, this, &PropertiesPanel::onLightSelected);

    connect(m_addLightBtn, &QPushButton::clicked, this, [this]() {
        if (!m_scene) return;
        int num = m_scene->lights().size() + 1;
        Light light;
        light.name = QString("Light %1").arg(num);
        light.position = QVector3D(0, 2.5f, 0);
        light.color = QVector3D(1, 1, 1);
        light.intensity = 5.0f;
        light.width = 1.0f;
        light.height = 1.0f;
        m_scene->addLight(light);
        refreshLightList();
        m_lightList->setCurrentRow(m_scene->lights().size() - 1);
        emit sceneChanged();
    });

    connect(m_removeLightBtn, &QPushButton::clicked, this, [this]() {
        int idx = m_lightList->currentRow();
        if (!m_scene || idx < 0) return;
        m_scene->removeLight(idx);
        refreshLightList();
        emit sceneChanged();
    });

    auto onLightPropChanged = [this]() {
        int idx = m_lightList->currentRow();
        if (!m_scene || idx < 0 || idx >= m_scene->lights().size()) return;

        Light &light = m_scene->lights()[idx];
        light.position = QVector3D(m_lightPosX->value(), m_lightPosY->value(), m_lightPosZ->value());
        light.rotation = QVector3D(m_lightRotX->value(), m_lightRotY->value(), m_lightRotZ->value());
        light.color = QVector3D(m_lightRed->value() / 255.0f,
                                m_lightGreen->value() / 255.0f,
                                m_lightBlue->value() / 255.0f);
        light.intensity = m_lightIntensity->value();
        light.width = m_lightWidth->value();
        light.height = m_lightHeight->value();

        QPalette pal = m_lightColorPreview->palette();
        pal.setColor(QPalette::Window, QColor(m_lightRed->value(), m_lightGreen->value(), m_lightBlue->value()));
        m_lightColorPreview->setPalette(pal);

        emit sceneChanged();
    };

    connect(m_lightPosX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onLightPropChanged);
    connect(m_lightPosY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onLightPropChanged);
    connect(m_lightPosZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onLightPropChanged);
    connect(m_lightRotX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onLightPropChanged);
    connect(m_lightRotY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onLightPropChanged);
    connect(m_lightRotZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onLightPropChanged);
    connect(m_lightRed, &QSlider::valueChanged, this, onLightPropChanged);
    connect(m_lightGreen, &QSlider::valueChanged, this, onLightPropChanged);
    connect(m_lightBlue, &QSlider::valueChanged, this, onLightPropChanged);
    connect(m_lightIntensity, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onLightPropChanged);
    connect(m_lightWidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onLightPropChanged);
    connect(m_lightHeight, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, onLightPropChanged);
}

void PropertiesPanel::onLightSelected(int index)
{
    if (!m_scene || index < 0 || index >= m_scene->lights().size()) return;
    updateLightUI();
}

void PropertiesPanel::updateLightUI()
{
    int idx = m_lightList->currentRow();
    if (!m_scene || idx < 0 || idx >= m_scene->lights().size()) return;

    const Light &light = m_scene->lights()[idx];

    // Block signals to avoid feedback
    auto block = [](QObject *obj, bool b) { obj->blockSignals(b); };
    QList<QObject*> widgets = {m_lightPosX, m_lightPosY, m_lightPosZ,
                                m_lightRotX, m_lightRotY, m_lightRotZ,
                                m_lightRed, m_lightGreen, m_lightBlue,
                                m_lightIntensity, m_lightWidth, m_lightHeight};
    for (auto *w : widgets) w->blockSignals(true);

    m_lightPosX->setValue(light.position.x());
    m_lightPosY->setValue(light.position.y());
    m_lightPosZ->setValue(light.position.z());
    m_lightRotX->setValue(light.rotation.x());
    m_lightRotY->setValue(light.rotation.y());
    m_lightRotZ->setValue(light.rotation.z());
    m_lightRed->setValue(int(light.color.x() * 255));
    m_lightGreen->setValue(int(light.color.y() * 255));
    m_lightBlue->setValue(int(light.color.z() * 255));
    m_lightIntensity->setValue(light.intensity);
    m_lightWidth->setValue(light.width);
    m_lightHeight->setValue(light.height);

    QPalette pal = m_lightColorPreview->palette();
    pal.setColor(QPalette::Window, QColor(int(light.color.x() * 255),
                                          int(light.color.y() * 255),
                                          int(light.color.z() * 255)));
    m_lightColorPreview->setPalette(pal);

    for (auto *w : widgets) w->blockSignals(false);
}

void PropertiesPanel::refreshLightList()
{
    m_lightList->clear();
    if (!m_scene) return;
    for (const auto &l : m_scene->lights()) {
        m_lightList->addItem(l.name);
    }
}

// ======================== RENDER TAB ========================

void PropertiesPanel::buildRenderTab(QWidget *tab)
{
    auto *layout = new QVBoxLayout(tab);

    auto *vpGroup = new QGroupBox("Viewport");
    auto *vpLayout = new QFormLayout(vpGroup);
    m_viewportSamplesSpin = new QSpinBox;
    m_viewportSamplesSpin->setRange(1, 64);
    m_viewportSamplesSpin->setValue(4);
    vpLayout->addRow("Samples:", m_viewportSamplesSpin);
    layout->addWidget(vpGroup);

    connect(m_viewportSamplesSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PropertiesPanel::viewportSamplesChanged);

    auto *renderGroup = new QGroupBox("Render Output");
    auto *renderLayout = new QFormLayout(renderGroup);

    m_renderSamplesSpin = new QSpinBox;
    m_renderSamplesSpin->setRange(1, 10000);
    m_renderSamplesSpin->setValue(128);
    renderLayout->addRow("Samples:", m_renderSamplesSpin);

    m_renderWidthSpin = new QSpinBox;
    m_renderWidthSpin->setRange(64, 4096);
    m_renderWidthSpin->setValue(320);
    renderLayout->addRow("Width:", m_renderWidthSpin);

    m_renderHeightSpin = new QSpinBox;
    m_renderHeightSpin->setRange(64, 4096);
    m_renderHeightSpin->setValue(240);
    renderLayout->addRow("Height:", m_renderHeightSpin);

    m_renderButton = new QPushButton("Render");
    renderLayout->addRow(m_renderButton);
    layout->addWidget(renderGroup);

    connect(m_renderButton, &QPushButton::clicked, this, [this]() {
        emit renderRequested(m_renderSamplesSpin->value());
    });

    layout->addStretch();
}

void PropertiesPanel::setScene(Scene *scene)
{
    m_scene = scene;
    m_objectCombo->clear();

    if (!m_scene) return;

    for (const auto &obj : m_scene->objects()) {
        m_objectCombo->addItem(obj->name());
    }

    if (!m_scene->objects().isEmpty()) {
        m_objectCombo->setCurrentIndex(0);
        onObjectSelected(0);
    }

    refreshLightList();
    if (!m_scene->lights().isEmpty()) {
        m_lightList->setCurrentRow(0);
    }
}

int PropertiesPanel::viewportSamples() const
{
    return m_viewportSamplesSpin->value();
}

int PropertiesPanel::renderSamples() const
{
    return m_renderSamplesSpin->value();
}

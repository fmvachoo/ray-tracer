#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QTabWidget>
#include <QListWidget>
#include "Scene.h"

class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget *parent = nullptr);

    void setScene(Scene *scene);
    int viewportSamples() const;
    int renderSamples() const;

signals:
    void sceneChanged();
    void viewportSamplesChanged(int spp);
    void renderRequested(int spp);

private:
    void buildUI();
    void buildObjectTab(QWidget *tab);
    void buildLightTab(QWidget *tab);
    void buildRenderTab(QWidget *tab);

    void onObjectSelected(int index);
    void updateMaterialUI();

    void onLightSelected(int index);
    void updateLightUI();
    void refreshLightList();

    Scene *m_scene = nullptr;

    QTabWidget *m_tabs = nullptr;

    // --- Object tab ---
    QComboBox *m_objectCombo = nullptr;
    QSlider *m_redSlider = nullptr;
    QSlider *m_greenSlider = nullptr;
    QSlider *m_blueSlider = nullptr;
    QLabel *m_colorPreview = nullptr;
    QSlider *m_roughnessSlider = nullptr;
    QLabel *m_roughnessLabel = nullptr;
    QSlider *m_transparencySlider = nullptr;
    QLabel *m_transparencyLabel = nullptr;
    QSlider *m_iorSlider = nullptr;
    QLabel *m_iorLabel = nullptr;

    // --- Light tab ---
    QListWidget *m_lightList = nullptr;
    QPushButton *m_addLightBtn = nullptr;
    QPushButton *m_removeLightBtn = nullptr;

    QDoubleSpinBox *m_lightPosX = nullptr;
    QDoubleSpinBox *m_lightPosY = nullptr;
    QDoubleSpinBox *m_lightPosZ = nullptr;

    QDoubleSpinBox *m_lightRotX = nullptr;
    QDoubleSpinBox *m_lightRotY = nullptr;
    QDoubleSpinBox *m_lightRotZ = nullptr;

    QSlider *m_lightRed = nullptr;
    QSlider *m_lightGreen = nullptr;
    QSlider *m_lightBlue = nullptr;
    QLabel *m_lightColorPreview = nullptr;

    QDoubleSpinBox *m_lightIntensity = nullptr;
    QDoubleSpinBox *m_lightWidth = nullptr;
    QDoubleSpinBox *m_lightHeight = nullptr;

    // --- Render tab ---
    QSpinBox *m_viewportSamplesSpin = nullptr;
    QSpinBox *m_renderSamplesSpin = nullptr;
    QSpinBox *m_renderWidthSpin = nullptr;
    QSpinBox *m_renderHeightSpin = nullptr;
    QPushButton *m_renderButton = nullptr;
};

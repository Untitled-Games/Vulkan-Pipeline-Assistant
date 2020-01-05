#include "vulkanmain.h"
#include "vulkanrenderer.h"

#include <QVulkanWindow>
#include <QVulkanFunctions>
#include <QLoggingCategory>
#include <QLayout>
#include <QMessageBox>

using namespace vpa;

class VulkanWindow : public QVulkanWindow {
public:
    VulkanWindow(VulkanMain* main) : m_main(main) { }
    QVulkanWindowRenderer* createRenderer() override {
        return new VulkanRenderer(this, m_main);
    }
private:
    VulkanMain* m_main;
};

VulkanMain::VulkanMain(QWidget* parent) {
    CreateVkInstance();
    m_vkWindow = new VulkanWindow(this);
    m_vkWindow->setVulkanInstance(&m_instance);
    m_vkWindow->show();

    m_container = QWidget::createWindowContainer(m_vkWindow);
    m_container->setFocusPolicy(Qt::NoFocus);
    parent->layout()->addWidget(m_container);
}

VulkanMain::~VulkanMain() {
    delete m_vkWindow;
    delete m_container;
}

void VulkanMain::CreateVkInstance() {
    m_instance.setLayers(QByteArrayList()
         << "VK_LAYER_GOOGLE_threading"
         << "VK_LAYER_LUNARG_parameter_validation"
         << "VK_LAYER_LUNARG_object_tracker"
         << "VK_LAYER_LUNARG_standard_validation"
         << "VK_LAYER_LUNARG_image"
         << "VK_LAYER_LUNARG_swapchain"
         << "VK_LAYER_GOOGLE_unique_objects");
    m_instance.setExtensions(QByteArrayList() << VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    if (!m_instance.create()) {
        qFatal("Could not create Vulkan Instance %d", m_instance.errorCode());
    }
}

void VulkanMain::WritePipelineCache() {
    if (m_renderer->WritePipelineCache()) {
        QMessageBox::information(m_container, "VulkanPipelineAssistant", "Pipeline cache has been written to test.vpac");
    }
    if (m_renderer->WritePipelineConfig()) {
        QMessageBox::information(m_container, "VulkanPipelineAssistant", "Pipeline config has been written to config.vpa");
    }
    if (m_renderer->ReadPipelineConfig()) {
        QMessageBox::information(m_container, "VulkanPipelineAssistant", "Pipeline config has been read from config.vpa");
    }
}

void VulkanMain::WritePipelineConfig() {
    if (m_renderer->WritePipelineConfig()) {
        QMessageBox::information(m_container, "VulkanPipelineAssistant", "Pipeline config has been written to config.vpa");
    }
}

PipelineConfig& VulkanMain::GetConfig() {
    return m_renderer->GetConfig();
}

Descriptors* VulkanMain::GetDescriptors() {
    return m_renderer->GetDescriptors();
}

void VulkanMain::Reload(const ReloadFlags flag) {
    m_renderer->Reload(flag);
}

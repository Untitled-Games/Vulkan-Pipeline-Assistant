/*
 * Author: Ralph Ridley
 * Date: 20/12/19
 */

#include "vulkanmain.h"
#include "vulkanrenderer.h"

#include <QVulkanWindow>
#include <QWidget>
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
         << "VK_LAYER_LUNARG_core_validation"
         << "VK_LAYER_LUNARG_image"
         << "VK_LAYER_LUNARG_swapchain"
         << "VK_LAYER_GOOGLE_unique_objects");

    if (!m_instance.create()) {
        qFatal("Could not create Vulkan Instance %d", m_instance.errorCode());
    }
}

void VulkanMain::WritePipelineCache() {
    if (m_renderer->WritePipelineCache()) {
        QMessageBox::information(m_container, "VulkanPipelineAssistant", "Pipeline cache has been written to test.vpac");
    }
}

PipelineConfig& VulkanMain::GetConfig() {
    return m_renderer->GetConfig();
}

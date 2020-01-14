#include "vulkanmain.h"
#include "vulkanrenderer.h"

#include <QVulkanWindow>
#include <QVulkanFunctions>
#include <QLoggingCategory>
#include <QLayout>
#include <QWidget>

#include "common.h"

namespace vpa {
    class VulkanWindow : public QVulkanWindow {
    public:
        VulkanWindow(VulkanMain* main, std::function<void(void)> creationCallback, std::function<void(void)> postInitCallback)
            : m_main(main), m_creationCallback(creationCallback), m_postInitCallback(postInitCallback) { }
        QVulkanWindowRenderer* createRenderer() override {
            return new VulkanRenderer(this, m_main, m_creationCallback, m_postInitCallback);
        }
    private:
        VulkanMain* m_main;
        std::function<void(void)> m_creationCallback;
        std::function<void(void)> m_postInitCallback;
    };

    VulkanMain::VulkanMain(QWidget* parent, std::function<void(void)> creationCallback, std::function<void(void)> postInitCallback)
        : m_vkWindow(nullptr), m_renderer(nullptr), m_container(nullptr) {
        CreateVkInstance();
        m_vkWindow = new VulkanWindow(this, creationCallback, postInitCallback);
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
        if (!m_instance.create())qFatal("Could not create Vulkan Instance %d", m_instance.errorCode());
    }

    void VulkanMain::WritePipelineCache() {
        if (m_renderer->WritePipelineCache().level == VPAErrorLevel::Ok) qDebug("Pipeline cache has been written to test.vpac");
        if (m_renderer->WritePipelineConfig().level == VPAErrorLevel::Ok) qDebug("Pipeline config has been written to config.vpa");
        if (m_renderer->ReadPipelineConfig().level == VPAErrorLevel::Ok) qDebug("Pipeline config has been read from config.vpa");
    }

    void VulkanMain::WritePipelineConfig() {
        if (m_renderer->WritePipelineConfig().level == VPAErrorLevel::Ok) {
            qDebug("Pipeline config has been written to config.vpa");
        }
    }

    PipelineConfig& VulkanMain::GetConfig() {
        return m_renderer->GetConfig();
    }

    Descriptors* VulkanMain::GetDescriptors() {
        return m_renderer ? m_renderer->GetDescriptors() : nullptr;
    }

    const VkPhysicalDeviceLimits& VulkanMain::Limits() const {
        return m_vkWindow->physicalDeviceProperties()->limits;
    }

    void VulkanMain::Reload(const ReloadFlags flag) {
        VPAError err = m_renderer->Reload(flag);
        if (err != VPA_OK) {
            m_renderer->SetValid(false);
            qDebug(qPrintable(err.message));
        }
        else {
            m_renderer->SetValid(true);
        }
    }
}

#ifndef VULKANMAIN_H
#define VULKANMAIN_H

#include <QVulkanInstance>
#include "reloadflags.h"

#define DEBUG_VKRESULT(result, name) if (result != VK_SUCCESS) qDebug("VkResult for %s, error code %i", name, qPrintable(QString::number(result, 16)))
#define WARNING_VKRESULT(result, name) if (result != VK_SUCCESS) qWarning("VkResult for %s, error code %i", name, qPrintable(QString::number(result, 16)))
#define FATAL_VKRESULT(result, name) if (result != VK_SUCCESS) qFatal("VkResult for %s, error code %i", name, qPrintable(QString::number(result, 16)))

class QVulkanWindow;
class QWidget;
namespace vpa {
    struct PipelineConfig;
    class VulkanRenderer;
    class MemoryAllocator;
    class VulkanMain {
        friend class VulkanRenderer;
    public:
        VulkanMain(QWidget* parent);
        ~VulkanMain();

        void WritePipelineCache();
        void WritePipelineConfig();
        PipelineConfig& GetConfig();

        //@TODO is this ok?
         void Reload(const ReloadFlags flag);

    private:
        void CreateVkInstance();

        QVulkanWindow* m_vkWindow;
        QVulkanInstance m_instance;
        VulkanRenderer* m_renderer;
        QWidget* m_container;
    };
}

#endif // VULKANMAIN_H

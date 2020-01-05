#ifndef VULKANMAIN_H
#define VULKANMAIN_H

#include <QVulkanInstance>
#include "reloadflags.h"

#define DEBUG_VKRESULT(result, name) if (result != VK_SUCCESS) qDebug("VkResult for %s, error code %i", name, result)
#define WARNING_VKRESULT(result, name) if (result != VK_SUCCESS) qWarning("VkResult for %s, error code %i", name, result)
#define FATAL_VKRESULT(result, name) if (result != VK_SUCCESS) qFatal("VkResult for %s, error code %i", name, result)
#define DESTROY_HANDLE(device, handle, func) if (handle != VK_NULL_HANDLE) { func(device, handle, nullptr); handle = VK_NULL_HANDLE; }

class QVulkanWindow;
class QWidget;
namespace vpa {
    struct PipelineConfig;
    class VulkanRenderer;
    class MemoryAllocator;
    class Descriptors;
    class VulkanMain {
        friend class VulkanRenderer;
    public:
        VulkanMain(QWidget* parent);
        ~VulkanMain();

        void WritePipelineCache();
        void WritePipelineConfig();
        PipelineConfig& GetConfig();

         void Reload(const ReloadFlags flag);

        Descriptors* GetDescriptors();

    private:
        void CreateVkInstance();

        QVulkanWindow* m_vkWindow;
        QVulkanInstance m_instance;
        VulkanRenderer* m_renderer;
        QWidget* m_container;
    };
}

#endif // VULKANMAIN_H

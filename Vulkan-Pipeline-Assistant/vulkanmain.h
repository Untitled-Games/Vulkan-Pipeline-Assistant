#ifndef VULKANMAIN_H
#define VULKANMAIN_H

#include <QVulkanInstance>

#include "reloadflags.h"

class QVulkanWindow;
class QWidget;

namespace vpa {
    struct PipelineConfig;
    class VulkanRenderer;
    class MemoryAllocator;
    class Descriptors;

    class VulkanMain final {
        friend class VulkanWindow;
        friend class VulkanRenderer;
    public:
        VulkanMain(QWidget* parent, std::function<void(void)> creationCallback, std::function<void(void)> postInitCallback);
        ~VulkanMain();

        void WritePipelineCache();
        void WritePipelineConfig();
        PipelineConfig& GetConfig();

         void Reload(const ReloadFlags flag);

        Descriptors* GetDescriptors();
        const VkPhysicalDeviceLimits& Limits() const;

    private:
        void CreateVkInstance();

        QVulkanWindow* m_vkWindow;
        QVulkanInstance m_instance;
        VulkanRenderer* m_renderer;
        QWidget* m_container;
    };
}

#endif // VULKANMAIN_H

/*
 * Author: Ralph Ridley
 * Date: 20/12/19
 * Functionality for the Vulkan window.
 */

#ifndef VULKANMAIN_H
#define VULKANMAIN_H

#include <QVulkanWindow>
#include "pipelineconfig.h"

namespace vpa {
    class VulkanRenderer;
    class VulkanMain {
        friend class VulkanRenderer;
    public:
        VulkanMain(QWidget* parent);
        ~VulkanMain();

        void WritePipelineCache();
        PipelineConfig& GetConfig();
    private:
        void CreateVkInstance();

        void UpdatePipeline();

        QVulkanWindow* m_vkWindow;
        QVulkanInstance m_instance;
        VulkanRenderer* m_renderer;
        QWidget* m_container;
    };
}

#endif // VULKANMAIN_H

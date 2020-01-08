#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QVulkanWindowRenderer>

#include "pipelineconfig.h"
#include "memoryallocator.h"
#include "reloadflags.h"

namespace vpa {
    class VulkanMain;
    class ShaderAnalytics;
    class VertexInput;
    class Descriptors;

    class VulkanRenderer : public QVulkanWindowRenderer {
    public:
        VulkanRenderer(QVulkanWindow* window, VulkanMain* main, std::function<void(void)> creationCallback);

        void initResources() override;
        void initSwapChainResources() override;
        void releaseSwapChainResources() override;
        void releaseResources() override;

        void startNextFrame() override;

        PipelineConfig& GetConfig() {
            return m_config;
        }

        Descriptors* GetDescriptors() {
            return m_descriptors;
        }

        VPAError WritePipelineCache();

        VPAError WritePipelineConfig();
        VPAError ReadPipelineConfig();
        VPAError Reload(const ReloadFlags flag);
    private:
        VPAError CreateRenderPass();
        VPAError CreatePipeline();
        VPAError CreatePipelineCache();
        VPAError CreateShaders();

        VkAttachmentDescription makeAttachment(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
            VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp, VkImageLayout initialLayout, VkImageLayout finalLayout);
        VkSubpassDescription makeSubpass(VkPipelineBindPoint pipelineType, QVector<VkAttachmentReference>& colourReferences,
            VkAttachmentReference* depthReference, VkAttachmentReference* resolve);
        VkSubpassDependency makeSubpassDependency(uint32_t srcIdx, uint32_t dstIdx, VkPipelineStageFlags srcStage,
            VkAccessFlags srcAccess, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess);

        bool m_initialised;
        QVulkanWindow* m_window;
        QVulkanDeviceFunctions* m_deviceFuncs;

        VkRenderPass m_renderPass;
        VkPipeline m_pipeline;
        VkPipelineLayout m_pipelineLayout;
        VkPipelineCache m_pipelineCache;

        ShaderAnalytics* m_shaderAnalytics;
        MemoryAllocator* m_allocator;
        VertexInput* m_vertexInput;
        Descriptors* m_descriptors;

        QVector<VkFramebuffer> m_frameBuffers;
        QVector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;

        PipelineConfig m_config;
        std::function<void(void)> m_creationCallback;
    };
}

#endif // VULKANRENDERER_H

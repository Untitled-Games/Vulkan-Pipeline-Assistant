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
        VulkanRenderer(QVulkanWindow* window, VulkanMain* main);

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

        bool WritePipelineCache();

        //TODO: move this to a more organized location
        bool WritePipelineConfig();
        bool ReadPipelineConfig();
        void Reload(const ReloadFlags flag);
    private:
        void CreateRenderPass();
        void CreatePipeline();
        void CreatePipelineCache();
        void CreateShaders();
        void UpdateDescriptorData();

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
    };
}

#endif // VULKANRENDERER_H

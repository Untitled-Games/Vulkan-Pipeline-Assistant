/*
 * Author: Ralph Ridley
 * Date: 20/12/19
 * Functionality for rendering through the Vulkan API.
 */

#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QVulkanWindowRenderer>

#include "pipelineconfig.h"
#include "memoryallocator.h"

namespace vpa {
    class VulkanMain;
    class ShaderAnalytics;
    class VertexInput;
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

        bool WritePipelineCache();

        //TODO: move this to a more organized location
        bool WritePipelineConfig();
        bool ReadPipelineConfig();
    private:
        void CreateRenderPass(PipelineConfig& config);
        void CreatePipeline(PipelineConfig& config);
        void CreatePipelineCache();
        void CreateVertexBuffer();

        VkShaderModule CreateShader(const QString& name);
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

        QVector<VkFramebuffer> m_frameBuffers;

        PipelineConfig m_config;
    };
}

#endif // VULKANRENDERER_H

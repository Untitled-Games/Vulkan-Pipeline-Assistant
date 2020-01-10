#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QVulkanWindowRenderer>

#include "pipelineconfig.h"
#include "memoryallocator.h"
#include "reloadflags.h"

namespace vpa {
    struct ImageInfo;
    class VulkanMain;
    class ShaderAnalytics;
    class VertexInput;
    class Descriptors;

    struct AttachmentImage {
        VkImageView view;
        Allocation allocation;
        bool isPresenting;
    };

    class VulkanRenderer : public QVulkanWindowRenderer {
    public:
        VulkanRenderer(QVulkanWindow* window, VulkanMain* main, std::function<void(void)> creationCallback, std::function<void(void)> postInitCallback);

        void initResources() override;
        void initSwapChainResources() override;
        void releaseSwapChainResources() override;
        void releaseResources() override;

        void startNextFrame() override;

        void SetValid(bool valid) { m_valid = valid; }
        PipelineConfig& GetConfig() { return m_config; }
        Descriptors* GetDescriptors() { return m_descriptors; }

        VPAError WritePipelineCache();

        VPAError WritePipelineConfig();
        VPAError ReadPipelineConfig();
        VPAError Reload(const ReloadFlags flag);

    private:
        VPAError CreateRenderPass();
        VPAError CreatePipeline();
        VPAError CreatePipelineCache();
        VPAError CreateShaders();

        VkAttachmentDescription MakeAttachment(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
            VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp, VkImageLayout initialLayout, VkImageLayout finalLayout);
        VkSubpassDescription MakeSubpass(VkPipelineBindPoint pipelineType, QVector<VkAttachmentReference>& colourReferences,
            VkAttachmentReference* depthReference, VkAttachmentReference* resolve);
        VkSubpassDependency MakeSubpassDependency(uint32_t srcIdx, uint32_t dstIdx, VkPipelineStageFlags srcStage,
            VkAccessFlags srcAccess, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess);
        VPAError MakeAttachmentImage(AttachmentImage& image, uint32_t height, uint32_t width, VkFormat format, VkImageUsageFlags usage, QString name, bool present);
        VPAError MakeFrameBuffers(QVector<VkImageView>& imageViews, uint32_t width, uint32_t height);

        bool m_initialised;
        bool m_valid;
        VulkanMain* m_main;
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
        std::function<void(void)> m_postInitCallback;

        size_t m_activeAttachment;
        bool m_useDepth;
        QVector<AttachmentImage> m_attachmentImages;
    };
}

#endif // VULKANRENDERER_H

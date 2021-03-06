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
    class ConfigValidator;

    struct AttachmentImage {
        VkImageView view;
        Allocation allocation;
        bool isPresenting;
    };

    class VulkanRenderer {
    public:
        VulkanRenderer(VulkanMain* main, std::function<void(void)> creationCallback);
        ~VulkanRenderer();

        void Init();
        void Release();
        void CleanUp();

        VPAError RenderFrame(VkCommandBuffer cmdBuffer, const uint32_t frameIdx);

        void SetValid(bool valid) { m_valid = valid; }
        PipelineConfig& GetConfig() { return m_config; }
        Descriptors* GetDescriptors() { return m_descriptors; }
        QStringList AttachmentNames() const;
        void SetActiveAttachment(uint32_t index);

        VPAError WritePipelineCache();

        VPAError WritePipelineConfig();
        VPAError ReadPipelineConfig();
        VPAError Reload(const ReloadFlags flag);

        VPAError CreateDefaultObjects();

        bool Valid() { return m_valid; }

    private:
        VPAError CreateRenderPass(VkRenderPass& renderPass, QVector<VkFramebuffer>& framebuffers, QVector<AttachmentImage>& attachmentImages, int colourAttachmentCount, bool hasDepth);
        VPAError CreatePipeline(const PipelineConfig& config, const VkVertexInputBindingDescription& bindingDescription, const QVector<VkVertexInputAttributeDescription>& attribDescriptions,
                                QVector<VkPipelineShaderStageCreateInfo>& shaderStageInfos, QVector<VkPipelineColorBlendAttachmentState> colourBlendAttachments, VkPipelineLayoutCreateInfo& layoutInfo,
                                VkRenderPass& renderPass, VkPipelineLayout& layout,
                                VkPipeline& pipeline, VkPipelineCache& cache);
        VPAError CreatePipelineCache();
        VPAError CreateShaders();

        bool DepthDrawing() const { return m_attachmentImages.size() == 1; }

        // Helper functions for making a render pass
        VkAttachmentDescription MakeAttachment(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
            VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp, VkImageLayout initialLayout, VkImageLayout finalLayout);
        VkSubpassDescription MakeSubpass(VkPipelineBindPoint pipelineType, QVector<VkAttachmentReference>& colourReferences,
            VkAttachmentReference* depthReference, VkAttachmentReference* resolve);
        VkSubpassDependency MakeSubpassDependency(uint32_t srcIdx, uint32_t dstIdx, VkPipelineStageFlags srcStage,
            VkAccessFlags srcAccess, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess);
        VPAError MakeAttachmentImage(AttachmentImage& image, uint32_t height, uint32_t width, VkFormat format, VkImageUsageFlags usage, QString name, bool present);
        VPAError MakeFrameBuffers(VkRenderPass& renderPass, QVector<VkFramebuffer>& framebuffers, QVector<VkImageView>& imageViews, uint32_t width, uint32_t height);
        VPAError MakeOutputPostPass();

        // Helper functions for making a graphics pipeline
        VkPipelineVertexInputStateCreateInfo MakeVertexInputStateCI(const VkVertexInputBindingDescription& bindingDescription, const QVector<VkVertexInputAttributeDescription>& attribDescriptions) const;
        VkPipelineInputAssemblyStateCreateInfo MakeInputAssemblyStateCI(const PipelineConfig& config) const;
        VkViewport MakeViewport() const;
        VkRect2D MakeScissor() const;
        VkPipelineViewportStateCreateInfo MakeViewportStateCI(const QVector<VkViewport>& viewports, const QVector<VkRect2D>& scissors) const;
        VkPipelineRasterizationStateCreateInfo MakeRasterizerStateCI(const PipelineConfig& config) const;
        VkPipelineMultisampleStateCreateInfo MakeMsaaCI(const PipelineConfig& config) const;
        VkPipelineDepthStencilStateCreateInfo MakeDepthStencilCI(const PipelineConfig& config) const;
        VkPipelineColorBlendAttachmentState MakeColourBlendAttachmentState(const ColourAttachmentConfig& config) const;
        VkPipelineColorBlendStateCreateInfo MakeColourBlendStateCI(const PipelineConfig& config, QVector<VkPipelineColorBlendAttachmentState>& colorBlendAttachments) const;
        VkGraphicsPipelineCreateInfo MakeGraphicsPipelineCI(const PipelineConfig& config, QVector<VkPipelineShaderStageCreateInfo>& shaderStageInfos,
                VkPipelineVertexInputStateCreateInfo& vertexInputInfo, VkPipelineInputAssemblyStateCreateInfo& inputAssembly, VkPipelineViewportStateCreateInfo& viewportState,
                VkPipelineRasterizationStateCreateInfo& rasterizer, VkPipelineMultisampleStateCreateInfo& multisampling, VkPipelineDepthStencilStateCreateInfo& depthStencil,
                VkPipelineColorBlendStateCreateInfo& colorBlending, VkPipelineLayout& layout, VkRenderPass& renderPass) const;

        bool m_initialised;
        bool m_valid;
        VulkanMain* m_main;
        QVulkanDeviceFunctions* m_deviceFuncs;

        VkRenderPass m_renderPass;
        VkPipeline m_pipeline;
        VkPipelineLayout m_pipelineLayout;
        VkPipelineCache m_pipelineCache;

        ShaderAnalytics* m_shaderAnalytics;
        MemoryAllocator* m_allocator;
        VertexInput* m_vertexInput;
        Descriptors* m_descriptors;
        ConfigValidator* m_validator;

        QVector<VkFramebuffer> m_framebuffers;
        QVector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;

        PipelineConfig m_config;
        std::function<void(void)> m_creationCallback;

        uint32_t m_activeAttachment;
        QVector<AttachmentImage> m_attachmentImages;

        VkPipeline m_outputPipeline;
        VkPipelineLayout m_outputPipelineLayout;
        QVector<VkSampler> m_outputSamplers;

        VkRenderPass m_defaultRenderPass;
        QVector<VkFramebuffer> m_defaultFramebuffers;
        AttachmentImage m_defaultDepthAttachment;
    };
}

#endif // VULKANRENDERER_H

#include "vulkanrenderer.h"

#include <QVulkanDeviceFunctions>
#include <QFile>
#include <QCoreApplication>
#include <QMatrix4x4>
#include <QMessageBox>

#include "vulkanmain.h"
#include "shaderanalytics.h"
#include "filemanager.h"
#include "vertexinput.h"
#include "descriptors.h"
#include "configvalidator.h"

namespace vpa {
    VulkanRenderer::VulkanRenderer(VulkanMain* main, std::function<void(void)> creationCallback, std::function<void(void)> postInitCallback)
        : m_initialised(false), m_valid(false), m_main(main), m_deviceFuncs(nullptr), m_renderPass(VK_NULL_HANDLE), m_pipeline(VK_NULL_HANDLE),
          m_pipelineLayout(VK_NULL_HANDLE), m_pipelineCache(VK_NULL_HANDLE), m_shaderAnalytics(nullptr), m_allocator(nullptr), m_vertexInput(nullptr),
          m_descriptors(nullptr), m_validator(nullptr), m_creationCallback(creationCallback), m_postInitCallback(postInitCallback), m_activeAttachment(0), m_useDepth(true),
          m_depthPipeline(VK_NULL_HANDLE), m_depthPipelineLayout(VK_NULL_HANDLE), m_depthSampler(VK_NULL_HANDLE) {
        m_main->m_renderer = this;
        m_config = {};
    }

    VulkanRenderer::~VulkanRenderer() {
        Release();
    }

    void VulkanRenderer::Init() {
        if (!m_initialised) {
            m_deviceFuncs = m_main->Details().deviceFunctions;
            VPAError err = VPA_OK;
            m_allocator = new MemoryAllocator(m_deviceFuncs, m_main, err);
            if (err != VPA_OK) VPA_FATAL("Device memory allocator fatal error. " + err.message);
            m_shaderAnalytics = new ShaderAnalytics(m_deviceFuncs, m_main->Device(), &m_config);
            m_validator = new ConfigValidator(m_config, m_main->Limits());
            CreateDefaultObjects();
            m_main->Reload(ReloadFlags::EverythingNoValidation);
            m_initialised = true;
        }
    }

    void VulkanRenderer::Release() {
        CleanUp();
        if (m_shaderAnalytics) delete m_shaderAnalytics;
        if (m_vertexInput) delete m_vertexInput;
        if (m_descriptors) delete m_descriptors;
        if (m_config.viewports) delete[] m_config.viewports;
        if (m_allocator) delete m_allocator;
        if (m_validator) delete m_validator;
        m_shaderAnalytics = nullptr;
        m_vertexInput = nullptr;
        m_descriptors = nullptr;
        m_config.viewports = nullptr;
        m_allocator = nullptr;
        m_validator = nullptr;
        m_valid = false;
        m_initialised = false;
    }

    void VulkanRenderer::CleanUp() {
        DESTROY_HANDLE(m_main->Device(), m_depthPipeline, m_deviceFuncs->vkDestroyPipeline);
        DESTROY_HANDLE(m_main->Device(), m_depthPipelineLayout, m_deviceFuncs->vkDestroyPipelineLayout);
        DESTROY_HANDLE(m_main->Device(), m_depthSampler, m_deviceFuncs->vkDestroySampler);
        DESTROY_HANDLE(m_main->Device(), m_pipeline, m_deviceFuncs->vkDestroyPipeline);
        DESTROY_HANDLE(m_main->Device(), m_pipelineLayout, m_deviceFuncs->vkDestroyPipelineLayout);
        DESTROY_HANDLE(m_main->Device(), m_pipelineCache, m_deviceFuncs->vkDestroyPipelineCache);
        DESTROY_HANDLE(m_main->Device(), m_renderPass, m_deviceFuncs->vkDestroyRenderPass);
        for (int i = 0; i < m_attachmentImages.size(); ++i) {
            if (!m_attachmentImages[i].isPresenting) {
                DESTROY_HANDLE(m_main->Device(), m_attachmentImages[i].view, m_deviceFuncs->vkDestroyImageView);
            }
            m_allocator->Deallocate(m_attachmentImages[i].allocation);
        }
        for (int i = 0; i < m_framebuffers.size(); ++i) {
            DESTROY_HANDLE(m_main->Device(), m_framebuffers[i], m_deviceFuncs->vkDestroyFramebuffer);
        }

        DESTROY_HANDLE(m_main->Device(), m_defaultRenderPass, m_deviceFuncs->vkDestroyRenderPass);
        DESTROY_HANDLE(m_main->Device(), m_defaultDepthAttachment.view, m_deviceFuncs->vkDestroyImageView);
        m_allocator->Deallocate(m_defaultDepthAttachment.allocation);
        for (int i = 0; i < m_defaultFramebuffers.size(); ++i) {
            DESTROY_HANDLE(m_main->Device(), m_defaultFramebuffers[i], m_deviceFuncs->vkDestroyFramebuffer);
        }
    }

    VPAError VulkanRenderer::RenderFrame(VkCommandBuffer cmdBuffer, const uint32_t frameIdx) {
        VkClearValue clearValues[2];
        clearValues[0].color = m_valid ? VkClearColorValue({{0.0f, 0.0f, 0.0f, 1.0f}}) : VkClearColorValue({{1.0f, 0.0f, 0.0f, 1.0f}}); // clear colour for each attachment
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.renderPass = m_valid ? m_renderPass : m_defaultRenderPass;
        beginInfo.framebuffer =  m_valid ? m_framebuffers[int(frameIdx)] : m_defaultFramebuffers[int(frameIdx)];
        beginInfo.renderArea.extent.width = m_main->Details().swapchainDetails.extent.width;
        beginInfo.renderArea.extent.height = m_main->Details().swapchainDetails.extent.height;
        beginInfo.clearValueCount = m_valid ? (m_useDepth ? 2 : 1) : 2;
        beginInfo.pClearValues = clearValues;

        m_deviceFuncs->vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

        if (m_valid) {
            m_descriptors->CmdPushConstants(cmdBuffer, m_pipelineLayout);
            m_descriptors->CmdBindSets(cmdBuffer, m_pipelineLayout);
            m_vertexInput->BindBuffers(cmdBuffer);
            m_deviceFuncs->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
            if (m_vertexInput->IsIndexed()) {
                m_deviceFuncs->vkCmdDrawIndexed(cmdBuffer, m_vertexInput->IndexCount(), 1, 0, 0, 0);
            }
            else {
                m_deviceFuncs->vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
            }
            m_deviceFuncs->vkCmdEndRenderPass(cmdBuffer);

            if (DepthDrawing()) {
                beginInfo.renderPass = m_defaultRenderPass;
                beginInfo.framebuffer =  m_defaultFramebuffers[int(frameIdx)];
                beginInfo.clearValueCount = 2;
                beginInfo.pClearValues = clearValues;

                VkDescriptorSet depthSet = m_descriptors->BuiltInSet(BuiltInSets::DepthPostPass);
                m_deviceFuncs->vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_depthPipelineLayout, 0, 1, &depthSet, 0, nullptr);

                m_deviceFuncs->vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
                m_deviceFuncs->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_depthPipeline);
                m_deviceFuncs->vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
                m_deviceFuncs->vkCmdEndRenderPass(cmdBuffer);
            }
        }
        else {
            m_deviceFuncs->vkCmdEndRenderPass(cmdBuffer);
        }

        return VPA_OK;
    }

    VPAError VulkanRenderer::WritePipelineCache() {
        size_t size;
        char* data = nullptr;

        VPA_VKCRITICAL_PASS(m_deviceFuncs->vkGetPipelineCacheData(m_main->Device(), m_pipelineCache, &size, data), "Failed to get pipeline cache data size");

        VPAError err = VPA_OK;
        data = new char[size];
        VPA_VKCRITICAL(m_deviceFuncs->vkGetPipelineCacheData(m_main->Device(), m_pipelineCache, &size, data), "Failed to get pipeline cache data", err);
        if (err != VPA_OK) {
            delete[] data;
            return err;
        }

        QString path = CONFIGDIR"cache.vpac";
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
             delete[] data;
             return VPA_WARN("Could not open file test.vpac");
        }
        else {
            file.write(data, qint64(size));
            file.close();
        }
        delete[] data;
        return VPA_OK;
    }

    VPAError VulkanRenderer::WritePipelineConfig() {
        return FileManager<PipelineConfig>::Writer(m_config);
    }

    VPAError VulkanRenderer::ReadPipelineConfig() {
        return FileManager<PipelineConfig>::Loader(m_config);
    }

    VPAError VulkanRenderer::Reload(const ReloadFlags flag) {
        m_deviceFuncs->vkDeviceWaitIdle(m_main->Device());

        if (flag & ReloadFlagBits::Validation) VPA_PASS_ERROR(m_validator->Validate(m_config));
        if (flag & ReloadFlagBits::Shaders) VPA_PASS_ERROR(CreateShaders());
        if (flag & ReloadFlagBits::RenderPass) VPA_PASS_ERROR(CreateRenderPass(m_renderPass, m_framebuffers, m_attachmentImages, int(m_shaderAnalytics->NumColourAttachments()), m_useDepth));
        if (flag & ReloadFlagBits::Pipeline) {
            VkPipelineLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            layoutInfo.setLayoutCount = uint32_t(m_descriptors->DescriptorSetLayouts().size());
            layoutInfo.pSetLayouts = m_descriptors->DescriptorSetLayouts().data();
            layoutInfo.pushConstantRangeCount = uint32_t(m_descriptors->PushConstantRanges().size());
            layoutInfo.pPushConstantRanges = m_descriptors->PushConstantRanges().data();

            auto bindingDescription = m_vertexInput->InputBindingDescription();
            auto attribDescriptions = m_vertexInput->InputAttribDescription();

            VPA_PASS_ERROR(CreatePipeline(m_config, bindingDescription, attribDescriptions, m_shaderStageInfos, layoutInfo, m_renderPass, m_pipelineLayout, m_pipeline, m_pipelineCache));
        }
        return VPA_OK;
    }

    VkAttachmentDescription VulkanRenderer::MakeAttachment(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
        VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp, VkImageLayout initialLayout, VkImageLayout finalLayout) {
        VkAttachmentDescription attachment = {};
        attachment.format = format;
        attachment.samples = samples;
        attachment.loadOp = loadOp;
        attachment.storeOp = storeOp;
        attachment.stencilLoadOp = stencilLoadOp;
        attachment.stencilStoreOp = stencilStoreOp;
        attachment.initialLayout = initialLayout;
        attachment.finalLayout = finalLayout;
        return attachment;
    }

    VkSubpassDescription VulkanRenderer::MakeSubpass(VkPipelineBindPoint pipelineType, QVector<VkAttachmentReference>& colourReferences, VkAttachmentReference* depthReference, VkAttachmentReference* resolve) {
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = pipelineType;
        subpass.colorAttachmentCount = uint32_t(colourReferences.size());
        subpass.pColorAttachments = colourReferences.data();
        subpass.pDepthStencilAttachment = depthReference;
        subpass.pResolveAttachments = resolve;
        return subpass;
    }

    VkSubpassDependency VulkanRenderer::MakeSubpassDependency(uint32_t srcIdx, uint32_t dstIdx, VkPipelineStageFlags srcStage,
        VkAccessFlags srcAccess, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess) {
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = srcIdx;
        dependency.dstSubpass = dstIdx;
        dependency.srcStageMask = srcStage;
        dependency.srcAccessMask = srcAccess;
        dependency.dstStageMask = dstStage;
        dependency.dstAccessMask = dstAccess;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        return dependency;
    }

     VPAError VulkanRenderer::MakeAttachmentImage(AttachmentImage& image, uint32_t height, uint32_t width, VkFormat format, VkImageUsageFlags usage, QString name, bool present) {
        image.isPresenting = present;

        VkImageCreateInfo createInfo = {
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            nullptr, 0,
            VK_IMAGE_TYPE_2D, format,
            { width, height, 1 }, 1, 1,
            VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
            usage,
            VK_SHARING_MODE_EXCLUSIVE,
            0, nullptr,
            VK_IMAGE_LAYOUT_UNDEFINED
        };

        size_t size = width * height * 4;
        VPA_PASS_ERROR(m_allocator->Allocate(size, createInfo, name, image.allocation));

        if (!present) {
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image.allocation.image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = createInfo.format;
            viewInfo.subresourceRange.aspectMask = usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = createInfo.mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = createInfo.arrayLayers;

            VPAError err = VPA_OK;
            VPA_VKCRITICAL(m_deviceFuncs->vkCreateImageView(m_main->Device(), &viewInfo, nullptr, &image.view),
                             qPrintable("create attachment image view for allocation '" + name + "'"), err);
            if (err != VPA_OK) {
                DESTROY_HANDLE(m_main->Device(), image.view, m_deviceFuncs->vkDestroyImageView);
                m_allocator->Deallocate(image.allocation);
                return err;
            }
        }
        else {
            image.view = VK_NULL_HANDLE; // Causes injection of correct present image in framebuffer creation
        }

        return VPA_OK;
    }

    VPAError VulkanRenderer::MakeFrameBuffers(VkRenderPass& renderPass, QVector<VkFramebuffer>& framebuffers, QVector<VkImageView>& imageViews, uint32_t width, uint32_t height) {
        for (int i = 0; i < framebuffers.size(); ++i) {
            DESTROY_HANDLE(m_main->Device(), framebuffers[i], m_deviceFuncs->vkDestroyFramebuffer);
        }

        int presentIdx = -1;
        for (int i = 0; i < imageViews.size(); ++i) {
            if (imageViews[i] == VK_NULL_HANDLE) {
                presentIdx = i;
                break;
            }
        }

        framebuffers.clear();
        framebuffers.resize(int(m_main->Details().swapchainDetails.imageCount));
        for (int i = 0; i < framebuffers.size(); ++i) {
            if (presentIdx >= 0) imageViews[presentIdx] = m_main->Details().swapchainDetails.imageViews[i];
            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = uint32_t(imageViews.size());
            framebufferInfo.pAttachments = imageViews.data();
            framebufferInfo.width = width;
            framebufferInfo.height = height;
            framebufferInfo.layers = 1;

            VPA_VKCRITICAL_PASS(m_deviceFuncs->vkCreateFramebuffer(m_main->Device(), &framebufferInfo, nullptr, &framebuffers[i]), "Failed to create framebuffer");
        }
        return VPA_OK;
    }

    VPAError VulkanRenderer::CreateRenderPass(VkRenderPass& renderPass, QVector<VkFramebuffer>& framebuffers, QVector<AttachmentImage>& attachmentImages, int colourAttachmentCount, bool hasDepth) {
        DESTROY_HANDLE(m_main->Device(), renderPass, m_deviceFuncs->vkDestroyRenderPass);
        for (int i = 0; i < attachmentImages.size(); ++i) {
            if (!attachmentImages[i].isPresenting) {
                DESTROY_HANDLE(m_main->Device(), attachmentImages[i].view, m_deviceFuncs->vkDestroyImageView);
            }
            m_allocator->Deallocate(attachmentImages[i].allocation);
        }
        attachmentImages.clear();

        uint32_t width = m_main->Details().swapchainDetails.extent.width;
        uint32_t height = m_main->Details().swapchainDetails.extent.height;
        QVector<VkAttachmentDescription> attachments;
        QVector<VkSubpassDescription> subpasses;
        QVector<VkSubpassDependency> dependencies;
        QVector<VkAttachmentReference> colourAttachmentRefs = QVector<VkAttachmentReference>(int(colourAttachmentCount));
        VkAttachmentReference depthAttachmentRef = {};
        QVector<VkImageView> attachmentImageViews = QVector<VkImageView>(int(colourAttachmentCount) + (hasDepth ? 1 : 0));

        attachmentImages.resize(attachmentImageViews.size());
        for (int i = 0; i < colourAttachmentCount; ++i) {
            bool present = i == m_activeAttachment;
            VkFormat colourFormat = m_main->Details().swapchainDetails.surfaceFormat.format;
            attachments.push_back(MakeAttachment(colourFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
            if (present) attachments[i].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference positionsRef = {};
            positionsRef.attachment = uint32_t(i);
            positionsRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colourAttachmentRefs[i] = positionsRef;

            VPA_PASS_ERROR(MakeAttachmentImage(attachmentImages[i], height, width,
                    colourFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, "colour attachment " + QString::number(i), present));
            attachmentImageViews[i] = attachmentImages[i].view;
        }

        if (hasDepth) {
            int index = attachmentImages.size() - 1;
            bool present = index == m_activeAttachment;
            VkFormat depthFormat = m_main->Details().swapchainDetails.depthFormat;
            attachments.push_back(MakeAttachment(depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
            if (present) attachments[index].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            depthAttachmentRef.attachment = uint32_t(colourAttachmentCount);
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            VPA_PASS_ERROR(MakeAttachmentImage(attachmentImages[index], height, width, depthFormat,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, "depth attachment", false));
            attachmentImageViews[index] = attachmentImages[index].view;

            if (present) VPA_PASS_ERROR(MakeDepthPresentPostPass(attachmentImageViews[index]));
        }
        else if (m_config.writables.depthWriteEnable) {
            return VPA_WARN("Depth write is enabled but there is no depth buffer.");
        }

        subpasses.push_back(MakeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, &depthAttachmentRef, nullptr));

        dependencies.push_back(MakeSubpassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));
        dependencies.push_back(MakeSubpassDependency(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT));

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = uint32_t(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = uint32_t(subpasses.size());
        renderPassInfo.pSubpasses = subpasses.data();
        renderPassInfo.dependencyCount = uint32_t(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        VPA_VKCRITICAL_PASS(m_deviceFuncs->vkCreateRenderPass(m_main->Device(), &renderPassInfo, nullptr, &renderPass), "Failed to create render pass");

        VPA_PASS_ERROR(MakeFrameBuffers(renderPass, framebuffers, attachmentImageViews, width, height));

        return VPA_OK;
    }

    VPAError VulkanRenderer::CreatePipeline(const PipelineConfig& config, const VkVertexInputBindingDescription& bindingDescription,
            const QVector<VkVertexInputAttributeDescription>& attribDescriptions, QVector<VkPipelineShaderStageCreateInfo>& shaderStageInfos,
            VkPipelineLayoutCreateInfo& layoutInfo, VkRenderPass& renderPass, VkPipelineLayout& layout, VkPipeline& pipeline, VkPipelineCache& cache) {
        DESTROY_HANDLE(m_main->Device(), pipeline, m_deviceFuncs->vkDestroyPipeline);
        DESTROY_HANDLE(m_main->Device(), layout, m_deviceFuncs->vkDestroyPipelineLayout);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = MakeVertexInputStateCI(bindingDescription, attribDescriptions);
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = MakeInputAssemblyStateCI(config);
        QVector<VkViewport> viewports = { MakeViewport() };

        if (m_config.viewports) delete[] m_config.viewports;
        m_config.viewports = new VkViewport[1];
        m_config.viewports[0] = viewports[0];
        m_config.viewportCount = 1;

        QVector<VkRect2D> scissors = { MakeScissor() };
        VkPipelineViewportStateCreateInfo viewportState = MakeViewportStateCI(viewports, scissors);
        VkPipelineRasterizationStateCreateInfo rasterizer = MakeRasterizerStateCI(config);
        VkPipelineMultisampleStateCreateInfo multisampling = MakeMsaaCI(config);
        VkPipelineDepthStencilStateCreateInfo depthStencil = MakeDepthStencilCI(config);

        QVector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
        colorBlendAttachments.push_back(MakeColourBlendAttachmentState(config.writables.attachments));
        VkPipelineColorBlendStateCreateInfo colourBlending = MakeColourBlendStateCI(config, colorBlendAttachments);

        VPA_VKCRITICAL_PASS(m_deviceFuncs->vkCreatePipelineLayout(m_main->Device(), &layoutInfo, nullptr, &layout), "Failed to create pipeline layout");

        VkGraphicsPipelineCreateInfo pipelineInfo = MakeGraphicsPipelineCI(config, shaderStageInfos, vertexInputInfo, inputAssembly, viewportState, rasterizer, multisampling, depthStencil, colourBlending, layout, renderPass);

        if (cache == VK_NULL_HANDLE) {
            VkPipelineCacheCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            createInfo.initialDataSize = 0;
            m_deviceFuncs->vkCreatePipelineCache(m_main->Device(), &createInfo, nullptr, &cache);
        }

        VPA_VKCRITICAL_PASS(m_deviceFuncs->vkCreateGraphicsPipelines(m_main->Device(), cache, 1, &pipelineInfo, nullptr, &pipeline), "Failed to create pipeline");

        return VPA_OK;
    }

    VPAError VulkanRenderer::CreateShaders() {
        m_shaderStageInfos.clear();
        VPA_PASS_ERROR(m_shaderAnalytics->LoadShaders(m_config.vertShader, m_config.fragShader));//, "/../shaders/tesc_test.spv", "/../shaders/tese_test.spv", "/../shaders/gs_test.spv");
        VkPipelineShaderStageCreateInfo shaderCreateInfo;
        if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::Vertex, shaderCreateInfo)) m_shaderStageInfos.push_back(shaderCreateInfo);
        if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::Fragment, shaderCreateInfo)) m_shaderStageInfos.push_back(shaderCreateInfo);
        if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::TessellationControl, shaderCreateInfo)) m_shaderStageInfos.push_back(shaderCreateInfo);
        if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::TessellationEvaluation, shaderCreateInfo)) m_shaderStageInfos.push_back(shaderCreateInfo);
        if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::Geometry, shaderCreateInfo)) m_shaderStageInfos.push_back(shaderCreateInfo);

        if (m_vertexInput) delete m_vertexInput;
        VPAError err = VPA_OK;
        m_vertexInput = new VertexInput(m_deviceFuncs, m_allocator, m_shaderAnalytics->InputAttributes(), MESHDIR"Teapot", true, err);
        if (err != VPA_OK) {
            delete m_vertexInput;
            m_vertexInput = nullptr;
            return err;
        }

        if (m_descriptors) delete m_descriptors;
        m_descriptors = new Descriptors(m_main, m_deviceFuncs, m_allocator, m_shaderAnalytics->DescriptorLayoutMap(), m_shaderAnalytics->PushConstantRanges(), m_main->Limits(), err);
        if (err != VPA_OK) {
            delete m_descriptors;
            return err;
        }

        m_creationCallback();
        return VPA_OK;
    }

    VPAError VulkanRenderer::CreateDefaultObjects() {
        uint32_t width = m_main->Details().swapchainDetails.extent.width;
        uint32_t height = m_main->Details().swapchainDetails.extent.height;
        QVector<VkAttachmentDescription> attachments(2);
        QVector<VkSubpassDescription> subpasses(1);
        QVector<VkSubpassDependency> dependencies(2);
        QVector<VkAttachmentReference> colourAttachmentRefs(1);
        VkAttachmentReference depthAttachmentRef = {};
        QVector<VkImageView> attachmentImageViews(2);
        VPA_PASS_ERROR(MakeAttachmentImage(m_defaultDepthAttachment, height, width, m_main->Details().swapchainDetails.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, "Default depth stencil attachment", false));
        attachmentImageViews[1] = m_defaultDepthAttachment.view;

        attachments[0] = MakeAttachment(m_main->Details().swapchainDetails.surfaceFormat.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        attachments[1] = MakeAttachment(m_main->Details().swapchainDetails.depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        VkAttachmentReference colourRef = {};
        colourRef.attachment = 0;
        colourRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colourAttachmentRefs[0] = colourRef;

        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpasses[0] = MakeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, &depthAttachmentRef, nullptr);

        dependencies[0] = MakeSubpassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        dependencies[1] = MakeSubpassDependency(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = uint32_t(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = uint32_t(subpasses.size());
        renderPassInfo.pSubpasses = subpasses.data();
        renderPassInfo.dependencyCount = uint32_t(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        VPA_VKCRITICAL_PASS(m_deviceFuncs->vkCreateRenderPass(m_main->Device(), &renderPassInfo, nullptr, &m_defaultRenderPass), "Failed to create render pass");

        VPA_PASS_ERROR(MakeFrameBuffers(m_defaultRenderPass, m_defaultFramebuffers, attachmentImageViews, width, height));

        return VPA_OK;
    }

    VPAError VulkanRenderer::MakeDepthPresentPostPass(VkImageView& imageView) {
        VkPipelineLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = m_descriptors->BuiltInSetLayout(BuiltInSets::DepthPostPass);
        layoutInfo.pushConstantRangeCount = 0;
        layoutInfo.pPushConstantRanges = nullptr;

        QByteArray blob;
        VkShaderModule vertModule;
        VkShaderModule fragModule;
        VPA_PASS_ERROR(m_shaderAnalytics->CreateModule(vertModule, SHADERDIR"fullscreen.spv", &blob));
        blob.clear();
        VPAError err = m_shaderAnalytics->CreateModule(fragModule, SHADERDIR"depth_frag.spv", &blob);
        if (err != VPA_OK) {
            DESTROY_HANDLE(m_main->Device(), vertModule, m_deviceFuncs->vkDestroyShaderModule);
            return err;
        }

        QVector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
        shaderStageInfos.push_back({ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                                     VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main", nullptr });
        shaderStageInfos.push_back({ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                                     VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main", nullptr });

        PipelineConfig config = {};
        config.writables.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        config.writables.cullMode = VK_CULL_MODE_BACK_BIT;
        config.writables.frontFace = VK_FRONT_FACE_CLOCKWISE;
        config.writables.depthTestEnable = VK_FALSE;
        config.writables.depthWriteEnable = VK_FALSE;

        VkRenderPass pass = m_defaultRenderPass;
        VkPipelineCache cache = VK_NULL_HANDLE;
        err = CreatePipeline(config, {}, {}, shaderStageInfos, layoutInfo, pass, m_depthPipelineLayout, m_depthPipeline, cache);
        if (err != VPA_OK) {
            DESTROY_HANDLE(m_main->Device(), vertModule, m_deviceFuncs->vkDestroyShaderModule);
            DESTROY_HANDLE(m_main->Device(), fragModule, m_deviceFuncs->vkDestroyShaderModule);
            return err;
        }

        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 0.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 1.0f;

        DESTROY_HANDLE(m_main->Device(), m_depthSampler, m_deviceFuncs->vkDestroySampler);

        VPA_VKCRITICAL(m_deviceFuncs->vkCreateSampler(m_main->Device(), &samplerInfo, nullptr, &m_depthSampler), "create sampler for depth post pass", err);
        if (err != VPA_OK) {
            DESTROY_HANDLE(m_main->Device(), vertModule, m_deviceFuncs->vkDestroyShaderModule);
            DESTROY_HANDLE(m_main->Device(), fragModule, m_deviceFuncs->vkDestroyShaderModule);
            DESTROY_HANDLE(m_main->Device(), m_depthSampler, m_deviceFuncs->vkDestroySampler);
            return err;
        }

        VkDescriptorImageInfo imageInfo = {};
        imageInfo = {};
        imageInfo.imageView = imageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler = m_depthSampler;

        VkWriteDescriptorSet writeSet = {};
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = m_descriptors->BuiltInSet(BuiltInSets::DepthPostPass);
        writeSet.dstBinding = 0;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.descriptorCount = 1;
        writeSet.pImageInfo = &imageInfo;

        m_deviceFuncs->vkUpdateDescriptorSets(m_main->Device(), 1, &writeSet, 0, nullptr);

        DESTROY_HANDLE(m_main->Device(), vertModule, m_deviceFuncs->vkDestroyShaderModule);
        DESTROY_HANDLE(m_main->Device(), fragModule, m_deviceFuncs->vkDestroyShaderModule);
        return VPA_OK;
    }

    VkPipelineVertexInputStateCreateInfo VulkanRenderer::MakeVertexInputStateCI(const VkVertexInputBindingDescription& bindingDescription,
            const QVector<VkVertexInputAttributeDescription>& attribDescriptions) const {
        VkPipelineVertexInputStateCreateInfo vertexInput = {};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.vertexAttributeDescriptionCount = uint32_t(attribDescriptions.size());
        vertexInput.pVertexBindingDescriptions = &bindingDescription;
        vertexInput.pVertexAttributeDescriptions = attribDescriptions.data();
        return vertexInput;
    }

    VkPipelineInputAssemblyStateCreateInfo VulkanRenderer::MakeInputAssemblyStateCI(const PipelineConfig& config) const {
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = config.writables.topology;
        inputAssembly.primitiveRestartEnable = config.writables.primitiveRestartEnable;
        return inputAssembly;
    }

    VkViewport VulkanRenderer::MakeViewport() const {
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = float(m_main->Details().swapchainDetails.extent.width);
        viewport.height = float(m_main->Details().swapchainDetails.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        return viewport;
    }

    VkRect2D VulkanRenderer::MakeScissor() const {
        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = m_main->Details().swapchainDetails.extent;
        return scissor;
    }

    VkPipelineViewportStateCreateInfo VulkanRenderer::MakeViewportStateCI(const QVector<VkViewport>& viewports, const QVector<VkRect2D>& scissors) const {
        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = uint32_t(viewports.size());
        viewportState.pViewports = viewports.data();
        viewportState.scissorCount = uint32_t(scissors.size());
        viewportState.pScissors = scissors.data();
        return viewportState;
    }

    VkPipelineRasterizationStateCreateInfo VulkanRenderer::MakeRasterizerStateCI(const PipelineConfig& config) const {
        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = config.writables.depthClampEnable;
        rasterizer.rasterizerDiscardEnable = config.writables.rasterizerDiscardEnable;
        rasterizer.polygonMode = config.writables.polygonMode;
        rasterizer.lineWidth = config.writables.lineWidth;
        rasterizer.cullMode = config.writables.cullMode;
        rasterizer.frontFace = config.writables.frontFace;
        rasterizer.depthBiasEnable = config.writables.depthBiasEnable;
        return rasterizer;
    }
    VkPipelineMultisampleStateCreateInfo VulkanRenderer::MakeMsaaCI(const PipelineConfig& config) const {
        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = config.writables.msaaSamples != VK_SAMPLE_COUNT_1_BIT;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; //TODO change this back if multisampling is working
        //multisampling.rasterizationSamples = m_config.writablePipelineConfig.msaaSamples;
        multisampling.minSampleShading = config.writables.minSampleShading;
        return multisampling;
    }

    VkPipelineDepthStencilStateCreateInfo VulkanRenderer::MakeDepthStencilCI(const PipelineConfig& config) const {
        VkPipelineDepthStencilStateCreateInfo depthStencil = {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = config.writables.depthTestEnable;
        depthStencil.depthWriteEnable = config.writables.depthWriteEnable;
        depthStencil.depthCompareOp = config.writables.depthCompareOp;
        depthStencil.depthBoundsTestEnable = config.writables.depthBoundsTest;
        depthStencil.stencilTestEnable = config.writables.stencilTestEnable;
        return depthStencil;
    }

    VkPipelineColorBlendAttachmentState VulkanRenderer::MakeColourBlendAttachmentState(const ColourAttachmentConfig& config) const {
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = config.blendEnable;
        colorBlendAttachment.alphaBlendOp = config.alphaBlendOp;
        colorBlendAttachment.srcAlphaBlendFactor = config.srcAlphaBlendFactor;
        colorBlendAttachment.dstAlphaBlendFactor = config.dstAlphaBlendFactor;
        colorBlendAttachment.colorBlendOp = config.colourBlendOp;
        colorBlendAttachment.srcColorBlendFactor = config.srcColourBlendFactor;
        colorBlendAttachment.dstColorBlendFactor = config.dstColourBlendFactor;
        return colorBlendAttachment;
    }

    VkPipelineColorBlendStateCreateInfo VulkanRenderer::MakeColourBlendStateCI(const PipelineConfig& config, QVector<VkPipelineColorBlendAttachmentState>& colorBlendAttachments) const {
        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = config.writables.logicOpEnable;
        colorBlending.logicOp = config.writables.logicOp;
        colorBlending.attachmentCount = uint32_t(colorBlendAttachments.size());
        colorBlending.pAttachments = colorBlendAttachments.data();
        memcpy(colorBlending.blendConstants, config.writables.blendConstants, 4 * sizeof(float));
        return colorBlending;
    }

    VkGraphicsPipelineCreateInfo VulkanRenderer::MakeGraphicsPipelineCI(const PipelineConfig& config, QVector<VkPipelineShaderStageCreateInfo>& shaderStageInfos,
            VkPipelineVertexInputStateCreateInfo& vertexInputInfo, VkPipelineInputAssemblyStateCreateInfo& inputAssembly, VkPipelineViewportStateCreateInfo& viewportState,
            VkPipelineRasterizationStateCreateInfo& rasterizer, VkPipelineMultisampleStateCreateInfo& multisampling, VkPipelineDepthStencilStateCreateInfo& depthStencil,
            VkPipelineColorBlendStateCreateInfo& colourBlending, VkPipelineLayout& layout, VkRenderPass& renderPass) const {
        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = uint32_t(shaderStageInfos.size());
        pipelineInfo.pStages = shaderStageInfos.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colourBlending;
        pipelineInfo.layout = layout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = config.subpassIdx;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        return pipelineInfo;
    }
}

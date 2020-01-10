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

namespace vpa {
    VulkanRenderer::VulkanRenderer(QVulkanWindow* window, VulkanMain* main, std::function<void(void)> creationCallback, std::function<void(void)> postInitCallback)
        : m_initialised(false), m_valid(false), m_main(main), m_window(window), m_deviceFuncs(nullptr), m_renderPass(VK_NULL_HANDLE), m_pipeline(VK_NULL_HANDLE),
          m_pipelineLayout(VK_NULL_HANDLE), m_pipelineCache(VK_NULL_HANDLE), m_shaderAnalytics(nullptr), m_allocator(nullptr), m_vertexInput(nullptr),
          m_descriptors(nullptr), m_creationCallback(creationCallback), m_postInitCallback(postInitCallback), m_activeAttachment(0), m_useDepth(true) {
        m_main->m_renderer = this;
        m_config = {};
    }

    void VulkanRenderer::initResources() {
        if (!m_initialised) {
            m_postInitCallback();
            m_deviceFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
            VPAError err = VPA_OK;
            m_allocator = new MemoryAllocator(m_deviceFuncs, m_window, err);
            if (err.level != VPAErrorLevel::Ok) VPA_FATAL("Device memory allocator fatal error. " + err.message)
            m_shaderAnalytics = new ShaderAnalytics(m_deviceFuncs, m_window->device(), &m_config);
        }
    }

    void VulkanRenderer::initSwapChainResources() {
        if (!m_initialised) {
            m_main->Reload(ReloadFlags::Everything);
            m_initialised = true;
        }
        QVector<VkImageView> imageViews;
        for (int i = 0; i < m_attachmentImages.size(); ++i) {
            if (size_t(i) == m_activeAttachment) m_attachmentImages[i].view = m_window->swapChainImageView(0);
            imageViews.push_back(m_attachmentImages[i].view);
        }
        MakeFrameBuffers(imageViews, uint32_t(m_window->swapChainImageSize().width()), uint32_t(m_window->swapChainImageSize().height()));
    }

    void VulkanRenderer::releaseSwapChainResources() { }

    void VulkanRenderer::releaseResources() {
        DESTROY_HANDLE(m_window->device(), m_pipeline, m_deviceFuncs->vkDestroyPipeline)
        DESTROY_HANDLE(m_window->device(), m_pipelineLayout, m_deviceFuncs->vkDestroyPipelineLayout)
        DESTROY_HANDLE(m_window->device(), m_pipelineCache, m_deviceFuncs->vkDestroyPipelineCache)
        DESTROY_HANDLE(m_window->device(), m_renderPass, m_deviceFuncs->vkDestroyRenderPass)
        for (int i = 0; i < m_frameBuffers.size(); ++i) {
            DESTROY_HANDLE(m_window->device(), m_frameBuffers[i], m_deviceFuncs->vkDestroyFramebuffer)
        }
        delete m_shaderAnalytics;
        if (m_vertexInput) delete m_vertexInput;
        if (m_descriptors) delete m_descriptors;
        if (m_config.viewports) delete[] m_config.viewports;
        delete m_allocator;
    }

    void VulkanRenderer::startNextFrame() {
        VkClearValue clearValues[2];
        clearValues[0].color = m_valid ? VkClearColorValue({{0.0f, 0.0f, 0.0f, 1.0f}}) : VkClearColorValue({{1.0f, 0.0f, 0.0f, 1.0f}});
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.renderPass = m_valid ? m_renderPass : m_window->defaultRenderPass();
        beginInfo.framebuffer =  m_valid ? m_frameBuffers[m_window->currentFrame()] : m_window->currentFramebuffer();
        const QSize imageSize = m_window->swapChainImageSize();
        beginInfo.renderArea.extent.width = uint32_t(imageSize.width());
        beginInfo.renderArea.extent.height = uint32_t(imageSize.height());
        beginInfo.clearValueCount = 2;
        beginInfo.pClearValues = clearValues;

        VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
        m_deviceFuncs->vkCmdBeginRenderPass(cmdBuf, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

        if (m_valid) {
            m_descriptors->CmdPushConstants(cmdBuf, m_pipelineLayout);
            m_descriptors->CmdBindSets(cmdBuf, m_pipelineLayout);
            m_vertexInput->BindBuffers(cmdBuf);
            m_deviceFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
            if (m_vertexInput->IsIndexed()) {
                m_deviceFuncs->vkCmdDrawIndexed(cmdBuf, m_vertexInput->IndexCount(), 1, 0, 0, 0);
            }
            else {
                m_deviceFuncs->vkCmdDraw(cmdBuf, 4, 1, 0, 0);
            }
        }

        m_deviceFuncs->vkCmdEndRenderPass(cmdBuf);
        if (m_useDepth && m_activeAttachment == size_t(m_attachmentImages.size()) - 1) {
            // TODO depth to linear image post-pass
        }

        m_window->frameReady();
        m_window->requestUpdate();
    }

    VPAError VulkanRenderer::WritePipelineCache() {
        size_t size;
        char* data = nullptr;

        VPA_VKCRITICAL_PASS(m_deviceFuncs->vkGetPipelineCacheData(m_window->device(), m_pipelineCache, &size, data), "Failed to get pipeline cache data size")

        VPAError err = VPA_OK;
        data = new char[size];
        VPA_VKCRITICAL(m_deviceFuncs->vkGetPipelineCacheData(m_window->device(), m_pipelineCache, &size, data), "Failed to get pipeline cache data", err)
        if (err.level != VPAErrorLevel::Ok) {
            delete[] data;
            return err;
        }

        QString path = QCoreApplication::applicationDirPath() + "/test.vpac";
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
        m_deviceFuncs->vkDeviceWaitIdle(m_window->device());
        if (flag & ReloadFlagBits::Shaders) { VPA_PASS_ERROR(CreateShaders()); }
        if (flag & ReloadFlagBits::RenderPass) { VPA_PASS_ERROR(CreateRenderPass()); }
        if (flag & ReloadFlagBits::Pipeline) { VPA_PASS_ERROR(CreatePipeline()); }
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
            viewInfo.subresourceRange.aspectMask = usage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = createInfo.mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = createInfo.arrayLayers;

            VPAError err = VPA_OK;
            VPA_VKCRITICAL(m_deviceFuncs->vkCreateImageView(m_window->device(), &viewInfo, nullptr, &image.view),
                             qPrintable("create attachment image view for allocation '" + name + "'"), err)
            if (err.level != VPAErrorLevel::Ok) {
                DESTROY_HANDLE(m_window->device(), image.view, m_deviceFuncs->vkDestroyImageView)
                m_allocator->Deallocate(image.allocation);
                return err;
            }
        }
        else {
            image.view = m_window->swapChainImageView(0);
        }

        return VPA_OK;
    }

    VPAError VulkanRenderer::MakeFrameBuffers(QVector<VkImageView>& imageViews, uint32_t width, uint32_t height) {
        for (int i = 0; i < m_frameBuffers.size(); ++i) {
            DESTROY_HANDLE(m_window->device(), m_frameBuffers[i], m_deviceFuncs->vkDestroyFramebuffer)
        }
        m_frameBuffers.clear();
        m_frameBuffers.resize(m_window->swapChainImageCount());
        for (int i = 0; i < m_attachmentImages.size(); ++i) {
            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = uint32_t(imageViews.size());
            framebufferInfo.pAttachments = imageViews.data();
            framebufferInfo.width = width;
            framebufferInfo.height =  height;
            framebufferInfo.layers = 1;

            VPA_VKCRITICAL_PASS(m_deviceFuncs->vkCreateFramebuffer(m_window->device(), &framebufferInfo, nullptr, &m_frameBuffers[i]), "Failed to create framebuffer")
        }
        return VPA_OK;
    }

    VPAError VulkanRenderer::CreateRenderPass() {
        DESTROY_HANDLE(m_window->device(), m_renderPass, m_deviceFuncs->vkDestroyRenderPass)
        for (int i = 0; i < m_attachmentImages.size(); ++i) {
            if (!m_attachmentImages[i].isPresenting) {
                DESTROY_HANDLE(m_window->device(), m_attachmentImages[i].view, m_deviceFuncs->vkDestroyImageView)
            }
            m_allocator->Deallocate(m_attachmentImages[i].allocation);
        }
        m_attachmentImages.clear();

        size_t attachmentCount = m_shaderAnalytics->NumColourAttachments();
        uint32_t width = uint32_t(m_window->swapChainImageSize().width());
        uint32_t height = uint32_t(m_window->swapChainImageSize().height());
        QVector<VkAttachmentDescription> attachments;
        QVector<VkSubpassDescription> subpasses;
        QVector<VkSubpassDependency> dependencies;
        QVector<VkAttachmentReference> colourAttachmentRefs = QVector<VkAttachmentReference>(int(attachmentCount));
        VkAttachmentReference depthAttachmentRef = {};
        QVector<VkImageView> attachmentImageViews = QVector<VkImageView>(int(attachmentCount) + (m_useDepth ? 1 : 0));

        m_attachmentImages.resize(attachmentImageViews.size());
        for (size_t i = 0; i < attachmentCount; ++i) {
            bool present = i == m_activeAttachment;
            attachments.push_back(MakeAttachment(m_window->colorFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
            if (present) attachments[int(i)].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference positionsRef = {};
            positionsRef.attachment = uint32_t(i);
            positionsRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colourAttachmentRefs[int(i)] = positionsRef;

            VPA_PASS_ERROR(MakeAttachmentImage(m_attachmentImages[int(i)], height, width,
                    m_window->colorFormat(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, "colour attachment " + QString::number(i), present));
            attachmentImageViews[int(i)] = m_attachmentImages[int(i)].view;
        }

        if (m_useDepth) {
            int index = m_attachmentImages.size() - 1;
            bool present = (size_t(index) == m_activeAttachment);
            attachments.push_back(MakeAttachment(m_window->depthStencilFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
            if (present) attachments[int(index)].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // TODO setup linear depth post pass

            depthAttachmentRef.attachment = uint32_t(attachmentCount);
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            VPA_PASS_ERROR(MakeAttachmentImage(m_attachmentImages[index], height, width,
                    m_window->depthStencilFormat(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, "depth attachment ", present));
            attachmentImageViews[index] = m_attachmentImages[index].view;
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

        VPA_VKCRITICAL_PASS(m_deviceFuncs->vkCreateRenderPass(m_window->device(), &renderPassInfo, nullptr, &m_renderPass), "Failed to create render pass")

        VPA_PASS_ERROR(MakeFrameBuffers(attachmentImageViews, width, height));

        return VPA_OK;
    }

    VPAError VulkanRenderer::CreatePipeline() {
        DESTROY_HANDLE(m_window->device(), m_pipeline, m_deviceFuncs->vkDestroyPipeline)
        DESTROY_HANDLE(m_window->device(), m_pipelineLayout, m_deviceFuncs->vkDestroyPipelineLayout)

        VkPipelineLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = uint32_t(m_descriptors->DescriptorSetLayouts().size());
        layoutInfo.pSetLayouts = m_descriptors->DescriptorSetLayouts().data();
        layoutInfo.pushConstantRangeCount = uint32_t(m_descriptors->PushConstantRanges().size());
        layoutInfo.pPushConstantRanges = m_descriptors->PushConstantRanges().data();

        auto bindingDesc = m_vertexInput->InputBindingDescription();
        auto attribDescs = m_vertexInput->InputAttribDescription();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = uint32_t(attribDescs.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
        vertexInputInfo.pVertexAttributeDescriptions = attribDescs.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = m_config.writables.topology;
        inputAssembly.primitiveRestartEnable = m_config.writables.primitiveRestartEnable;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = float(m_window->swapChainImageSize().width());
        viewport.height = float(m_window->swapChainImageSize().height());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        if (m_config.viewports) delete[] m_config.viewports;
        m_config.viewports = new VkViewport[1];
        m_config.viewports[0] = viewport;
        m_config.viewportCount = 1;

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = { uint32_t(m_window->swapChainImageSize().width()), uint32_t(m_window->swapChainImageSize().height()) };

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = m_config.writables.depthClampEnable;
        rasterizer.rasterizerDiscardEnable = m_config.writables.rasterizerDiscardEnable;
        rasterizer.polygonMode = m_config.writables.polygonMode;
        rasterizer.lineWidth = m_config.writables.lineWidth;
        rasterizer.cullMode = m_config.writables.cullMode;
        rasterizer.frontFace = m_config.writables.frontFace;
        rasterizer.depthBiasEnable = m_config.writables.depthBiasEnable;

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = m_config.writables.msaaSamples != VK_SAMPLE_COUNT_1_BIT;
        //TODO change this back if multisampling is working
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        //multisampling.rasterizationSamples = m_config.writablePipelineConfig.msaaSamples;
        multisampling.minSampleShading = m_config.writables.minSampleShading;

        VkPipelineDepthStencilStateCreateInfo depthStencil = {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = m_config.writables.depthTestEnable;
        depthStencil.depthWriteEnable = m_config.writables.depthWriteEnable;
        depthStencil.depthCompareOp = m_config.writables.depthCompareOp;
        depthStencil.depthBoundsTestEnable = m_config.writables.depthBoundsTest;
        depthStencil.stencilTestEnable = m_config.writables.stencilTestEnable;

        QVector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
        for (size_t i = 0; i < 1; ++i) {
            VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = m_config.writables.attachments.blendEnable;
            colorBlendAttachment.alphaBlendOp = m_config.writables.attachments.alphaBlendOp;
            colorBlendAttachment.srcAlphaBlendFactor = m_config.writables.attachments.srcAlphaBlendFactor;
            colorBlendAttachment.dstAlphaBlendFactor = m_config.writables.attachments.dstAlphaBlendFactor;
            colorBlendAttachment.colorBlendOp = m_config.writables.attachments.colourBlendOp;
            colorBlendAttachment.srcColorBlendFactor = m_config.writables.attachments.srcColourBlendFactor;
            colorBlendAttachment.dstColorBlendFactor = m_config.writables.attachments.dstColourBlendFactor;
            colorBlendAttachments.push_back(colorBlendAttachment);
        }

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = m_config.writables.logicOpEnable;
        colorBlending.logicOp = m_config.writables.logicOp;
        colorBlending.attachmentCount = uint32_t(colorBlendAttachments.size());
        colorBlending.pAttachments = colorBlendAttachments.data();
        memcpy(colorBlending.blendConstants, m_config.writables.blendConstants, 4 * sizeof(float));

        VPA_VKCRITICAL_PASS(m_deviceFuncs->vkCreatePipelineLayout(m_window->device(), &layoutInfo, nullptr, &m_pipelineLayout), "Failed to create pipeline layout")

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = uint32_t(m_shaderStageInfos.size());
        pipelineInfo.pStages = m_shaderStageInfos.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = m_config.subpassIdx;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (m_pipelineCache == VK_NULL_HANDLE) {
            VkPipelineCacheCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            createInfo.initialDataSize = 0;
            m_deviceFuncs->vkCreatePipelineCache(m_window->device(), &createInfo, nullptr, &m_pipelineCache);
        }

        VPA_VKCRITICAL_PASS(m_deviceFuncs->vkCreateGraphicsPipelines(m_window->device(), m_pipelineCache, 1, &pipelineInfo, nullptr, &m_pipeline), "Failed to create pipeline")

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
        if (err.level != VPAErrorLevel::Ok) {
            delete m_vertexInput;
            return err;
        }

        if (m_descriptors) delete m_descriptors;
        m_descriptors = new Descriptors(m_window, m_deviceFuncs, m_allocator, m_shaderAnalytics->DescriptorLayoutMap(), m_shaderAnalytics->PushConstantRanges(), err);
        if (err.level != VPAErrorLevel::Ok) {
            delete m_descriptors;
            return err;
        }

        m_creationCallback();
        return VPA_OK;
    }
}

#include "vulkanrenderer.h"

#include <QVulkanDeviceFunctions>
#include <QFile>
#include <QCoreApplication>
#include <QMatrix4x4>

#include "vulkanmain.h"
#include "shaderanalytics.h"
#include "filemanager.h"
#include "vertexinput.h"
#include "descriptors.h"

#define MESHDIR "../../Resources/Meshes/"

using namespace vpa;

VulkanRenderer::VulkanRenderer(QVulkanWindow* window, VulkanMain* main, std::function<void(void)> creationCallback)
    : m_initialised(false), m_window(window), m_pipelineCache(VK_NULL_HANDLE), m_pipeline(VK_NULL_HANDLE),
      m_pipelineLayout(VK_NULL_HANDLE), m_renderPass(VK_NULL_HANDLE), m_vertexInput(nullptr), m_descriptors(nullptr),
      m_creationCallback(creationCallback) {
    main->m_renderer = this;
    m_config = {};
}

void VulkanRenderer::initResources() {
    m_deviceFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    m_allocator = new MemoryAllocator(m_deviceFuncs, m_window);
    m_shaderAnalytics = new ShaderAnalytics(m_deviceFuncs, m_window->device(), &m_config);
}

void VulkanRenderer::initSwapChainResources() {
    if (!m_initialised) {
        Reload(ReloadFlags::EVERYTHING);
        m_initialised = true;
    }
}

void VulkanRenderer::releaseSwapChainResources() {
}

void VulkanRenderer::releaseResources() {
    DESTROY_HANDLE(m_window->device(), m_pipeline, m_deviceFuncs->vkDestroyPipeline);
    DESTROY_HANDLE(m_window->device(), m_pipelineLayout, m_deviceFuncs->vkDestroyPipelineLayout);
    DESTROY_HANDLE(m_window->device(), m_pipelineCache, m_deviceFuncs->vkDestroyPipelineCache);
    DESTROY_HANDLE(m_window->device(), m_renderPass, m_deviceFuncs->vkDestroyRenderPass);
    delete m_shaderAnalytics;
    if (m_vertexInput) delete m_vertexInput;
    if (m_descriptors) delete m_descriptors;
    if (m_config.viewports) delete[] m_config.viewports;
    delete m_allocator;
}

void VulkanRenderer::startNextFrame() {
    VkClearValue clearValues[2];
    clearValues[0].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = m_renderPass;
    beginInfo.framebuffer = m_window->currentFramebuffer();

    const QSize imageSize = m_window->swapChainImageSize();
    beginInfo.renderArea.extent.width = uint32_t(imageSize.width());
    beginInfo.renderArea.extent.height = uint32_t(imageSize.height());
    beginInfo.clearValueCount = 2;
    beginInfo.pClearValues = clearValues;

    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    m_deviceFuncs->vkCmdBeginRenderPass(cmdBuf, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

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
    m_deviceFuncs->vkCmdEndRenderPass(cmdBuf);

    m_window->frameReady();
    m_window->requestUpdate();
}

bool VulkanRenderer::WritePipelineCache() {
    bool success = false;
    size_t size;
    char* data = nullptr;
    if (m_deviceFuncs->vkGetPipelineCacheData(m_window->device(), m_pipelineCache, &size, data) != VK_SUCCESS) {
        qWarning("Failed to get pipeline cache data size");
    }
    else {
        data = new char[size];
        if (m_deviceFuncs->vkGetPipelineCacheData(m_window->device(), m_pipelineCache, &size, data) != VK_SUCCESS) {
            qWarning("Failed to get pipeline cache data");
        }
        else {
            QString path = QCoreApplication::applicationDirPath() + "/test.vpac";
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly)) {
                 qWarning("Could not open file test.vpac");
            }
            else {
                file.write(data, size);
                file.close();
                success = true;
            }
        }
        delete[] data;
    }
    return success;
}

bool VulkanRenderer::WritePipelineConfig() {
    bool success = true;
    FileManager<PipelineConfig>::Writer(m_config);
    return success;
}

bool VulkanRenderer::ReadPipelineConfig()
{
    bool success = false;
    FileManager<PipelineConfig>::Loader(m_config);
    FileManager<PipelineConfig>::Writer(m_config, "config_rewrite.vpa");
    success = true;
    return success;
}

void VulkanRenderer::Reload(const ReloadFlags flag) {
    m_deviceFuncs->vkDeviceWaitIdle(m_window->device());
    if (flag & ReloadFlagBits::DESCRIPTOR_VALUES) UpdateDescriptorData();
    if (flag & ReloadFlagBits::RENDER_PASS) CreateRenderPass();
    if (flag & ReloadFlagBits::SHADERS) CreateShaders();
    if (flag & ReloadFlagBits::PIPELINE) CreatePipeline();
    m_creationCallback();
}

VkAttachmentDescription VulkanRenderer::makeAttachment(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
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

VkSubpassDescription VulkanRenderer::makeSubpass(VkPipelineBindPoint pipelineType, QVector<VkAttachmentReference>& colourReferences, VkAttachmentReference* depthReference, VkAttachmentReference* resolve) {
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = uint32_t(colourReferences.size());
    subpass.pColorAttachments = colourReferences.data();
    subpass.pDepthStencilAttachment = depthReference;
    subpass.pResolveAttachments = resolve;
    return subpass;
}

VkSubpassDependency VulkanRenderer::makeSubpassDependency(uint32_t srcIdx, uint32_t dstIdx, VkPipelineStageFlags srcStage,
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

void VulkanRenderer::CreateRenderPass() {
    DESTROY_HANDLE(m_window->device(), m_renderPass, m_deviceFuncs->vkDestroyRenderPass);

    QVector<VkAttachmentDescription> attachments;
    QVector<VkSubpassDescription> subpasses;
    QVector<VkSubpassDependency> dependencies;

    // Colour
    attachments.push_back(makeAttachment(m_window->colorFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
    // Depth
    attachments.push_back(makeAttachment(m_window->depthStencilFormat(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    QVector<VkAttachmentReference> colourAttachmentRefs;
    VkAttachmentReference positionsRef = {};
    positionsRef.attachment = 0;
    positionsRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colourAttachmentRefs.push_back(positionsRef);

    dependencies.push_back(makeSubpassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));
    dependencies.push_back(makeSubpassDependency(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT));

    subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, &depthAttachmentRef, nullptr));

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (m_deviceFuncs->vkCreateRenderPass(m_window->device(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        qFatal("Failed to create render pass");
        return;
    }
}

void VulkanRenderer::CreatePipeline() {
    DESTROY_HANDLE(m_window->device(), m_pipeline, m_deviceFuncs->vkDestroyPipeline);
    DESTROY_HANDLE(m_window->device(), m_pipelineLayout, m_deviceFuncs->vkDestroyPipelineLayout);

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = m_descriptors->DescriptorSetLayouts().size();
    layoutInfo.pSetLayouts = m_descriptors->DescriptorSetLayouts().data();
    layoutInfo.pushConstantRangeCount = m_descriptors->PushConstantRanges().size();
    layoutInfo.pPushConstantRanges = m_descriptors->PushConstantRanges().data();

    auto bindingDesc = m_vertexInput->InputBindingDescription();
    auto attribDescs = m_vertexInput->InputAttribDescription();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = attribDescs.size();
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.pVertexAttributeDescriptions = attribDescs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = m_config.writablePipelineConfig.topology;
    inputAssembly.primitiveRestartEnable = m_config.writablePipelineConfig.primitiveRestartEnable;

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
    rasterizer.depthClampEnable = m_config.writablePipelineConfig.depthClampEnable;
    rasterizer.rasterizerDiscardEnable = m_config.writablePipelineConfig.rasterizerDiscardEnable;
    rasterizer.polygonMode = m_config.writablePipelineConfig.polygonMode;
    rasterizer.lineWidth = m_config.writablePipelineConfig.lineWidth;
    rasterizer.cullMode = m_config.writablePipelineConfig.cullMode;
    rasterizer.frontFace = m_config.writablePipelineConfig.frontFace;
    rasterizer.depthBiasEnable = m_config.writablePipelineConfig.depthBiasEnable;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = m_config.writablePipelineConfig.msaaSamples != VK_SAMPLE_COUNT_1_BIT;
    //TODO change this back if multisampling is working
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    //multisampling.rasterizationSamples = m_config.writablePipelineConfig.msaaSamples;
    multisampling.minSampleShading = m_config.writablePipelineConfig.minSampleShading;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = m_config.writablePipelineConfig.depthTestEnable;
    depthStencil.depthWriteEnable = m_config.writablePipelineConfig.depthWriteEnable;
    depthStencil.depthCompareOp = m_config.writablePipelineConfig.depthCompareOp;
    depthStencil.depthBoundsTestEnable = m_config.writablePipelineConfig.depthBoundsTest;
    depthStencil.stencilTestEnable = m_config.writablePipelineConfig.stencilTestEnable;

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
    for (size_t i = 0; i < 1; ++i) {
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = m_config.writablePipelineConfig.attachments.blendEnable;
        colorBlendAttachment.alphaBlendOp = m_config.writablePipelineConfig.attachments.alphaBlendOp;
        colorBlendAttachment.srcAlphaBlendFactor = m_config.writablePipelineConfig.attachments.srcAlphaBlendFactor;
        colorBlendAttachment.dstAlphaBlendFactor = m_config.writablePipelineConfig.attachments.dstAlphaBlendFactor;
        colorBlendAttachment.colorBlendOp = m_config.writablePipelineConfig.attachments.colourBlendOp;
        colorBlendAttachment.srcColorBlendFactor = m_config.writablePipelineConfig.attachments.srcColourBlendFactor;
        colorBlendAttachment.dstColorBlendFactor = m_config.writablePipelineConfig.attachments.dstColourBlendFactor;
        colorBlendAttachments.push_back(colorBlendAttachment);
    }

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = m_config.writablePipelineConfig.logicOpEnable;
    colorBlending.logicOp = m_config.writablePipelineConfig.logicOp;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = colorBlendAttachments.data();
    memcpy(colorBlending.blendConstants, m_config.writablePipelineConfig.blendConstants, 4 * sizeof(float));

    if (m_deviceFuncs->vkCreatePipelineLayout(m_window->device(), &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        qFatal("Failed to create pipeline layout");
    }

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

    if(m_deviceFuncs->vkCreateGraphicsPipelines(m_window->device(), m_pipelineCache, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        qFatal("Failed to create pipeline");
    }
}

void VulkanRenderer::CreateShaders() {
    // Load and analyse shaders
    m_shaderStageInfos.clear();
    m_shaderAnalytics->LoadShaders("/../shaders/vs_test.spv", "/../shaders/fs_test.spv");//, "/../shaders/tesc_test.spv", "/../shaders/tese_test.spv", "/../shaders/gs_test.spv");
    VkPipelineShaderStageCreateInfo shaderCreateInfo;
    if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::VERTEX, shaderCreateInfo)) m_shaderStageInfos.push_back(shaderCreateInfo);
    if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::FRAGMENT, shaderCreateInfo)) m_shaderStageInfos.push_back(shaderCreateInfo);
    if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::TESS_CONTROL, shaderCreateInfo)) m_shaderStageInfos.push_back(shaderCreateInfo);
    if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::TESS_EVAL, shaderCreateInfo)) m_shaderStageInfos.push_back(shaderCreateInfo);
    if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::GEOMETRY, shaderCreateInfo)) m_shaderStageInfos.push_back(shaderCreateInfo);

    // Determine new vertex input
    if (m_vertexInput) delete m_vertexInput;
    m_vertexInput = new VertexInput(m_window, m_deviceFuncs, m_allocator, m_shaderAnalytics->InputAttributes(), MESHDIR"Teapot", true);

    // Determine new descriptor layout
    if (m_descriptors) delete m_descriptors;
    m_descriptors = new Descriptors(m_window, m_deviceFuncs, m_allocator, m_shaderAnalytics->DescriptorLayoutMap(), m_shaderAnalytics->PushConstantRanges());

    // ------- Test data TODO remove when interface complete ------
    QMatrix4x4 model;
    QMatrix4x4 view;
    QMatrix4x4 projection;
    model.setToIdentity();
    model.scale(0.1f, 0.1f, 0.1f);
    view.lookAt(QVector3D(0.0, 10.0, 20.0), QVector3D(0.0, 0.0, 0.0), QVector3D(0.0, 1.0, 0.0));
    projection.perspective(45.0, m_window->width() / m_window->height(), 1.0, 100.0);
    projection.data()[5] *= -1;
    QMatrix4x4 mvp = projection * view * model;
    m_descriptors->WriteBufferData(0, 0, 16 * sizeof(float), 0, mvp.data());

    float colourMask[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    m_descriptors->WritePushConstantData(ShaderStage::FRAGMENT, 4 * sizeof(float), colourMask);

    // TODO Determine new colour attachment count
}

void VulkanRenderer::UpdateDescriptorData() {
    // TODO link up to descriptor update functions
}

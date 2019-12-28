/*
 * Author: Ralph Ridley
 * Date: 20/12/19
 * Modified By: Ori Lazar
     .---.
   .'_:___".
   |__ --==|
   [  ]  :[|
   |__| I=[|
   / / ____|
  |-/.____.'
 /___\ /___\
 */

#include "vulkanrenderer.h"

#include <QVulkanDeviceFunctions>
#include <QFile>
#include <QCoreApplication>
#include "vulkanmain.h"
#include "shaderanalytics.h"
#include "filemanager.h"

using namespace vpa;

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

static Vertex vertexData[] = {
    { -1, -1, 0, 1, 0, 0, 1 },
    {-1, 1, 0, 0, 1, 0, 1 },
    { 1, -1, 0, 0, 0, 1, 1},
    { 1, 1, 0, 1, 1, 1, 1}
};

VulkanRenderer::VulkanRenderer(QVulkanWindow* window, VulkanMain* main) : m_window(window), m_pipelineCache(VK_NULL_HANDLE), m_initialised(false) {
    main->m_renderer = this;
    m_config = {};
}

void VulkanRenderer::initResources() {
    m_deviceFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    m_shaderAnalytics = new ShaderAnalytics(m_deviceFuncs, m_window->device());
    m_shaderAnalytics->m_pConfig = &m_config; //TODO: Change the way these guys connect with eachother
}

void VulkanRenderer::initSwapChainResources() {
    if (!m_initialised) {
        CreateVertexBuffer();
        CreateRenderPass(m_config);
        m_initialised = true;
    }
}

void VulkanRenderer::releaseSwapChainResources() {
}

void VulkanRenderer::releaseResources() {
    m_deviceFuncs->vkDestroyPipeline(m_window->device(), m_pipeline, nullptr);
    m_deviceFuncs->vkDestroyPipelineLayout(m_window->device(), m_pipelineLayout, nullptr);
    //m_deviceFuncs->vkDestroyPipelineCache(m_window->device(), m_pipelineCache, nullptr);
    m_deviceFuncs->vkDestroyRenderPass(m_window->device(), m_renderPass, nullptr);
    delete m_shaderAnalytics;
}

void VulkanRenderer::startNextFrame() {

    VkClearValue clearValues[2];
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = m_renderPass;
    beginInfo.framebuffer = m_window->currentFramebuffer();//

    const QSize imageSize = m_window->swapChainImageSize();
    beginInfo.renderArea.extent.width = imageSize.width();
    beginInfo.renderArea.extent.height = imageSize.height();
    beginInfo.clearValueCount = 2;
    beginInfo.pClearValues = clearValues;

    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    m_deviceFuncs->vkCmdBeginRenderPass(cmdBuf, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkDeviceSize offset = 0;
    m_deviceFuncs->vkCmdBindVertexBuffers(cmdBuf, 0, 1, &m_vertexBuffer, &offset);
    m_deviceFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    m_deviceFuncs->vkCmdDraw(cmdBuf, 4, 1, 0, 0);
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
    FileManager<PipelineConfig>::Writer(m_config); //todo: return boolean
    return success;
}

bool VulkanRenderer::ReadPipelineConfig()
{
    bool success = false;
    FileManager<PipelineConfig>::Loader(m_config); //todo: return boolean
    FileManager<PipelineConfig>::Writer(m_config, "config_rewrite.vpa"); //todo: return boolean
    success = true;
    return success;
}

VkAttachmentDescription makeAttachment(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
    VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp, VkImageLayout initialLayout, VkImageLayout finalLayout)
{
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

VkSubpassDescription makeSubpass(VkPipelineBindPoint pipelineType, QVector<VkAttachmentReference>& colourReferences, VkAttachmentReference* depthReference, VkAttachmentReference* resolve)
{
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colourReferences.size());
    subpass.pColorAttachments = colourReferences.data();
    subpass.pDepthStencilAttachment = depthReference;
    subpass.pResolveAttachments = resolve;
    return subpass;
}

VkSubpassDependency makeSubpassDependency(uint32_t srcIdx, uint32_t dstIdx, VkPipelineStageFlags srcStage,
    VkAccessFlags srcAccess, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess)
{
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

// From https://doc.qt.io/archives/qt-5.11/qtgui-hellovulkantexture-hellovulkantexture-cpp.html
VkShaderModule VulkanRenderer::CreateShader(const QString& name) {
    QFile file(QCoreApplication::applicationDirPath() + name);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to read shader %s", qPrintable(name));
        return VK_NULL_HANDLE;
    }
    QByteArray blob = file.readAll();
    file.close();

    VkShaderModuleCreateInfo shaderInfo;
    memset(&shaderInfo, 0, sizeof(shaderInfo));
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = blob.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t *>(blob.constData());
    VkShaderModule shaderModule;
    VkResult err = m_deviceFuncs->vkCreateShaderModule(m_window->device(), &shaderInfo, nullptr, &shaderModule);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create shader module: %d", err);
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

void VulkanRenderer::CreateRenderPass(PipelineConfig& config) {
    VkDevice device = m_window->device();
    m_deviceFuncs->vkDeviceWaitIdle(device);

    QVector<VkAttachmentDescription> attachments;
    QVector<VkSubpassDescription> subpasses;
    QVector<VkSubpassDependency> dependencies;

    //createColourBuffer(device, swapChainDetails); // TODO
    //auto depthFormat = createDepthBuffer(device, swapChainDetails); // TODO

    // Colour
    attachments.push_back(makeAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    // Depth
    //attachments.push_back(makeAttachment(depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
    //    VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

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

    subpasses.push_back(makeSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachmentRefs, nullptr, nullptr));

    /*QVector<VkImageView> attachmentImageViews;
    for (Image& image : m_attachmentImages) {
        attachmentImageViews.push_back(image.view);
    }*/

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (m_deviceFuncs->vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        qFatal("Failed to create render pass");
        return;
    }

    /*m_frameBuffers.resize(m_window->swapChainImageCount());
    for (size_t i = 1; i < m_window->swapChainImageCount(); ++i) {
        attachmentImageViews[i] = m_window->swapChainImageView(i);
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentImageViews.size());
        framebufferInfo.pAttachments = attachmentImageViews.data();
        framebufferInfo.width = m_window->swapChainImageSize().width();
        framebufferInfo.height = m_window->swapChainImageSize().height();
        framebufferInfo.layers = 1;

        if (m_deviceFuncs->vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_frameBuffers[i]) != VK_SUCCESS) {
            qFatal("Failed to create framebuffer.");
            return;
        }
    }*/

    CreatePipeline(config);
}

void VulkanRenderer::CreatePipeline(PipelineConfig& config) {
    QVector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
    m_shaderAnalytics->LoadShaders("/../shaders/vs_test.spv", "/../shaders/fs_test.spv");//, "/../shaders/tesc_test.spv", "/../shaders/tese_test.spv", "/../shaders/gs_test.spv");
    VkPipelineShaderStageCreateInfo shaderCreateInfo;
    if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::VETREX, shaderCreateInfo)) shaderStageInfos.push_back(shaderCreateInfo);
    if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::FRAGMENT, shaderCreateInfo)) shaderStageInfos.push_back(shaderCreateInfo);
    if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::TESS_CONTROL, shaderCreateInfo)) shaderStageInfos.push_back(shaderCreateInfo);
    if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::TESS_EVAL, shaderCreateInfo)) shaderStageInfos.push_back(shaderCreateInfo);
    if (m_shaderAnalytics->GetStageCreateInfo(ShaderStage::GEOMETRY, shaderCreateInfo)) shaderStageInfos.push_back(shaderCreateInfo);

    // TODO actual configurable layouts
    VkPushConstantRange pcRange = {};
    pcRange.size = 16 * sizeof(float);
    pcRange.offset = 0;
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 0;
    layoutInfo.pSetLayouts = nullptr;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pcRange;

    VkVertexInputBindingDescription bindingDesc = {
       0,
       7 * sizeof(float),
       VK_VERTEX_INPUT_RATE_VERTEX
   };
   VkVertexInputAttributeDescription attribDescs[] = {
       {
           0,
           0,
           VK_FORMAT_R32G32B32_SFLOAT,
           0
       },
       { // colour
           1,
           0,
           VK_FORMAT_R32G32B32A32_SFLOAT,
           3 * sizeof(float)
       }
   };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.pVertexAttributeDescriptions = attribDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = config.writablePipelineConfig.topology;
    inputAssembly.primitiveRestartEnable = config.writablePipelineConfig.primitiveRestartEnable;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_window->swapChainImageSize().width();
    viewport.height = (float)m_window->swapChainImageSize().height();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { (uint32_t)m_window->swapChainImageSize().width(), (uint32_t)m_window->swapChainImageSize().height() };

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = config.writablePipelineConfig.depthClampEnable;
    rasterizer.rasterizerDiscardEnable = config.writablePipelineConfig.rasterizerDiscardEnable;
    rasterizer.polygonMode = config.writablePipelineConfig.polygonMode;
    rasterizer.lineWidth = config.writablePipelineConfig.lineWidth;
    rasterizer.cullMode = config.writablePipelineConfig.cullMode;
    rasterizer.frontFace = config.writablePipelineConfig.frontFace;
    rasterizer.depthBiasEnable = config.writablePipelineConfig.depthBiasEnable;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = config.writablePipelineConfig.msaaSamples != VK_SAMPLE_COUNT_1_BIT;
    multisampling.rasterizationSamples = config.writablePipelineConfig.msaaSamples;
    multisampling.minSampleShading = config.writablePipelineConfig.minSampleShading;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = config.writablePipelineConfig.depthTestEnable;
    depthStencil.depthWriteEnable = config.writablePipelineConfig.depthWriteEnable;
    depthStencil.depthCompareOp = config.writablePipelineConfig.depthCompareOp;
    depthStencil.depthBoundsTestEnable = config.writablePipelineConfig.depthBoundsTest;
    depthStencil.stencilTestEnable = config.writablePipelineConfig.stencilTestEnable;

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
    for (size_t i = 0; i < 1; ++i) {
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = config.writablePipelineConfig.attachments.blendEnable;
        colorBlendAttachment.alphaBlendOp = config.writablePipelineConfig.attachments.alphaBlendOp;
        colorBlendAttachment.srcAlphaBlendFactor = config.writablePipelineConfig.attachments.srcAlphaBlendFactor;
        colorBlendAttachment.dstAlphaBlendFactor = config.writablePipelineConfig.attachments.dstAlphaBlendFactor;
        colorBlendAttachment.colorBlendOp = config.writablePipelineConfig.attachments.colourBlendOp;
        colorBlendAttachment.srcColorBlendFactor = config.writablePipelineConfig.attachments.srcColourBlendFactor;
        colorBlendAttachment.dstColorBlendFactor = config.writablePipelineConfig.attachments.dstColourBlendFactor;
        colorBlendAttachments.push_back(colorBlendAttachment);
    }

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = config.writablePipelineConfig.logicOpEnable;
    colorBlending.logicOp = config.writablePipelineConfig.logicOp;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = colorBlendAttachments.data();
    memcpy(colorBlending.blendConstants, config.writablePipelineConfig.blendConstants, 4 * sizeof(float));

    if (m_deviceFuncs->vkCreatePipelineLayout(m_window->device(), &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        qFatal("Failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStageInfos.size());
    pipelineInfo.pStages = shaderStageInfos.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = config.subpassIdx;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    /*VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
    if (pipelineCreateInfo.depthBiasEnable) pipelineCreateInfo.dynamicState.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    if (pipelineCreateInfo.dynamicState.size() > 0) {
        dynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0, (uint32_t)pipelineCreateInfo.dynamicState.size(), pipelineCreateInfo.dynamicState.data() };
        pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
    }

    if (inputAssembly.topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST && tessellationInfo != nullptr) {
        pipelineInfo.pTessellationState = tessellationInfo;
    }*/

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

void VulkanRenderer::CreateVertexBuffer() {
    VkBufferCreateInfo bufInfo = {};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    const VkDeviceSize vertexAllocSize = sizeof(vertexData);
    bufInfo.size = vertexAllocSize;
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    if (m_deviceFuncs->vkCreateBuffer(m_window->device(), &bufInfo, nullptr, &m_vertexBuffer) != VK_SUCCESS) {
        qFatal("Failed to create vertex buffer");
    }

    VkMemoryRequirements memReq;
    m_deviceFuncs->vkGetBufferMemoryRequirements(m_window->device(), m_vertexBuffer, &memReq);

    VkMemoryAllocateInfo memAllocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReq.size,
        m_window->hostVisibleMemoryIndex()
    };

    if (m_deviceFuncs->vkAllocateMemory(m_window->device(), &memAllocInfo, nullptr, &m_vertexMemory) != VK_SUCCESS) {
        qFatal("Failed to allocate memory for vertex buffer");
    }

    if (m_deviceFuncs->vkBindBufferMemory(m_window->device(), m_vertexBuffer, m_vertexMemory, 0) != VK_SUCCESS) {
        qFatal("Failed to bind vertex buffer");
    }

    quint8* data;
    if (m_deviceFuncs->vkMapMemory(m_window->device(), m_vertexMemory, 0, memReq.size, 0, reinterpret_cast<void **>(&data)) != VK_SUCCESS) {
        qFatal("Failed to map vertex buffer memory");
    }
    memcpy(data, vertexData, sizeof(vertexData));
    m_deviceFuncs->vkUnmapMemory(m_window->device(), m_vertexMemory);
}

VulkanRenderer::Image VulkanRenderer::CreateImage(const QString& name, VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height) {
    Image img;
    img.name = name;
    img.width = width;
    img.height = height;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (m_deviceFuncs->vkCreateImage(m_window->device(), &imageInfo, nullptr, &img.image) != VK_SUCCESS) {
        qFatal("Failed to create image.");
    }

    VkMemoryRequirements memReq;
    m_deviceFuncs->vkGetImageMemoryRequirements(m_window->device(), img.image, &memReq);

    VkMemoryAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReq.size,
        m_window->deviceLocalMemoryIndex()
    };
    if (m_deviceFuncs->vkAllocateMemory(m_window->device(), &allocInfo, nullptr, &img.memory) != VK_SUCCESS) {
        qFatal("Could not allocate memory for image");
    }
    // TODO finish function lol
    return img;
}

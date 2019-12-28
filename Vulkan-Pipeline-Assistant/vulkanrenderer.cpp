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

#define MESHDIR "../../Resources/Meshes/"

#include "vulkanrenderer.h"

#include <QVulkanDeviceFunctions>
#include <QFile>
#include <QCoreApplication>
#include "vulkanmain.h"
#include "shaderanalytics.h"
#include "filemanager.h"
#include "vertexinput.h"

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

VulkanRenderer::VulkanRenderer(QVulkanWindow* window, VulkanMain* main)
    : m_window(window), m_pipelineCache(VK_NULL_HANDLE), m_initialised(false), m_vertexInput(nullptr){
    main->m_renderer = this;
    m_config = {};
}

void VulkanRenderer::initResources() {
    m_deviceFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    m_allocator = new MemoryAllocator(m_deviceFuncs, m_window);
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
    m_deviceFuncs->vkDestroyPipelineCache(m_window->device(), m_pipelineCache, nullptr);
    m_deviceFuncs->vkDestroyRenderPass(m_window->device(), m_renderPass, nullptr);
    delete m_shaderAnalytics;
    if (m_vertexInput) delete m_vertexInput;
    m_allocator->Deallocate(m_vertexAllocation);
    delete m_allocator;
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
    m_deviceFuncs->vkCmdBindVertexBuffers(cmdBuf, 0, 1, &m_vertexAllocation.buffer, &offset);
    //m_vertexInput->BindBuffers(cmdBuf);
    m_deviceFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    //if (m_vertexInput->IsIndexed()) {
    //    m_deviceFuncs->vkCmdDrawIndexed(cmdBuf, m_vertexInput->IndexCount(), 1, 0, 0, 0);
    //}
    //else {
        m_deviceFuncs->vkCmdDraw(cmdBuf, 4, 1, 0, 0);
    //}
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
    subpass.colorAttachmentCount = static_cast<uint32_t>(colourReferences.size());
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

void VulkanRenderer::CreateRenderPass(PipelineConfig& config) {
    VkDevice device = m_window->device();
    m_deviceFuncs->vkDeviceWaitIdle(device);

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

    if (m_deviceFuncs->vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        qFatal("Failed to create render pass");
        return;
    }

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

    if (m_vertexInput) delete m_vertexInput;
    //m_vertexInput = new VertexInput(m_window, m_deviceFuncs, m_shaderAnalytics->InputAttributes(), MESHDIR"Teapot", true);

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

    //auto bindingDesc = m_vertexInput->InputBindingDescription();
    //auto attribDescs = m_vertexInput->InputAttribDescription();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;//attribDescs.size();
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.pVertexAttributeDescriptions = attribDescs;//.data();

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
    m_vertexAllocation = m_allocator->Allocate(sizeof(vertexData), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    unsigned char* dataPtr = m_allocator->MapMemory(m_vertexAllocation);
    memcpy(dataPtr, vertexData, sizeof(vertexData));
    m_allocator->UnmapMemory(m_vertexAllocation);
}

#ifndef PIPELINECONFIG_H
#define PIPELINECONFIG_H

#include <vulkan/vulkan.h>

#include <iostream>
#include <QFile>

#include "spirvresource.h"
#include "common.h"

namespace vpa {
    struct ColourAttachmentConfig {
        VkBool32 blendEnable = VK_TRUE;
        VkColorComponentFlags writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;
        VkBlendOp colourBlendOp = VK_BLEND_OP_ADD;
        VkBlendFactor srcColourBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        VkBlendFactor dstColourBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    };

    struct WritablePipelineConfig {
        //TODO: Move me for good coding practice
        std::ostream& WriteShaderDataToFile(std::ostream& out, const QByteArray* byteArray) const;

        // Shader Data
        QByteArray shaderBlobs[size_t(ShaderStage::Count_)] = {
                QByteArray(1, '0'), QByteArray(1, '0'), QByteArray(1, '0'), QByteArray(1, '0'), QByteArray(1, '0')
        };

        // Vertex input
        uint32_t vertexBindingCount = 0;
        uint32_t vertexAttribCount = 0;

        VkVertexInputBindingDescription vertexBindingDescriptions = {2, 5, VK_VERTEX_INPUT_RATE_MAX_ENUM};
        int numAttribDescriptions = 1;
        VkVertexInputAttributeDescription vertexAttribDescriptions[4];

        // Vertex input assmebly
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkBool32 primitiveRestartEnable = VK_FALSE;

        // Tessellation state
        uint32_t patchControlPoints = 0;

        // Rasteriser state
        VkBool32 rasterizerDiscardEnable = VK_FALSE;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        float lineWidth = 1.0f;
        VkCullModeFlagBits cullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        VkBool32 depthClampEnable = VK_FALSE;
        VkBool32 depthBiasEnable = VK_FALSE;
        float depthBiasConstantFactor = 1.0f;
        float depthBiasClamp = 1.0f;
        float depthBiasSlopeFactor = 1.0f;

        // Multisample state
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        float minSampleShading = 0.2f;

        // Depth stencil state
        VkBool32 depthTestEnable = VK_FALSE;
        VkBool32 depthWriteEnable = VK_FALSE;
        VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
        VkBool32 depthBoundsTest = VK_FALSE;
        VkBool32 stencilTestEnable = VK_FALSE;

        // Colour blend attachment states (vector)
        ColourAttachmentConfig attachments;

        // Colour blend state
        VkBool32 logicOpEnable = VK_FALSE;
        VkLogicOp logicOp = VK_LOGIC_OP_COPY;
        float blendConstants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    };

    struct PipelineConfig {
        VPAError LoadConfiguration(std::vector<char>& buffer, const int bufferSize);

        // Shaders
        QString vertShader = "";
        QString fragShader = "";
        QString tescShader = "";
        QString teseShader = "";
        QString geomShader = "";

        // Pipeline layout ##
        //TODO: Make viewport and scissor rects more configurable
        //TODO: Multisample coverage stuff
        //TODO: Dynamic state

        // Viewport
        VkViewport* viewports = nullptr;

        // Scissor
        VkRect2D* scissorRects = nullptr;

        // Viewport state
        uint32_t viewportCount = 0;
        uint32_t scissorCount = 0;

        // Render pass info
        VkRenderPass renderPass = VK_NULL_HANDLE;
        uint32_t subpassIdx = 0;
        uint32_t outputAttachmentIndex = 0;
        uint32_t attachmentCount = 0;

        WritablePipelineConfig writables;
    };

    std::ostream& operator<<(std::ostream& out, const WritablePipelineConfig& config);
    std::ostream& operator<<(std::ostream& out, const PipelineConfig& config);

}
#endif // PIPELINECONFIG_H

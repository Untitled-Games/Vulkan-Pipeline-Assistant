/*
 * Author: Ralph Ridley
 * Date: 19/12/19
 * Define all configuration options for a graphics pipeline.
 */

#ifndef PIPELINECONFIG_H
#define PIPELINECONFIG_H

#include <vulkan/vulkan.h>

#include <iostream>

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

    struct PipelineConfig {

        //todo: Note down the size of these shaders somewhere so that we can  serialize this into binary format
        // Shaders
        char* vertShader = nullptr;
        char* fragShader = nullptr;
        char* tescShader = nullptr;
        char* teseShader = nullptr;
        char* geomShader = nullptr;

        // Vertex input
        uint32_t vertexBindingCount = 0;
        uint32_t vertexAttribCount = 0;



        const VkVertexInputBindingDescription* vertexBindingDescriptions;
        const VkVertexInputAttributeDescription* vertexAttribDescriptions;

        // Vertex input assembly
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        VkBool32 primitiveRestartEnable = VK_FALSE;

        // Tessellation state
        uint32_t patchControlPoints = 0;

        // Pipeline layout ##

        // Viewport
        VkViewport* viewports = nullptr;

        // Scissor
        VkRect2D* scissorRects = nullptr;

        // Viewport state
        uint32_t viewportCount = 0;
        uint32_t scissorCount = 0;

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
        // TODO multisample coverage stuff

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

        // Dynamic state TODO

        // Render pass info
        VkRenderPass renderPass = VK_NULL_HANDLE;
        uint32_t subpassIdx = 0;
        uint32_t outputAttachmentIndex = 0;
        uint32_t attachmentCount = 0;
    };

    std::ostream& operator<<(std::ostream& out, const PipelineConfig& config);
    std::istream& operator>>(std::istream& in, PipelineConfig& config);
}

#endif // PIPELINECONFIG_H

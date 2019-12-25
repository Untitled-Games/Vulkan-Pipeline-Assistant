/*
 * Author: Ralph Ridley
 * Date: 20/12/19
 * Functionality for rendering through the Vulkan API.
 */

#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QVulkanWindowRenderer>
#include "pipelineconfig.h"

namespace vpa {
    class VulkanMain;
    class ShaderAnalytics;
    class VulkanRenderer : public QVulkanWindowRenderer {
        struct Image {
            QString name;
            VkImage image;
            VkImageView view;
            VkDeviceMemory memory;
            uint32_t width;
            uint32_t height;
        };

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

        //todo: merge this -- Configuration Branch --
        bool WritePipelineConfig(); 
        bool ReadPipelineConfig();
    private:
        void CreateRenderPass(PipelineConfig& config);
        void CreatePipeline(PipelineConfig& config);
        void CreatePipelineCache();
        void CreateVertexBuffer();

        VkShaderModule CreateShader(const QString& name);
        Image CreateImage(const QString& name, VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height);

        bool m_initialised;
        QVulkanWindow* m_window;
        QVulkanDeviceFunctions* m_deviceFuncs;

        VkRenderPass m_renderPass;
        VkPipeline m_pipeline;
        VkPipelineLayout m_pipelineLayout;
        VkPipelineCache m_pipelineCache;
        ShaderAnalytics* m_shaderAnalytics;

        QVector<VkFramebuffer> m_frameBuffers;
        QVector<Image> m_attachmentImages;

        VkBuffer m_vertexBuffer;
        VkDeviceMemory m_vertexMemory;
        VkBuffer m_indexBuffer;

        Image m_texture;

        PipelineConfig m_config;
    };
}

#endif // VULKANRENDERER_H

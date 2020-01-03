#ifndef SHADER_H
#define SHADER_H

#include "spirvresource.h"
#include <Lib/spirv-cross/spirv_cross.hpp>

#include <QString>
#include <QPair>
#include <QHash>
#include <QVulkanDeviceFunctions>

namespace vpa {
    struct PipelineConfig;

    class ShaderAnalytics {
    public:
        ShaderAnalytics(QVulkanDeviceFunctions* deviceFuncs, VkDevice device, PipelineConfig* config);
        ~ShaderAnalytics();

        void LoadShaders(const QString& vert, const QString& frag = "", const QString& tesc = "", const QString& tese = "", const QString& geom = "");
        bool GetStageCreateInfo(ShaderStage stage, VkPipelineShaderStageCreateInfo& createInfo);
        VkShaderModule GetModule(ShaderStage stage);
        void SetShader(ShaderStage stage, const QString& name);

        QVector<SpirvResource> InputAttributes();
        size_t NumColourAttachments();
        DescriptorLayoutMap& DescriptorLayoutMap();
        QVector<SpirvResource> PushConstantRange(ShaderStage stage);

    private:
        void CreateModule(ShaderStage stage, const QString& name);
        size_t CalculateResourceSize(spirv_cross::Compiler* compiler, spirv_cross::Resource* res);
        void AnalyseDescriptorLayout();
        void Destroy();

        QVulkanDeviceFunctions* m_deviceFuncs;
        VkDevice m_device;
        PipelineConfig* m_config;

        VkShaderModule m_modules[size_t(ShaderStage::count_)];
        spirv_cross::Compiler* m_compilers[size_t(ShaderStage::count_)];
        spirv_cross::ShaderResources m_resources[size_t(ShaderStage::count_)];

        vpa::DescriptorLayoutMap m_descriptorLayoutMap;
    };
}

#endif // SHADER_H

/*
 * Author: Ralph Ridley
 * Date: 22/12/19
 * Wrap shader creation and analytics, managing reflection on the shader
 * code to determine requirements for the pipeline and render pass.
 */
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

    using DescriptorLayoutMap = QHash<QPair<uint32_t, uint32_t>, SpirvResource>;

    class ShaderAnalytics {
    public:
        ShaderAnalytics(QVulkanDeviceFunctions* deviceFuncs, VkDevice device);
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
        VkShaderStageFlagBits StageToVkStageFlag(ShaderStage stage);
        size_t CalculateResourceSize(spirv_cross::Compiler* compiler, spirv_cross::Resource* res);
        void AnalyseDescriptorLayout();

        QVulkanDeviceFunctions* m_deviceFuncs;
        VkDevice m_device;

        VkShaderModule m_modules[size_t(ShaderStage::count_)];
        spirv_cross::Compiler* m_compilers[size_t(ShaderStage::count_)];
        spirv_cross::ShaderResources m_resources[size_t(ShaderStage::count_)];

        vpa::DescriptorLayoutMap m_descriptorLayoutMap;

    //TODO: move this to a more organized location
    public:
        PipelineConfig* m_pConfig;
    };
}

#endif // SHADER_H

/*
 * Author: Ralph Ridley
 * Date: 22/12/19
 * Wrap shader creation and analytics, managing reflection on the shader
 * code to determine requirements for the pipeline and render pass.
 */
#ifndef SHADER_H
#define SHADER_H

#include <QString>
#include <QPair>
#include <QHash>
#include <QVulkanDeviceFunctions>
#include <Lib/spirv-cross/spirv_cross.hpp>

namespace vpa {

struct PipelineConfig;

    enum class ShaderStage {
        VETREX, FRAGMENT, TESS_CONTROL, TESS_EVAL, GEOMETRY, count_
    };

    enum class SpirvResourceType {
        INPUT_ATTRIBUTE, UNIFORM_BUFFER, STORAGE_BUFFER, SAMPLER_IMAGE, STORAGE_IMAGE, PUSH_CONSTANT
    };

    struct SpirvResource {
        QString name;
        size_t size;
        SpirvResourceType resourceType;
        SPIRV_CROSS_NAMESPACE::Resource spirvResource;

        bool operator==(const SpirvResource& other) {
            return name == other.name && size == other.size && resourceType == other.resourceType;
        }
        bool operator!=(const SpirvResource& other) {
            return !operator==(other);
        }
    };

    using DescriptorLayoutMap = QHash<QPair<uint32_t, uint32_t>, SpirvResource>;

    class ShaderAnalytics {
    public:
        ShaderAnalytics(QVulkanDeviceFunctions* deviceFuncs, VkDevice device);
        ~ShaderAnalytics();

        void LoadShaders(const QString& vert, const QString& frag = "", const QString& tesc = "", const QString& tese = "", const QString& geom = "");
        bool GetStageCreateInfo(ShaderStage stage, VkPipelineShaderStageCreateInfo& createInfo);
        VkShaderModule GetModule(ShaderStage stage);
        void SetShader(ShaderStage stage, const QString& name);

        SPIRV_CROSS_NAMESPACE::SmallVector<SPIRV_CROSS_NAMESPACE::Resource> InputAttributes();
        size_t NumColourAttachments();
        DescriptorLayoutMap& DescriptorLayoutMap();
        SPIRV_CROSS_NAMESPACE::SmallVector<SPIRV_CROSS_NAMESPACE::Resource> PushConstantRange(ShaderStage stage);

    private:
        void CreateModule(ShaderStage stage, const QString& name);
        VkShaderStageFlagBits StageToVkStageFlag(ShaderStage stage);
        size_t CalculateResourceSize(SPIRV_CROSS_NAMESPACE::Compiler* compiler, SPIRV_CROSS_NAMESPACE::Resource& res);
        void AnalyseDescriptorLayout();

        QVulkanDeviceFunctions* m_deviceFuncs;
        VkDevice m_device;

        VkShaderModule m_modules[size_t(ShaderStage::count_)];
        SPIRV_CROSS_NAMESPACE::Compiler* m_compilers[size_t(ShaderStage::count_)];
        SPIRV_CROSS_NAMESPACE::ShaderResources m_resources[size_t(ShaderStage::count_)];

        vpa::DescriptorLayoutMap m_descriptorLayoutMap;

    //TODO: move this to a more organized location
    public:
        PipelineConfig* m_pConfig;
    };
}

#endif // SHADER_H

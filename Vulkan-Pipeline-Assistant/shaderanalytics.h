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

        size_t NumColourAttachments() const;
        const QVector<SpvResource*>& InputAttributes() const { return m_inputAttributes; }
        const DescriptorLayoutMap& DescriptorLayoutMap() const { return m_descriptorLayoutMap; }
        const QVector<SpvResource*>& PushConstantRanges() const { return m_pushConstants; }

    private:
        void CreateModule(ShaderStage stage, const QString& name);

        size_t GetComponentSize(const spirv_cross::SPIRType& spirType);
        SpvType* CreateVectorType(const spirv_cross::SPIRType& spirType);
        SpvType* CreateMatrixType(const spirv_cross::SPIRType& spirType);
        SpvType* CreateImageType(const spirv_cross::SPIRType& spirType);
        SpvType* CreateArrayType(spirv_cross::Compiler* compiler, const spirv_cross::SPIRType& spirType);
        SpvType* CreateStructType(spirv_cross::Compiler* compiler,const spirv_cross::SPIRType& spirType);
        SpvType* CreateType(spirv_cross::Compiler* compiler, spirv_cross::Resource& res);
        SpvType* CreateType(spirv_cross::Compiler* compiler, const spirv_cross::SPIRType& spirType, bool ignoreArray = false);

        void BuildPushConstants();
        void BuildInputAttributes();
        void BuildDescriptorLayoutMap();
        void Destroy();

        QVulkanDeviceFunctions* m_deviceFuncs;
        VkDevice m_device;
        PipelineConfig* m_config;

        VkShaderModule m_modules[size_t(ShaderStage::count_)];
        spirv_cross::Compiler* m_compilers[size_t(ShaderStage::count_)];
        spirv_cross::ShaderResources m_resources[size_t(ShaderStage::count_)];

        QVector<SpvResource*> m_inputAttributes;
        QVector<SpvResource*> m_pushConstants;
        vpa::DescriptorLayoutMap m_descriptorLayoutMap;
    };
}

#endif // SHADER_H

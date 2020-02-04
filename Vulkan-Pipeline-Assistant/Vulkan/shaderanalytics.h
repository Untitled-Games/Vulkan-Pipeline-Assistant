#ifndef SHADER_H
#define SHADER_H

#include <Lib/spirv-cross/spirv_cross.hpp>
#include <QString>
#include <QPair>
#include <QHash>
#include <QVulkanDeviceFunctions>

#include "spirvresource.h"
#include "../common.h"

class QPlainTextEdit;

namespace vpa {
    struct PipelineConfig;

    class ShaderAnalytics final {
    public:
        ShaderAnalytics(QVulkanDeviceFunctions* deviceFuncs, VkDevice device, PipelineConfig* config);
        ~ShaderAnalytics();

        VPAError LoadShaders(const QString& vert, const QString& frag = "", const QString& tesc = "", const QString& tese = "", const QString& geom = "");
        bool GetStageCreateInfo(ShaderStage stage, VkPipelineShaderStageCreateInfo& createInfo);
        VkShaderModule GetModule(ShaderStage stage);
        void SetShader(ShaderStage stage, const QString& name);

        size_t NumColourAttachments() const;
        QStringList ColourAttachmentNames() const;
        const QVector<SpvResource*>& InputAttributes() const { return m_inputAttributes; }
        const DescriptorLayoutMap& DescriptorLayoutMap() const { return m_descriptorLayoutMap; }
        const QVector<SpvResource*>& PushConstantRanges() const { return m_pushConstants; }

        VPAError CreateModule(VkShaderModule& module, const QString& name, QByteArray* blob);

        static bool TryCompile(QString& srcName, QPlainTextEdit* console = nullptr);

    private:
        static QString SourceNameToBinaryName(const QString& srcName);
        VPAError CreateModule(ShaderStage stage, const QString& name);
        VPAError Validate(ShaderStage stage);

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
        VPAError BuildDescriptorLayoutMap();
        void Destroy();

        QVulkanDeviceFunctions* m_deviceFuncs;
        VkDevice m_device;
        PipelineConfig* m_config;

        VkShaderModule m_modules[size_t(ShaderStage::Count_)];
        spirv_cross::Compiler* m_compilers[size_t(ShaderStage::Count_)];
        spirv_cross::ShaderResources m_resources[size_t(ShaderStage::Count_)];

        QVector<SpvResource*> m_inputAttributes;
        QVector<SpvResource*> m_pushConstants;
        vpa::DescriptorLayoutMap m_descriptorLayoutMap;
    };
}

#endif // SHADER_H

#include "shaderanalytics.h"

#include <QFile>
#include <QCoreApplication>

#include "pipelineconfig.h"
#include "common.h"

using SPIRV_CROSS_NAMESPACE::SPIRType;
using SPIRV_CROSS_NAMESPACE::Compiler;
using SPIRV_CROSS_NAMESPACE::SmallVector;
using SPIRV_CROSS_NAMESPACE::Resource;

namespace vpa {
    const QString ShaderStageStrings[size_t(ShaderStage::Count_)] = { "Vertex", "Fragment", "TessControl", "TessEval", "Geometry" };
    const QString SpvGroupNameStrings[size_t(SpvGroupName::Count_)] = { "InputAttribute", "UniformBuffer", "StorageBuffer", "PushConstant", "Image" };
    const QString SpvTypeNameStrings[size_t(SpvTypeName::Count_)] = { "Image", "Array", "Vector", "Matrix", "Struct" };
    const QString SpvImageTypeNameStrings[size_t(SpvImageTypeName::Count_)] = { "Texture1D", "Texture2D", "Texture3D", "TextureCube", "UnknownTexture" };

    ShaderAnalytics::ShaderAnalytics(QVulkanDeviceFunctions* deviceFuncs, VkDevice device, PipelineConfig* config)
        : m_deviceFuncs(deviceFuncs), m_device(device), m_config(config) {
        memset(m_modules, VK_NULL_HANDLE, sizeof(m_modules));
        memset(m_compilers, NULL, sizeof(m_compilers));
    }

    ShaderAnalytics::~ShaderAnalytics() {
        Destroy();
    }

    void ShaderAnalytics::Destroy() {
        for (size_t i = 0; i < size_t(ShaderStage::Count_); ++i) {
            if (m_modules[i] != VK_NULL_HANDLE) m_deviceFuncs->vkDestroyShaderModule(m_device, m_modules[i], nullptr);
            if (m_compilers[i] != nullptr) delete m_compilers[i];
        }
        memset(m_modules, VK_NULL_HANDLE, sizeof(m_modules));
        memset(m_compilers, NULL, sizeof(m_compilers));

        for (SpvResource* resource : m_inputAttributes) {
            delete resource;
        }
        for (SpvResource* resource : m_pushConstants) {
            delete resource;
        }
        for (SpvResource* resource : m_descriptorLayoutMap.values()) {
            delete resource;
        }
        m_pushConstants.clear();
        m_inputAttributes.clear();
        m_descriptorLayoutMap.clear();
    }

    VPAError ShaderAnalytics::LoadShaders(const QString& vert, const QString& frag, const QString& tesc, const QString& tese, const QString& geom) {
        Destroy();
        if (vert == "") {
            return VPA_WARN("Vertex shader required");
        }
        else {
            VPA_PASS_ERROR(CreateModule(ShaderStage::Vertex, vert));
        }

        if ((tesc != "" || tese != "") && !(tesc != "" && tese != "")) return VPA_WARN("Missing either tess control or tess eval shaders.");

        if (frag != "") { VPA_PASS_ERROR(CreateModule(ShaderStage::Fragment, frag)); }
        if (tesc != "") { VPA_PASS_ERROR(CreateModule(ShaderStage::TessellationControl, tesc)); }
        if (tese != "") { VPA_PASS_ERROR(CreateModule(ShaderStage::TessellationEvaluation, tese)); }
        if (geom != "") { VPA_PASS_ERROR(CreateModule(ShaderStage::Geometry, geom)); }

        BuildPushConstants();
        BuildInputAttributes();
        return BuildDescriptorLayoutMap();
    }

    bool ShaderAnalytics::GetStageCreateInfo(ShaderStage stage, VkPipelineShaderStageCreateInfo& createInfo) {
        if (m_modules[size_t(stage)] != VK_NULL_HANDLE) {
            createInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                           StageToVkStageFlag(stage), m_modules[size_t(stage)], "main", nullptr };
            return true;
        }
        else {
            return false;
        }
    }

    VkShaderModule ShaderAnalytics::GetModule(ShaderStage stage) {
        return m_modules[size_t(stage)];
    }

    void ShaderAnalytics::SetShader(ShaderStage stage, const QString& name) {
        if (m_modules[size_t(stage)] != VK_NULL_HANDLE) m_deviceFuncs->vkDestroyShaderModule(m_device, m_modules[size_t(stage)], nullptr);
        CreateModule(stage, name);
    }

    size_t ShaderAnalytics::NumColourAttachments() const {
        return m_modules[size_t(ShaderStage::Fragment)] != VK_NULL_HANDLE ? m_resources[size_t(ShaderStage::Fragment)].stage_outputs.size() : 0;
    }

    VPAError ShaderAnalytics::CreateModule(ShaderStage stage, const QString& name) {
        QFile file(QCoreApplication::applicationDirPath() + name);
        if (!file.open(QIODevice::ReadOnly)) {
            m_modules[size_t(stage)] = VK_NULL_HANDLE;
            return VPA_WARN("Failed to read shader " + name);
        }

        QByteArray blob = file.readAll();
        file.close();

        VkShaderModuleCreateInfo shaderInfo;
        memset(&shaderInfo, 0, sizeof(shaderInfo));
        shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderInfo.codeSize = size_t(blob.size());
        shaderInfo.pCode = reinterpret_cast<const uint32_t*>(blob.constData());
        VkShaderModule shaderModule = VK_NULL_HANDLE;
        VPA_VKCRITICAL_PASS(m_deviceFuncs->vkCreateShaderModule(m_device, &shaderInfo, nullptr, &shaderModule), "shader module creation")

        if (m_compilers[size_t(stage)] != nullptr) {
            delete m_compilers[size_t(stage)];
        }

        m_modules[size_t(stage)] = shaderModule;
        std::vector<uint32_t> spirvCrossBuf((size_t(blob.size()) / (sizeof(uint32_t) / sizeof(unsigned char))));
        memcpy(spirvCrossBuf.data(), blob.data(), size_t(blob.size()));
        m_compilers[size_t(stage)] = new Compiler(std::move(spirvCrossBuf));
        m_resources[size_t(stage)] = m_compilers[size_t(stage)]->get_shader_resources();

        m_config->writablePipelineConfig.shaderBlobs[size_t(stage)] = blob;
        return VPA_OK;
    }

    size_t ShaderAnalytics::GetComponentSize(const SPIRType& spirType) {
        if (spirType.basetype == SPIRType::Boolean || spirType.basetype == SPIRType::Int || spirType.basetype == SPIRType::UInt ||
                spirType.basetype == SPIRType::Float) {
            return sizeof(uint32_t);
        }
        else if(spirType.basetype == SPIRType::Int64 || spirType.basetype == SPIRType::UInt64 || spirType.basetype == SPIRType::Double) {
            return sizeof(uint64_t);
        }
        else if(spirType.basetype == SPIRType::Short || spirType.basetype == SPIRType::UShort || spirType.basetype == SPIRType::Half) {
            return sizeof(uint16_t);
        }
        else if (spirType.basetype == SPIRType::SByte || spirType.basetype == SPIRType::UByte) {
            return sizeof(uint8_t);
        }
        else {
            return 0; // For Unknown, Void, AtomicCounter, Sampler, AccelerationStructureNV
        }
    }

    SpvType* ShaderAnalytics::CreateVectorType(const SPIRType& spirType) {
        SpvVectorType* type = new SpvVectorType();
        type->length = spirType.vecsize;
        type->size = GetComponentSize(spirType) * spirType.vecsize;
        return type;
    }

    SpvType* ShaderAnalytics::CreateMatrixType(const SPIRType& spirType) {
        SpvMatrixType* type = new SpvMatrixType();
        type->columns = spirType.columns;
        type->rows = spirType.vecsize;
        type->size = GetComponentSize(spirType) * type->columns * type->rows;
        return type;
    }

    SpvType* ShaderAnalytics::CreateImageType(const SPIRType& spirType) {
        SpvImageType* type = new SpvImageType();
        type->size = 0;
        type->imageTypename = spirType.image.dim == spv::Dim1D ? SpvImageTypeName::Tex1D :
                                               spirType.image.dim == spv::Dim2D ? SpvImageTypeName::Tex2D :
                                               spirType.image.dim == spv::Dim3D ? SpvImageTypeName::Tex3D :
                                               spirType.image.dim == spv::DimCube ? SpvImageTypeName::TexCube :
                                               SpvImageTypeName::TexUnknown;
        type->isDepth = spirType.image.depth;
        type->isArrayed = spirType.image.arrayed;
        type->sampled = spirType.image.sampled == 1;
        type->multisampled = spirType.image.ms;
        type->format = VkFormat::VK_FORMAT_UNDEFINED; // TODO conversions from spirv cross image format to VkFormat
        return type;
    }

    SpvType* ShaderAnalytics::CreateArrayType(spirv_cross::Compiler* compiler, const SPIRType& spirType) {
        SpvArrayType* type = new SpvArrayType();
        type->lengths.resize(int(spirType.array.size()));
        type->lengthsUnsized.resize(int(spirType.array.size()));
        memset(type->lengthsUnsized.data(), false, size_t(type->lengthsUnsized.size()) * sizeof(bool));
        for (size_t i = 0; i < spirType.array.size(); ++i) {
            size_t iReversed = spirType.array.size() - (i+1);
            if (spirType.array_size_literal[iReversed]) {
                type->lengths[int(i)] = spirType.array[iReversed];
                if (type->lengths[int(i)] == 0) {
                    type->lengths[int(i)] = 1; // Unsized arrays defaulted to length 1
                    type->lengthsUnsized[int(i)] = true;
                }
            }
            else {
                type->lengths[int(i)] = 1; // Specialisation constant arrays set as 1
            }
        }
        type->subtype = CreateType(compiler, spirType, true);
        type->size = type->subtype->size;
        for (int i = 0; i < type->lengths.size(); ++i){
            type->size *= type->lengths[i];
        }

        return type;
    }

    SpvType* ShaderAnalytics::CreateStructType(spirv_cross::Compiler* compiler, const SPIRType& spirType) {
        SpvStructType* type = new SpvStructType();
        type->size = compiler->get_declared_struct_size(spirType);

        uint32_t memberCount = uint32_t(spirType.member_types.size());
        type->members.resize(int(memberCount));
        type->memberOffsets.resize(int(memberCount));
        for (uint32_t i = 0; i < memberCount; ++i) {
            type->members[int(i)] = CreateType(compiler, compiler->get_type(spirType.member_types[i]));
            type->members[int(i)]->name = QString::fromStdString(compiler->get_member_name(spirType.self, i));
            type->members[int(i)]->size = compiler->get_declared_struct_member_size(spirType, i);
            type->memberOffsets[int(i)] = compiler->type_struct_member_offset(spirType, i);
        }
        return type;
    }

    SpvType* ShaderAnalytics::CreateType(spirv_cross::Compiler* compiler, spirv_cross::Resource& res) {
        return CreateType(compiler, compiler->get_type(res.base_type_id));
    }

    SpvType* ShaderAnalytics::CreateType(spirv_cross::Compiler* compiler, const spirv_cross::SPIRType& spirType, bool ignoreArray) {
        SpvType* type;
        if (!spirType.array.empty() && !ignoreArray) {
            type = CreateArrayType(compiler, spirType);
        }
        else if (spirType.basetype == SPIRType::Image || spirType.basetype == SPIRType::SampledImage) {
            type = CreateImageType(spirType);
        }
        else if (spirType.basetype == SPIRType::Struct) {
            type = CreateStructType(compiler, spirType);
        }
        else if (spirType.columns > 1) {
            type = CreateMatrixType(spirType);
        }
        else {
            type = CreateVectorType(spirType);
        }
        type->name = "";
        return type;
    }

    void ShaderAnalytics::BuildPushConstants() {
        for (size_t i = 0; i < size_t(ShaderStage::Count_); ++i) {
            if (m_modules[i] != VK_NULL_HANDLE) {
                SmallVector<Resource>& pushConstantsBuffers = m_resources[i].push_constant_buffers;
                for (size_t j = 0; j < pushConstantsBuffers.size(); ++j) {
                    SpvResource* resource = new SpvResource();
                    resource->name = QString::fromStdString(pushConstantsBuffers[j].name);
                    resource->group = new SpvPushConstantGroup(ShaderStage(i));
                    resource->type = CreateType(m_compilers[i], pushConstantsBuffers[j]);
                    m_pushConstants.push_back(resource);
                }
            }
        }
    }

    void ShaderAnalytics::BuildInputAttributes() {
        size_t vertexIdx = size_t(ShaderStage::Vertex);
        SmallVector<Resource>& stageInputs = m_resources[vertexIdx].stage_inputs;
        m_inputAttributes.resize(int(stageInputs.size()));
        for (size_t i = 0; i < stageInputs.size(); ++i) {
            m_inputAttributes[int(i)] = new SpvResource();
            m_inputAttributes[int(i)]->name = QString::fromStdString(stageInputs[i].name);
            m_inputAttributes[int(i)]->group = new SpvInputAttribGroup(m_compilers[vertexIdx]->get_decoration(stageInputs[i].id, spv::DecorationLocation));
            m_inputAttributes[int(i)]->type = CreateType(m_compilers[vertexIdx], stageInputs[i]);
        }
    }

    VPAError ShaderAnalytics::BuildDescriptorLayoutMap() {
        for (size_t i = 0; i < size_t(ShaderStage::Count_); ++i) {
            if (m_modules[i] != VK_NULL_HANDLE) {
                // Use binding and set as key and add to map. If key exists and the resources are different then the shaders are not compatible.
                QVector<SmallVector<Resource>> descriptorResources = { m_resources[i].sampled_images, m_resources[i].storage_images,
                                                                       m_resources[i].storage_buffers, m_resources[i].uniform_buffers };
                QVector<SpvGroupName> groups = { SpvGroupName::Image, SpvGroupName::Image,
                                                 SpvGroupName::StorageBuffer, SpvGroupName::UniformBuffer };

                for (int k = 0 ; k < descriptorResources.size(); ++k) {
                    for (auto& resource : descriptorResources[k]) {
                        uint32_t set = m_compilers[i]->get_decoration(resource.id, spv::DecorationDescriptorSet);
                        uint32_t binding = m_compilers[i]->get_decoration(resource.id, spv::DecorationBinding);
                        SpvResource* res = new SpvResource();
                        res->name = QString::fromStdString(m_compilers[i]->get_name(resource.id));
                        if (res->name == "") res->name = QString::fromStdString(m_compilers[i]->get_name(resource.base_type_id));
                        res->group = new SpvDescriptorGroup(set, binding, StageToVkStageFlag(ShaderStage(i)), groups[k]);
                        res->type = CreateType(m_compilers[i], resource);

                        auto key = QPair<uint32_t, uint32_t>(set, binding);
                        if (m_descriptorLayoutMap.contains(key)) {
                            if (res != m_descriptorLayoutMap[key]) {
                                delete res;
                                return VPA_WARN("Duplicate set and binding found, but with different values. Shaders are not compatible.");
                            }
                            else {
                                reinterpret_cast<SpvDescriptorGroup*>(m_descriptorLayoutMap[key]->group)->AddStageFlag(StageToVkStageFlag(ShaderStage(i)));
                                delete res;
                            }
                        }
                        else {
                            m_descriptorLayoutMap.insert(key, res);
                        }
                    }
                }
            }
        }
        return VPA_OK;
    }
}

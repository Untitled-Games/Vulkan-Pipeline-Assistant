#include "shaderanalytics.h"

#include <QFile>
#include <QCoreApplication>

#include "pipelineconfig.h"
#include "vulkanmain.h"

using namespace vpa;
using namespace SPIRV_CROSS_NAMESPACE;

const QString vpa::ShaderStageStrings[size_t(ShaderStage::count_)] = { "Vertex", "Fragment", "TessControl", "TessEval", "Geometry" };
const QString vpa::SpvGroupnameStrings[size_t(SpvGroupname::count_)] = { "InputAttribute", "UniformBuffer", "StorageBuffer", "PushConstant", "Image" };
const QString vpa::SpvTypenameStrings[size_t(SpvTypename::count_)] = { "Image", "Array", "Vector", "Matrix", "Struct" };
const QString vpa::SpvImageTypenameStrings[size_t(SpvImageTypename::count_)] = { "Texture1D", "Texture2D", "Texture3D", "TextureCube", "UnknownTexture" };

enum class SpvTypename {
    IMAGE, ARRAY, VECTOR, MATRIX, STRUCT, count_
};

enum class SpvImageTypename {
    TEX1D, TEX2D, TEX3D, TEX_CUBE, TEX_UNKNOWN, count_ //, TEX_BUFFER, TEX_RECT. Note these are not handled yet
};
ShaderAnalytics::ShaderAnalytics(QVulkanDeviceFunctions* deviceFuncs, VkDevice device, PipelineConfig* config)
    : m_deviceFuncs(deviceFuncs), m_device(device), m_config(config) {
    memset(m_modules, VK_NULL_HANDLE, sizeof(m_modules));
    memset(m_compilers, NULL, sizeof(m_compilers));
}

ShaderAnalytics::~ShaderAnalytics() {
    Destroy();
}

void ShaderAnalytics::Destroy() {
    for (size_t i = 0; i < size_t(ShaderStage::count_); ++i) {
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
}

void ShaderAnalytics::LoadShaders(const QString& vert, const QString& frag, const QString& tesc, const QString& tese, const QString& geom) {
    Destroy();
    if (vert == "") {
        qWarning("No vertex shader provided, switch to default"); // TODO do it maybe
    }
    else {
        CreateModule(ShaderStage::VERTEX, vert);
    }

    if (tesc != "" || tese != "") {
        assert(tesc != "" && tese != "");
    }

    if (frag != "") CreateModule(ShaderStage::FRAGMENT, frag);
    if (tesc != "") CreateModule(ShaderStage::TESS_CONTROL, tesc);
    if (tese != "") CreateModule(ShaderStage::TESS_EVAL, tese);
    if (geom != "") CreateModule(ShaderStage::GEOMETRY, geom);

    BuildPushConstants();
    BuildInputAttributes();
    BuildDescriptorLayoutMap();
    qDebug("Loaded shaders.");

#ifdef QT_DEBUG
    for (SpvResource* resource : m_inputAttributes) {
        qDebug() << resource;
    }
    for (SpvResource* resource : m_pushConstants) {
        qDebug() << resource;
    }
    for (SpvResource* resource : m_descriptorLayoutMap.values()) {
        qDebug() << resource;
    }
#endif
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
    return m_modules[size_t(ShaderStage::FRAGMENT)] != VK_NULL_HANDLE ? m_resources[size_t(ShaderStage::FRAGMENT)].stage_outputs.size() : 0;
}

void ShaderAnalytics::CreateModule(ShaderStage stage, const QString& name) {
    QFile file(QCoreApplication::applicationDirPath() + name);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to read shader %s", qPrintable(name));
        m_modules[size_t(stage)] = VK_NULL_HANDLE;
        return;
    }

    QByteArray blob = file.readAll();
    file.close();

    VkShaderModuleCreateInfo shaderInfo;
    memset(&shaderInfo, 0, sizeof(shaderInfo));
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = blob.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t*>(blob.constData());
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    WARNING_VKRESULT(m_deviceFuncs->vkCreateShaderModule(m_device, &shaderInfo, nullptr, &shaderModule), "shader module creation");

    if (m_compilers[size_t(stage)] != nullptr) {
        delete m_compilers[size_t(stage)];
    }

    m_modules[size_t(stage)] = shaderModule;
    std::vector<uint32_t> spirvCrossBuf((blob.size() / (sizeof(uint32_t) / sizeof(unsigned char))));
    memcpy(spirvCrossBuf.data(), blob.data(), blob.size());
    m_compilers[size_t(stage)] = new Compiler(std::move(spirvCrossBuf));
    m_resources[size_t(stage)] = m_compilers[size_t(stage)]->get_shader_resources();

    m_config->writablePipelineConfig.shaderBlobs[size_t(stage)] = blob;
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
    type->imageTypename = spirType.image.dim == spv::Dim1D ? SpvImageTypename::TEX1D :
                                           spirType.image.dim == spv::Dim2D ? SpvImageTypename::TEX2D :
                                           spirType.image.dim == spv::Dim3D ? SpvImageTypename::TEX3D :
                                           spirType.image.dim == spv::DimCube ? SpvImageTypename::TEX_CUBE :
                                           SpvImageTypename::TEX_UNKNOWN;
    type->isDepth = spirType.image.depth;
    type->isArrayed = spirType.image.arrayed;
    type->sampled = spirType.image.sampled == 1;
    type->multisampled = spirType.image.ms;
    type->format = VkFormat::VK_FORMAT_UNDEFINED; // TODO conversions from spirv cross image format to VkFormat
    return type;
}

SpvType* ShaderAnalytics::CreateArrayType(spirv_cross::Compiler* compiler, const SPIRType& spirType) {
    SpvArrayType* type = new SpvArrayType();
    type->lengths.resize(spirType.array.size());
    for (size_t i = 0; i < spirType.array.size(); ++i) {
        if (spirType.array_size_literal[i]) { // TODO test unsized arrays
            type->lengths[i] = spirType.array[i];
        }
        else {
            type->lengths[i] = 1; // Unsized and specialisation constant arrays set as 1
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
        type->members[i] = CreateType(compiler, compiler->get_type(spirType.member_types[i]));
        type->members[i]->name = QString::fromStdString(compiler->get_member_name(spirType.self, i));
        type->members[i]->size = compiler->get_declared_struct_member_size(spirType, i);
        type->memberOffsets[i] = compiler->type_struct_member_offset(spirType, i);
    } // TODO test structs of structs
    return type;
}

SpvType* ShaderAnalytics::CreateType(spirv_cross::Compiler* compiler, spirv_cross::Resource& res) {
    return CreateType(compiler, compiler->get_type(res.base_type_id));
}

SpvType* ShaderAnalytics::CreateType(spirv_cross::Compiler* compiler, const spirv_cross::SPIRType& spirType, bool ignoreArray) {
    SpvType* type;
    if (spirType.basetype == SPIRType::Image || spirType.basetype == SPIRType::SampledImage) {
        type = CreateImageType(spirType);
    }
    else if (spirType.basetype == SPIRType::Struct) {
        type = CreateStructType(compiler, spirType);
    }
    else if (!spirType.array.empty() && !ignoreArray) {
        type = CreateArrayType(compiler, spirType);
    }
    else if (spirType.columns > 1) {
        type = CreateMatrixType(spirType);
    }
    else {
        type = CreateVectorType(spirType);
    }
    // TODO test, is it possible to have columns = 0? e.g. with just an attrib { float f }
    type->name = "";
    return type;
}

void ShaderAnalytics::BuildPushConstants() {
    for (size_t i = 0; i < size_t(ShaderStage::count_); ++i) {
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
    size_t vertexIdx = size_t(ShaderStage::VERTEX);
    SmallVector<Resource>& stageInputs = m_resources[vertexIdx].stage_inputs;
    m_inputAttributes.resize(int(stageInputs.size()));
    for (size_t i = 0; i < stageInputs.size(); ++i) {
        m_inputAttributes[i] = new SpvResource();
        m_inputAttributes[i]->name = QString::fromStdString(stageInputs[i].name);
        m_inputAttributes[i]->group = new SpvInputAttribGroup(m_compilers[vertexIdx]->get_decoration(stageInputs[i].id, spv::DecorationLocation));
        m_inputAttributes[i]->type = CreateType(m_compilers[vertexIdx], stageInputs[i]); // TODO need something to calculate the type and put it here
    }
}

void ShaderAnalytics::BuildDescriptorLayoutMap() {
    for (size_t i = 0; i < size_t(ShaderStage::count_); ++i) {
        if (m_modules[i] != VK_NULL_HANDLE) {
            // Use binding and set as key and add to map. If key exists and the resources are different then the shaders are not compatible.
            QVector<SmallVector<Resource>> descriptorResources = { m_resources[i].sampled_images, m_resources[i].storage_images,
                                                                   m_resources[i].storage_buffers, m_resources[i].uniform_buffers };
            QVector<SpvGroupname> groups = { SpvGroupname::IMAGE, SpvGroupname::IMAGE,
                                             SpvGroupname::STORAGE_BUFFER, SpvGroupname::UNIFORM_BUFFER };

            for (int k = 0 ; k < descriptorResources.size(); ++k) {
                for (auto& resource : descriptorResources[k]) {
                    uint32_t set = m_compilers[i]->get_decoration(resource.id, spv::DecorationDescriptorSet);
                    uint32_t binding = m_compilers[i]->get_decoration(resource.id, spv::DecorationBinding);
                    SpvResource* res = new SpvResource();
                    res->name = QString::fromStdString(m_compilers[i]->get_name(resource.id));
                    res->group = new SpvDescriptorGroup(set, binding, StageToVkStageFlag(ShaderStage(i)), groups[k]);
                    res->type = CreateType(m_compilers[i], resource);

                    auto key = QPair<uint32_t, uint32_t>(set, binding);
                    if (m_descriptorLayoutMap.contains(key)) {
                        if (res != m_descriptorLayoutMap[key]) {
                            qFatal("Duplicate set and binding found, but with different values. Shaders are not compatible.");
                        }
                        else {
                            ((SpvDescriptorGroup*)m_descriptorLayoutMap[key]->group)->AddStageFlag(StageToVkStageFlag(ShaderStage(i)));
                        }
                        delete res;
                    }
                    else {
                        m_descriptorLayoutMap.insert(key, res);
                    }
                }
            }
        }
    }
}

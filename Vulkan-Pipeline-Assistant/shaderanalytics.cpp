#include "shaderanalytics.h"

#include <QFile>
#include <QCoreApplication>

#include "PipelineConfig.h"
#include "vulkanmain.h"

using namespace vpa;
using namespace SPIRV_CROSS_NAMESPACE;

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
}

void ShaderAnalytics::LoadShaders(const QString& vert, const QString& frag, const QString& tesc, const QString& tese, const QString& geom) {
    Destroy();
    if (vert == "") {
        qWarning("No vertex shader provided, switch to default"); // TODO do it maybe
    }
    else {
        CreateModule(ShaderStage::VETREX, vert);
    }

    if (tesc != "" || tese != "") {
        assert(tesc != "" && tese != "");
    }

    if (frag != "") CreateModule(ShaderStage::FRAGMENT, frag);
    if (tesc != "") CreateModule(ShaderStage::TESS_CONTROL, tesc);
    if (tese != "") CreateModule(ShaderStage::TESS_EVAL, tese);
    if (geom != "") CreateModule(ShaderStage::GEOMETRY, geom);

    AnalyseDescriptorLayout();
    qDebug("Loaded shaders.");
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

QVector<SpirvResource> ShaderAnalytics::InputAttributes() {
    size_t vertexIdx = size_t(ShaderStage::VETREX);
    SmallVector<Resource>& stageInputs = m_resources[vertexIdx].stage_inputs;
    QVector<SpirvResource> attribs(stageInputs.size());
    for (int i = 0; i < stageInputs.size(); ++i) {
        attribs[i].name = QString::fromStdString(stageInputs[i].name);
        attribs[i].resourceType = SpirvResourceType::INPUT_ATTRIBUTE;
        attribs[i].spirvResource = &stageInputs[i];
        attribs[i].compiler = m_compilers[vertexIdx];
        attribs[i].size = CalculateResourceSize(attribs[i].compiler, attribs[i].spirvResource);
    }
    return attribs;
}

size_t ShaderAnalytics::NumColourAttachments() {
    return m_modules[size_t(ShaderStage::FRAGMENT)] != VK_NULL_HANDLE ? m_resources[size_t(ShaderStage::FRAGMENT)].stage_outputs.size() : 0;
}

DescriptorLayoutMap& ShaderAnalytics::DescriptorLayoutMap() {
    return m_descriptorLayoutMap;
}

QVector<SpirvResource> ShaderAnalytics::PushConstantRange(ShaderStage stage) {
    if (m_modules[size_t(stage)] != VK_NULL_HANDLE) {
        size_t stageIdx = size_t(stage);
        SmallVector<Resource>& pushConstantsBuffers = m_resources[stageIdx].push_constant_buffers;
        QVector<SpirvResource> resources(pushConstantsBuffers.size());
        for (size_t i = 0; i < pushConstantsBuffers.size(); ++i) {
            resources[i].name = QString::fromStdString(pushConstantsBuffers[i].name);
            resources[i].resourceType = SpirvResourceType::PUSH_CONSTANT;
            resources[i].spirvResource = &pushConstantsBuffers[i];
            resources[i].compiler = m_compilers[stageIdx];
            resources[i].size = CalculateResourceSize(resources[i].compiler, resources[i].spirvResource);
        }
        return resources;
    }
    else {
        return { };
    }
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
    shaderInfo.pCode = reinterpret_cast<const uint32_t *>(blob.constData());
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

VkShaderStageFlagBits ShaderAnalytics::StageToVkStageFlag(ShaderStage stage) {
    switch (stage) {
    case ShaderStage::VETREX:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::FRAGMENT:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderStage::TESS_CONTROL:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case ShaderStage::TESS_EVAL:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case ShaderStage::GEOMETRY:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    default:
        assert(false);
    }
}

size_t ShaderAnalytics::CalculateResourceSize(Compiler* compiler, Resource* res) {
    SPIRType type = compiler->get_type(res->base_type_id);
    if (type.basetype == SPIRType::Struct) {
        return compiler->get_declared_struct_size(type);
    }
    else {
        size_t size = 0;
        if (type.basetype == SPIRType::Boolean || type.basetype == SPIRType::Int || type.basetype == SPIRType::UInt ||
                type.basetype == SPIRType::Float) {
            size = sizeof(uint32_t);
        }
        else if(type.basetype == SPIRType::Int64 || type.basetype == SPIRType::UInt64 || type.basetype == SPIRType::Double) {
            size = sizeof(uint64_t);
        }
        else if(type.basetype == SPIRType::Short || type.basetype == SPIRType::UShort || type.basetype == SPIRType::Half) {
            size = sizeof(uint16_t);
        }
        else {
            return 0; // For Unknown, Void, SByte, UByte, AtomicCounter, Image, SampledImage, Sampler, AccelerationStructureNV
        }
        size *= type.vecsize;
        size *= type.columns;
        //size *= type.width;
        for (size_t i = 0; i < type.array.size(); ++i) {
            if (type.array_size_literal[i]) size *= type.array[i];
            else qWarning("Unsized array, size is unknown and shader cannot be used.");
        }
        return size;
    }
}

void ShaderAnalytics::AnalyseDescriptorLayout() {
    for (size_t i = 0; i < size_t(ShaderStage::count_); ++i) {
        if (m_modules[i] != VK_NULL_HANDLE) {
            // Use binding and set as key and add to map. If key exists and the resources are different then the shaders are not compatible.
            QVector<SmallVector<Resource>> descriptorResources = { m_resources[i].sampled_images, m_resources[i].storage_images,
                                                                   m_resources[i].storage_buffers, m_resources[i].uniform_buffers };
            QVector<SpirvResourceType> types = { SpirvResourceType::SAMPLER_IMAGE, SpirvResourceType::STORAGE_IMAGE,
                                                 SpirvResourceType::STORAGE_BUFFER, SpirvResourceType::UNIFORM_BUFFER };
            for (size_t k = 0 ; k < descriptorResources.size(); ++k) {
                for (auto& resource : descriptorResources[k]) {
                    uint32_t set = m_compilers[i]->get_decoration(resource.id, spv::DecorationDescriptorSet);
                    uint32_t binding = m_compilers[i]->get_decoration(resource.id, spv::DecorationBinding);
                    SpirvResource res = { QString::fromStdString(m_compilers[i]->get_name(resource.id)), CalculateResourceSize(m_compilers[i], &resource), types[k],
                                          &resource, m_compilers[i] };
                    auto key = QPair<uint32_t, uint32_t>(set, binding);
                    if (m_descriptorLayoutMap.contains(key) && res != m_descriptorLayoutMap[key]) {
                        qWarning("Duplicate set and binding found, but with different values. Shaders are not compatible.");
                    }
                    else {
                        m_descriptorLayoutMap[key] = res;
                    }
                }
            }
        }
    }
}

#include "shaderanalytics.h"

#include <QFile>
#include <QDir>
#include <QProcess>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QRegularExpression>

#include "pipelineconfig.h"
#include "common.h"

using SPIRV_CROSS_NAMESPACE::SPIRType;
using SPIRV_CROSS_NAMESPACE::Compiler;
using SPIRV_CROSS_NAMESPACE::SmallVector;
using SPIRV_CROSS_NAMESPACE::Resource;

namespace vpa {
    const QString ShaderStageStrings[size_t(ShaderStage::Count_)] = { "Vertex", "TessControl", "TessEval", "Geometry", "Fragment" };
    const QString SpvGroupNameStrings[size_t(SpvGroupName::Count_)] = { "InputAttribute", "UniformBuffer", "StorageBuffer", "PushConstant", "Image" };
    const QString SpvTypeNameStrings[size_t(SpvTypeName::Count_)] = { "Image", "Array", "Vector", "Matrix", "Struct" };
    const QString SpvImageTypeNameStrings[size_t(SpvImageTypeName::Count_)] = { "Texture1D", "Texture2D", "Texture3D", "TextureCube", "UnknownTexture" };

    bool operator==(const spirv_cross::SPIRType& t0, const spirv_cross::SPIRType& t1) {
        bool base = t0.basetype == t1.basetype;
        bool cols = t0.columns == t1.columns;
        bool vec = t0.vecsize == t1.vecsize;
        bool arr = t0.array.size() == t1.array.size();
        if (arr) {
            for (size_t i = 0; i < t0.array.size(); ++i) {
                arr = arr && t0.array[i] == t1.array[i];
            }
        }
        return base && cols && vec && arr;
    }

    bool operator!=(const spirv_cross::SPIRType& t0, const spirv_cross::SPIRType& t1) {
        return !operator==(t0, t1);
    }

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
            DESTROY_HANDLE(m_device, m_modules[i], m_deviceFuncs->vkDestroyShaderModule);
            if (m_compilers[i] != nullptr) delete m_compilers[i];
        }
        memset(m_modules, VK_NULL_HANDLE, sizeof(m_modules));
        memset(m_compilers, NULL, sizeof(m_compilers));

        for (SpvResource* resource : m_inputAttributes) {
            if (resource) delete resource;
        }
        for (SpvResource* resource : m_pushConstants) {
            if (resource)  delete resource;
        }
        for (SpvResource* resource : m_descriptorLayoutMap.values()) {
            if (resource)  delete resource;
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
            VPA_PASS_ERROR(CreateModule(ShaderStage::Vertex, SourceNameToBinaryName(vert)));
        }

        if ((tesc != "" || tese != "") && !(tesc != "" && tese != "")) return VPA_WARN("Missing either tess control or tess eval shaders.");

        if (frag != "") { VPA_PASS_ERROR(CreateModule(ShaderStage::Fragment, SourceNameToBinaryName(frag))); }
        if (tesc != "") { VPA_PASS_ERROR(CreateModule(ShaderStage::TessellationControl, SourceNameToBinaryName(tesc))); }
        if (tese != "") { VPA_PASS_ERROR(CreateModule(ShaderStage::TessellationEvaluation, SourceNameToBinaryName(tese))); }
        if (geom != "") { VPA_PASS_ERROR(CreateModule(ShaderStage::Geometry, SourceNameToBinaryName(geom))); }

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

    QStringList ShaderAnalytics::ColourAttachmentNames() const {
        if (m_modules[size_t(ShaderStage::Fragment)] == VK_NULL_HANDLE) {
            return { };
        }
        else {
            QStringList attachmentNames;
            for (const Resource& res : m_resources[size_t(ShaderStage::Fragment)].stage_outputs) {
                attachmentNames.push_back(QString::fromStdString(res.name));
            }
            return attachmentNames;
        }
    }

    VPAError ShaderAnalytics::CreateModule(VkShaderModule& module, const QString& name, QByteArray* blob) {
        QFile file(name);
        if (!file.open(QIODevice::ReadOnly)) {
            return VPA_WARN("Failed to read shader binary " + name);
        }

        *blob = file.readAll();
        file.close();

        VkShaderModuleCreateInfo shaderInfo;
        memset(&shaderInfo, 0, sizeof(shaderInfo));
        shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderInfo.codeSize = size_t(blob->size());
        shaderInfo.pCode = reinterpret_cast<const uint32_t*>(blob->constData());
        VPA_VKCRITICAL_PASS(m_deviceFuncs->vkCreateShaderModule(m_device, &shaderInfo, nullptr, &module), "shader module creation");

        return VPA_OK;
    }

    QVector<CompileError> ShaderAnalytics::TryCompile(QString& srcName, QString* outBinName, QPlainTextEdit* console) {
        QString binName = SourceNameToBinaryName(srcName);
        if (outBinName) *outBinName = binName;

        QProcess proc;
        proc.setWorkingDirectory(QDir::currentPath());
        proc.setProcessChannelMode(QProcess::MergedChannels);
        QByteArray vkSdkPath = qgetenv("VK_SDK_PATH");
        vkSdkPath.replace("\\", "/");
        proc.setProgram(vkSdkPath + "/Bin/glslc.exe");
        proc.setArguments({ "-c", srcName, "-o", binName });
        proc.start();
        proc.waitForFinished(5000);

        QString result = proc.readAllStandardOutput();
        proc.close();

        if (console) {
            if (result.size() > 0) console->appendPlainText(result);
            else console->appendPlainText("Shader compilation for " + srcName.mid(srcName.lastIndexOf('/') + 1) + " success.");
        }

        QVector<CompileError> errors;
        if (result.size() > 0) {
            QVector<QStringRef> errMessages = result.splitRef(QRegularExpression("[:\n]+"));
            for (int i = 0; i + 5 < errMessages.size(); i+=5) {
                // Example: ../Resources/Shaders/Source/vs_test.vert:10: error: 'location' : overlapping use of location 3
                CompileError err;
                // Discard file [0] Get line number [1] Discard "error" [2] Get error type [3] Get error message [4]
                err.lineNumber = errMessages[i + 1].toInt();
                err.message = errMessages[i + 2] + " " + errMessages[i + 3];
                errors.push_back(err);
            }
        }

        return errors;
    }

    // srcNames are assumed to be in order of ShaderStage, undefined behaviour if this is not the case
    QHash<ShaderStage, QVector<CompileError>> ShaderAnalytics::TryCompile(QString srcNames[size_t(ShaderStage::Count_)], VkPhysicalDeviceLimits limits, QPlainTextEdit* console) {
        QHash<ShaderStage, QVector<CompileError>> compileErrors;
        QString binNames[size_t(ShaderStage::Count_)];
        for (size_t i = 0; i < size_t(ShaderStage::Count_); ++i) {
            if (srcNames[i] != "") {
                QVector<CompileError> errs = TryCompile(srcNames[i], &binNames[i], console);
                if (errs.size() > 0) compileErrors[ShaderStage(i)].append(errs);
            }
            else {
                binNames[i] = "";
            }
        }
        if (compileErrors.size() > 0) return compileErrors;

        compileErrors[ShaderStage::Count_] = ValidateLinker(binNames, limits);

        if (console) {
            for (CompileError& err : compileErrors[ShaderStage::Count_]) {
                console->appendPlainText(err.message);
            }
            int linkErrorCount = compileErrors[ShaderStage::Count_].size();
            console->appendPlainText(linkErrorCount == 0 ? "Link successful." : QString::number(linkErrorCount) + " link errors generated.");
        }

        return compileErrors;
    }

    QVector<CompileError> ShaderAnalytics::ValidateLinker(QString binNames[size_t(ShaderStage::Count_)], VkPhysicalDeviceLimits limits) {
        // Get spirv-cross compilers for each shader binary
        QVector<CompileError> linkErrors;

        if (binNames[size_t(ShaderStage::Vertex)] == "") {
            linkErrors.push_back({ CompileError::LinkerErrorValue, "No vertex stage found, but is required." });
            return linkErrors;
        }

        Compiler* compilers[size_t(ShaderStage::Count_)];
        memset(compilers, NULL, sizeof(compilers));
        size_t shaderCount = 0;
        for (size_t i = 0; i < size_t(ShaderStage::Count_); ++i) {
            if (binNames[i] != "") {
                QFile file(binNames[i]);
                if (!file.open(QIODevice::ReadOnly)) {
                    linkErrors.push_back({ CompileError::LinkerErrorValue, "Failed to open file for linking (stage " + ShaderStageStrings[i] + ")" });
                    continue;
                }
                QByteArray blob = file.readAll();
                file.close();

                std::vector<uint32_t> spirvCrossBuf((size_t(blob.size()) / (sizeof(uint32_t) / sizeof(unsigned char))));
                memcpy(spirvCrossBuf.data(), blob.data(), size_t(blob.size()));
                compilers[i] = new Compiler(std::move(spirvCrossBuf));

                ++shaderCount;
            }
        }

        // Validate stages in general TODO check geom, tesc, and tese against limits
        if (compilers[size_t(ShaderStage::Vertex)]->get_shader_resources().stage_inputs.size() > limits.maxVertexInputAttributes) {
            linkErrors.push_back({ CompileError::LinkerErrorValue, "Vertex stage input attribute count exceeds device limits " + QString::number(limits.maxVertexInputAttributes) });
        }
        if (shaderCount == 1) {
            for (size_t i = 0; i < size_t(ShaderStage::Count_); ++i) {
                delete compilers[i];
            }
            return linkErrors; // Early exit when only vertex shader is used
        }

        if ((compilers[size_t(ShaderStage::TessellationControl)] != nullptr) != (compilers[size_t(ShaderStage::TessellationEvaluation)] != nullptr)) {
            linkErrors.push_back({ CompileError::LinkerErrorValue, "Tessellation control or evaluation exists but the other does not." });
        }

        if (compilers[size_t(ShaderStage::Fragment)] != nullptr && compilers[size_t(ShaderStage::Fragment)]->get_shader_resources().stage_outputs.size() > limits.maxFragmentOutputAttachments) {
            linkErrors.push_back({ CompileError::LinkerErrorValue, "Fragment stage output attachment count exceeds device limits " + QString::number(limits.maxFragmentOutputAttachments) });
        }

        // Validate stage in and out blocks for linking
        for (size_t i = 0, j = i + 1; i < size_t(ShaderStage::Count_) - 1 && j < size_t(ShaderStage::Count_); ++i, ++j) {
            if (compilers[i] == nullptr) continue;
            while (compilers[j] == nullptr && j < size_t(ShaderStage::Count_)) ++j;
            if (j == size_t(ShaderStage::Count_)) break;

            const auto& outputs = compilers[i]->get_shader_resources().stage_outputs;
            const auto& inputs = compilers[j]->get_shader_resources().stage_inputs;

            if (outputs.size() != inputs.size()) {
                linkErrors.push_back({ CompileError::LinkerErrorValue, "Output count for stage " + ShaderStageStrings[i] + " mismatch with input count for stage " + ShaderStageStrings[j] });
                continue;
            }

            for (size_t outIdx = 0; outIdx < outputs.size(); ++outIdx) {
                uint32_t outLocation = compilers[i]->get_decoration(outputs[outIdx].id, spv::DecorationLocation);
                SPIRType outType = compilers[i]->get_type(outputs[outIdx].base_type_id);
                for (size_t inIdx = 0; inIdx < inputs.size(); ++inIdx) {
                    uint32_t inLocation = compilers[j]->get_decoration(inputs[inIdx].id, spv::DecorationLocation);
                    SPIRType inType = compilers[j]->get_type(inputs[inIdx].base_type_id);
                    if (outLocation == inLocation) {
                        if (outType != inType) {
                            linkErrors.push_back({ CompileError::LinkerErrorValue, "Type mismatch for location " + QString::number(outLocation) + " in stages " + ShaderStageStrings[i] + " and " + ShaderStageStrings[j] });
                        }
                        break;
                    }
                    if (inIdx == inputs.size() - 1) {
                        linkErrors.push_back({ CompileError::LinkerErrorValue, "No IN block location in stage " + ShaderStageStrings[j] + " for OUT location " + QString::number(outLocation) + " in stage " + ShaderStageStrings[outIdx] });
                    }
                }
            }
        }

        for (size_t i = 0; i < size_t(ShaderStage::Count_); ++i) {
            delete compilers[i];
        }

        return linkErrors;
    }

    QString ShaderAnalytics::SourceNameToBinaryName(const QString& srcName) {
        QString binName = SHADERDIR + srcName.mid(srcName.lastIndexOf('/') + 1);
        binName.truncate(binName.lastIndexOf('.'));
        binName += ".spv";
        return binName;
    }

    VPAError ShaderAnalytics::CreateModule(ShaderStage stage, const QString& name) {
        m_modules[size_t(stage)] = VK_NULL_HANDLE;

        QByteArray blob;
        VPA_PASS_ERROR(CreateModule(m_modules[size_t(stage)], name, &blob));

        if (m_compilers[size_t(stage)] != nullptr) {
            delete m_compilers[size_t(stage)];
        }

        std::vector<uint32_t> spirvCrossBuf((size_t(blob.size()) / (sizeof(uint32_t) / sizeof(unsigned char))));
        memcpy(spirvCrossBuf.data(), blob.data(), size_t(blob.size()));
        m_compilers[size_t(stage)] = new Compiler(std::move(spirvCrossBuf));
        m_resources[size_t(stage)] = m_compilers[size_t(stage)]->get_shader_resources();

        m_config->writables.shaderBlobs[size_t(stage)] = blob;
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

#include "PipelineConfig.h"

#include <QFile>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>

#define WRITE_FILE_LINE(x) out << x << "|\n"
#define FLOAT_DECIMAL_PLACES 8

namespace vpa {
    typedef std::vector<char, std::allocator<char>>::iterator FILE_ITERATOR;

    std::string FloatToString(const float a_value, const int n = FLOAT_DECIMAL_PLACES) {
        std::ostringstream out;
        out.precision(n);
        out << std::fixed << a_value;
        return out.str();
    }

    std::ostream& WritablePipelineConfig::WriteShaderDataToFile(std::ostream& out, const QByteArray* byteArray) const {
        out << byteArray->size() << "|";
        const char* data = byteArray->data();
        for(int i = 0; i < byteArray->size(); i++) {
            out << *data;
            ++data;
        }
        out << "|\n";
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const PipelineConfig& config) {
        qDebug("Writing file from PipelineConfig.");

        ///////////////////////////////////////////////////////
        //// START OF CONFIG
        ///////////////////////////////////////////////////////
        WRITE_FILE_LINE("VPA_CONFIG_BEGIN");

        ///////////////////////////////////////////////////////
        //// SHADER DATA
        ///////////////////////////////////////////////////////
        config.writables.WriteShaderDataToFile(out, &(config.writables.shaderBlobs[size_t(ShaderStage::Vertex)]));
        config.writables.WriteShaderDataToFile(out, &(config.writables.shaderBlobs[size_t(ShaderStage::Fragment)]));
        config.writables.WriteShaderDataToFile(out, &(config.writables.shaderBlobs[size_t(ShaderStage::TessellationControl)]));
        config.writables.WriteShaderDataToFile(out, &(config.writables.shaderBlobs[size_t(ShaderStage::TessellationEvaluation)]));
        config.writables.WriteShaderDataToFile(out, &(config.writables.shaderBlobs[size_t(ShaderStage::Geometry)]));

        ///////////////////////////////////////////////////////
        //// VERTEX INPUT
        ///////////////////////////////////////////////////////
        WRITE_FILE_LINE(config.writables.vertexBindingCount);
        WRITE_FILE_LINE(config.writables.vertexAttribCount);
        WRITE_FILE_LINE(config.writables.vertexBindingDescriptions.binding << ' '
                        << config.writables.vertexBindingDescriptions.stride << ' '
                        << config.writables.vertexBindingDescriptions.inputRate);
        WRITE_FILE_LINE(config.writables.numAttribDescriptions);
        for(int i = 0; i < config.writables.numAttribDescriptions; i++) {
            WRITE_FILE_LINE(config.writables.vertexAttribDescriptions[i].location << ' '
                            << config.writables.vertexAttribDescriptions[i].binding << ' '
                            << config.writables.vertexAttribDescriptions[i].format << ' '
                            << config.writables.vertexAttribDescriptions[i].offset);
        }
        for(int i = config.writables.numAttribDescriptions; i < 4; i++) {
            WRITE_FILE_LINE('-');
        }

        // vertex input assembly
        WRITE_FILE_LINE(config.writables.topology);
        WRITE_FILE_LINE(config.writables.primitiveRestartEnable);

        // Tesselation state
        WRITE_FILE_LINE(config.writables.patchControlPoints);

        // Rasteriser state
        WRITE_FILE_LINE(config.writables.rasterizerDiscardEnable);
        WRITE_FILE_LINE(config.writables.polygonMode);
        WRITE_FILE_LINE(FloatToString(config.writables.lineWidth));
        WRITE_FILE_LINE(config.writables.cullMode);
        WRITE_FILE_LINE(config.writables.frontFace);
        WRITE_FILE_LINE(config.writables.depthClampEnable);
        WRITE_FILE_LINE(config.writables.depthBiasEnable);
        WRITE_FILE_LINE(FloatToString(config.writables.depthBiasConstantFactor));
        WRITE_FILE_LINE(FloatToString(config.writables.depthBiasClamp));
        WRITE_FILE_LINE(FloatToString(config.writables.depthBiasSlopeFactor));

        // Multisample state
        WRITE_FILE_LINE(config.writables.msaaSamples);
        WRITE_FILE_LINE(FloatToString(config.writables.minSampleShading));

        // Depth stencil state
        WRITE_FILE_LINE(config.writables.depthTestEnable);
        WRITE_FILE_LINE(config.writables.depthWriteEnable);
        WRITE_FILE_LINE(config.writables.depthCompareOp);
        WRITE_FILE_LINE(config.writables.depthBoundsTest);
        WRITE_FILE_LINE(config.writables.stencilTestEnable);

        // Colour blend attachment states (vector)
        WRITE_FILE_LINE(config.writables.attachments.blendEnable << ' '
                       << config.writables.attachments.writeMask << ' '
                       << config.writables.attachments.alphaBlendOp << ' '
                       << config.writables.attachments.colourBlendOp << ' '
                       << config.writables.attachments.srcColourBlendFactor << ' '
                       << config.writables.attachments.dstColourBlendFactor << ' '
                       << config.writables.attachments.srcAlphaBlendFactor << ' '
                       << config.writables.attachments.dstAlphaBlendFactor);

        // Colour blend state
        WRITE_FILE_LINE(config.writables.logicOpEnable);
        WRITE_FILE_LINE(config.writables.logicOp);
        WRITE_FILE_LINE(FloatToString(config.writables.blendConstants[0]) << ' '
                       << FloatToString(config.writables.blendConstants[1]) << ' '
                       << FloatToString(config.writables.blendConstants[2]) << ' '
                       << FloatToString(config.writables.blendConstants[3]) );

        ///////////////////////////////////////////////////////
        //// END OF CONFIG
        ///////////////////////////////////////////////////////
        WRITE_FILE_LINE("VPA_CONFIG_END");
        return out;
    }

    std::string GetNextLine(FILE_ITERATOR* iterator, std::vector<char>* buffer) {
        std::vector<char>::iterator it = std::find(*iterator, (*buffer).end(), '|');
        if(it != (*buffer).end()) {
            std::string data(*iterator, it);
            *iterator = it + 1;
            return data;
        }
        qFatal("Attempted to access data past the end of the file! This has probably happened because you edited the data in the file through unsafely!");
        return "";
    }

    VPAError PipelineConfig::LoadConfiguration(std::vector<char>& buffer, const int bufferSize) {
        // Keep a reference to the current position we are reading from
        auto readPosition = buffer.begin();

        ///////////////////////////////////////////////////////
        //// START OF CONFIG
        ///////////////////////////////////////////////////////
        std::string activeData = GetNextLine(&readPosition, &buffer);
        if(!(activeData == "VPA_CONFIG_BEGIN")) return VPA_CRITICAL("VPA_CONFIG_BEGIN format invalid, missing object header.");
        try {
            ///////////////////////////////////////////////////////
            //// SHADER DATA
            ///////////////////////////////////////////////////////
            for(size_t i = 0; i < size_t(ShaderStage::Count_); i++) {
                int shaderSize = std::stoi(GetNextLine(&readPosition, &buffer));
                std::string shaderData = GetNextLine(&readPosition, &buffer);
                bool complete = false;
                while(!complete) {
                    if(shaderSize ==  shaderData.size()) {
                        complete = true;
                    }
                    else {
                        shaderData += '|';
                        if(shaderSize ==  shaderData.size()) {
                            complete = true;
                            break;
                        }
                        shaderData += GetNextLine(&readPosition, &buffer);
                    }
                }
                QByteArray blob(shaderData.data(), shaderData.size());
                writables.shaderBlobs[i] = blob;
            }

            ///////////////////////////////////////////////////////
            //// VERTEX INPUT
            ///////////////////////////////////////////////////////
            writables.vertexBindingCount = std::stoi(GetNextLine(&readPosition, &buffer));
            writables.vertexAttribCount = std::stoi(GetNextLine(&readPosition, &buffer));
            activeData = GetNextLine(&readPosition, &buffer);

            std::stringstream ss(activeData);
            std::string bufferString;
            ss >> bufferString;
            writables.vertexBindingDescriptions.binding = std::stoi(bufferString);
            ss >> bufferString;
            writables.vertexBindingDescriptions.stride = std::stoi(bufferString);
            ss >> bufferString;
            writables.vertexBindingDescriptions.inputRate = (VkVertexInputRate) std::stoi(bufferString);

            // attribute descriptions
            writables.numAttribDescriptions = std::stoi(GetNextLine(&readPosition, &buffer));
            assert(writables.numAttribDescriptions >= 0 && writables.numAttribDescriptions <= 4);
            
            // attribute descriptions
            for(int i = 0; i < writables.numAttribDescriptions; i++) {
                activeData = GetNextLine(&readPosition, &buffer);
                ss = std::stringstream(activeData);
                ss >> bufferString;
                writables.vertexAttribDescriptions[i].location = std::stoi(bufferString);
                ss >> bufferString;
                writables.vertexAttribDescriptions[i].binding = std::stoi(bufferString);
                ss >> bufferString;
                writables.vertexAttribDescriptions[i].format = (VkFormat) std::stoi(bufferString);
                ss >> bufferString;
                writables.vertexAttribDescriptions[i].offset = std::stoi(bufferString);
            }

            // read in rest of pipeline data in order to move the iterator position
            for(int i = writables.numAttribDescriptions; i < 4; i++) {
                activeData = GetNextLine(&readPosition, &buffer);
            }

            // Vertex Input Assembly
            writables.topology = (VkPrimitiveTopology) std::stoi(GetNextLine(&readPosition, &buffer));
            writables.primitiveRestartEnable = std::stoi(GetNextLine(&readPosition, &buffer));

            // Tesselation state
            writables.patchControlPoints = std::stoi(GetNextLine(&readPosition, &buffer));

            // Rasteriser state
            writables.rasterizerDiscardEnable = std::stoi(GetNextLine(&readPosition, &buffer));
            writables.polygonMode = (VkPolygonMode) std::stoi(GetNextLine(&readPosition, &buffer));
            writables.lineWidth = std::stof(GetNextLine(&readPosition, &buffer));
            writables.cullMode = (VkCullModeFlagBits) std::stoi(GetNextLine(&readPosition, &buffer));
            writables.frontFace = (VkFrontFace) std::stoi(GetNextLine(&readPosition, &buffer));
            writables.depthClampEnable = std::stoi(GetNextLine(&readPosition, &buffer));
            writables.depthBiasEnable = std::stoi(GetNextLine(&readPosition, &buffer));
            writables.depthBiasConstantFactor = std::stof(GetNextLine(&readPosition, &buffer));
            writables.depthBiasClamp = std::stof(GetNextLine(&readPosition, &buffer));
            writables.depthBiasSlopeFactor = std::stof(GetNextLine(&readPosition, &buffer));

            // Multisample state
            writables.msaaSamples = (VkSampleCountFlagBits) std::stoi(GetNextLine(&readPosition, &buffer));
            writables.minSampleShading = std::stof(GetNextLine(&readPosition, &buffer));

            // Depth stencil state
            writables.depthTestEnable = std::stoi(GetNextLine(&readPosition, &buffer));
            writables.depthWriteEnable = std::stoi(GetNextLine(&readPosition, &buffer));
            writables.depthCompareOp = (VkCompareOp) std::stoi(GetNextLine(&readPosition, &buffer));
            writables.depthBoundsTest = std::stoi(GetNextLine(&readPosition, &buffer));
            writables.stencilTestEnable = std::stoi(GetNextLine(&readPosition, &buffer));

            // Colour blend attachment states (vector)
            activeData = GetNextLine(&readPosition, &buffer);
            ss = std::stringstream(activeData);
            ss >> bufferString;
            writables.attachments.blendEnable = std::stoi(bufferString);
            ss >> bufferString;
            writables.attachments.writeMask = std::stoi(bufferString);
            ss >> bufferString;
            writables.attachments.alphaBlendOp = (VkBlendOp) std::stoi(bufferString);
            ss >> bufferString;
            writables.attachments.colourBlendOp = (VkBlendOp) std::stoi(bufferString);
            ss >> bufferString;
            writables.attachments.srcColourBlendFactor = (VkBlendFactor) std::stoi(bufferString);
            ss >> bufferString;
            writables.attachments.dstColourBlendFactor = (VkBlendFactor) std::stoi(bufferString);
            ss >> bufferString;
            writables.attachments.srcAlphaBlendFactor = (VkBlendFactor) std::stoi(bufferString);
            ss >> bufferString;
            writables.attachments.dstAlphaBlendFactor = (VkBlendFactor) std::stoi(bufferString);

            // Colour blend state
            writables.logicOpEnable = std::stoi(GetNextLine(&readPosition, &buffer));
            writables.logicOp = (VkLogicOp) std::stoi(GetNextLine(&readPosition, &buffer));

            // 1
            activeData = GetNextLine(&readPosition, &buffer);
            ss = std::stringstream(activeData);
            ss >> bufferString;
            writables.blendConstants[0] = std::stof(bufferString);
            ss >> bufferString;
            writables.blendConstants[1] = std::stof(bufferString);
            ss >> bufferString;
            writables.blendConstants[2] = std::stof(bufferString);
            ss >> bufferString;
            writables.blendConstants[3] = std::stof(bufferString);
        }
        catch (std::invalid_argument const &e) {
            return VPA_CRITICAL("Bad input: std::invalid_argument was thrown");
        }
        catch (std::out_of_range const &e) {
            return VPA_CRITICAL("Data Overflow: std::out_of_range was thrown");
        }

        activeData = GetNextLine(&readPosition, &buffer);
        if(!(activeData == "VPA_CONFIG_END" || activeData == "\rVPA_CONFIG_END")) {
            return VPA_CRITICAL("VPA_CONFIG_END format invalid, missing object footer.");
        }
        return VPA_OK;
    }

}

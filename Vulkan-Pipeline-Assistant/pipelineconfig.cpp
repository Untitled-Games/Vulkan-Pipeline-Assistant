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
        config.writablePipelineConfig.WriteShaderDataToFile(out, &(config.writablePipelineConfig.shaderBlobs[size_t(ShaderStage::VERTEX)]));
        config.writablePipelineConfig.WriteShaderDataToFile(out, &(config.writablePipelineConfig.shaderBlobs[size_t(ShaderStage::FRAGMENT)]));
        config.writablePipelineConfig.WriteShaderDataToFile(out, &(config.writablePipelineConfig.shaderBlobs[size_t(ShaderStage::TESS_CONTROL)]));
        config.writablePipelineConfig.WriteShaderDataToFile(out, &(config.writablePipelineConfig.shaderBlobs[size_t(ShaderStage::TESS_EVAL)]));
        config.writablePipelineConfig.WriteShaderDataToFile(out, &(config.writablePipelineConfig.shaderBlobs[size_t(ShaderStage::GEOMETRY)]));

        ///////////////////////////////////////////////////////
        //// VERTEX INPUT
        ///////////////////////////////////////////////////////
        WRITE_FILE_LINE(config.writablePipelineConfig.vertexBindingCount);
        WRITE_FILE_LINE(config.writablePipelineConfig.vertexAttribCount);
        WRITE_FILE_LINE(config.writablePipelineConfig.vertexBindingDescriptions.binding << ' '
                        << config.writablePipelineConfig.vertexBindingDescriptions.stride << ' '
                        << config.writablePipelineConfig.vertexBindingDescriptions.inputRate);
        WRITE_FILE_LINE(config.writablePipelineConfig.numAttribDescriptions);
        for(int i = 0; i < config.writablePipelineConfig.numAttribDescriptions; i++) {
            WRITE_FILE_LINE(config.writablePipelineConfig.vertexAttribDescriptions[i].location << ' '
                            << config.writablePipelineConfig.vertexAttribDescriptions[i].binding << ' '
                            << config.writablePipelineConfig.vertexAttribDescriptions[i].format << ' '
                            << config.writablePipelineConfig.vertexAttribDescriptions[i].offset);
        }
        for(int i = config.writablePipelineConfig.numAttribDescriptions; i < 4; i++) {
            WRITE_FILE_LINE('-');
        }

        // vertex input assembly
        WRITE_FILE_LINE(config.writablePipelineConfig.topology);
        WRITE_FILE_LINE(config.writablePipelineConfig.primitiveRestartEnable);

        // Tesselation state
        WRITE_FILE_LINE(config.writablePipelineConfig.patchControlPoints);

        // Rasteriser state
        WRITE_FILE_LINE(config.writablePipelineConfig.rasterizerDiscardEnable);
        WRITE_FILE_LINE(config.writablePipelineConfig.polygonMode);
        WRITE_FILE_LINE(FloatToString(config.writablePipelineConfig.lineWidth));
        WRITE_FILE_LINE(config.writablePipelineConfig.cullMode);
        WRITE_FILE_LINE(config.writablePipelineConfig.frontFace);
        WRITE_FILE_LINE(config.writablePipelineConfig.depthClampEnable);
        WRITE_FILE_LINE(config.writablePipelineConfig.depthBiasEnable);
        WRITE_FILE_LINE(FloatToString(config.writablePipelineConfig.depthBiasConstantFactor));
        WRITE_FILE_LINE(FloatToString(config.writablePipelineConfig.depthBiasClamp));
        WRITE_FILE_LINE(FloatToString(config.writablePipelineConfig.depthBiasSlopeFactor));

        // Multisample state
        WRITE_FILE_LINE(config.writablePipelineConfig.msaaSamples);
        WRITE_FILE_LINE(FloatToString(config.writablePipelineConfig.minSampleShading));

        // Depth stencil state
        WRITE_FILE_LINE(config.writablePipelineConfig.depthTestEnable);
        WRITE_FILE_LINE(config.writablePipelineConfig.depthWriteEnable);
        WRITE_FILE_LINE(config.writablePipelineConfig.depthCompareOp);
        WRITE_FILE_LINE(config.writablePipelineConfig.depthBoundsTest);
        WRITE_FILE_LINE(config.writablePipelineConfig.stencilTestEnable);

        // Colour blend attachment states (vector)
        WRITE_FILE_LINE(config.writablePipelineConfig.attachments.blendEnable << ' '
                       << config.writablePipelineConfig.attachments.writeMask << ' '
                       << config.writablePipelineConfig.attachments.alphaBlendOp << ' '
                       << config.writablePipelineConfig.attachments.colourBlendOp << ' '
                       << config.writablePipelineConfig.attachments.srcColourBlendFactor << ' '
                       << config.writablePipelineConfig.attachments.dstColourBlendFactor << ' '
                       << config.writablePipelineConfig.attachments.srcAlphaBlendFactor << ' '
                       << config.writablePipelineConfig.attachments.dstAlphaBlendFactor);

        // Colour blend state
        WRITE_FILE_LINE(config.writablePipelineConfig.logicOpEnable);
        WRITE_FILE_LINE(config.writablePipelineConfig.logicOp);
        WRITE_FILE_LINE(FloatToString(config.writablePipelineConfig.blendConstants[0]) << ' '
                       << FloatToString(config.writablePipelineConfig.blendConstants[1]) << ' '
                       << FloatToString(config.writablePipelineConfig.blendConstants[2]) << ' '
                       << FloatToString(config.writablePipelineConfig.blendConstants[3]) );

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
            for(size_t i = 0; i < size_t(ShaderStage::count_); i++) {
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
                writablePipelineConfig.shaderBlobs[i] = blob;
            }

            ///////////////////////////////////////////////////////
            //// VERTEX INPUT
            ///////////////////////////////////////////////////////
            writablePipelineConfig.vertexBindingCount = std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.vertexAttribCount = std::stoi(GetNextLine(&readPosition, &buffer));
            activeData = GetNextLine(&readPosition, &buffer);

            std::stringstream ss(activeData);
            std::string bufferString;
            ss >> bufferString;
            writablePipelineConfig.vertexBindingDescriptions.binding = std::stoi(bufferString);
            ss >> bufferString;
            writablePipelineConfig.vertexBindingDescriptions.stride = std::stoi(bufferString);
            ss >> bufferString;
            writablePipelineConfig.vertexBindingDescriptions.inputRate = (VkVertexInputRate) std::stoi(bufferString);

            // attribute descriptions
            writablePipelineConfig.numAttribDescriptions = std::stoi(GetNextLine(&readPosition, &buffer));
            assert(writablePipelineConfig.numAttribDescriptions >= 0 && writablePipelineConfig.numAttribDescriptions <= 4);
            
            // attribute descriptions
            for(int i = 0; i < writablePipelineConfig.numAttribDescriptions; i++) {
                activeData = GetNextLine(&readPosition, &buffer);
                ss = std::stringstream(activeData);
                ss >> bufferString;
                writablePipelineConfig.vertexAttribDescriptions[i].location = std::stoi(bufferString);
                ss >> bufferString;
                writablePipelineConfig.vertexAttribDescriptions[i].binding = std::stoi(bufferString);
                ss >> bufferString;
                writablePipelineConfig.vertexAttribDescriptions[i].format = (VkFormat) std::stoi(bufferString);
                ss >> bufferString;
                writablePipelineConfig.vertexAttribDescriptions[i].offset = std::stoi(bufferString);
            }

            // read in rest of pipeline data in order to move the iterator position
            for(int i = writablePipelineConfig.numAttribDescriptions; i < 4; i++) {
                activeData = GetNextLine(&readPosition, &buffer);
            }

            // Vertex Input Assembly
            writablePipelineConfig.topology = (VkPrimitiveTopology) std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.primitiveRestartEnable = std::stoi(GetNextLine(&readPosition, &buffer));

            // Tesselation state
            writablePipelineConfig.patchControlPoints = std::stoi(GetNextLine(&readPosition, &buffer));

            // Rasteriser state
            writablePipelineConfig.rasterizerDiscardEnable = std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.polygonMode = (VkPolygonMode) std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.lineWidth = std::stof(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.cullMode = (VkCullModeFlagBits) std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.frontFace = (VkFrontFace) std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.depthClampEnable = std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.depthBiasEnable = std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.depthBiasConstantFactor = std::stof(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.depthBiasClamp = std::stof(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.depthBiasSlopeFactor = std::stof(GetNextLine(&readPosition, &buffer));

            // Multisample state
            writablePipelineConfig.msaaSamples = (VkSampleCountFlagBits) std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.minSampleShading = std::stof(GetNextLine(&readPosition, &buffer));

            // Depth stencil state
            writablePipelineConfig.depthTestEnable = std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.depthWriteEnable = std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.depthCompareOp = (VkCompareOp) std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.depthBoundsTest = std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.stencilTestEnable = std::stoi(GetNextLine(&readPosition, &buffer));

            // Colour blend attachment states (vector)
            activeData = GetNextLine(&readPosition, &buffer);
            ss = std::stringstream(activeData);
            ss >> bufferString;
            writablePipelineConfig.attachments.blendEnable = std::stoi(bufferString);
            ss >> bufferString;
            writablePipelineConfig.attachments.writeMask = std::stoi(bufferString);
            ss >> bufferString;
            writablePipelineConfig.attachments.alphaBlendOp = (VkBlendOp) std::stoi(bufferString);
            ss >> bufferString;
            writablePipelineConfig.attachments.colourBlendOp = (VkBlendOp) std::stoi(bufferString);
            ss >> bufferString;
            writablePipelineConfig.attachments.srcColourBlendFactor = (VkBlendFactor) std::stoi(bufferString);
            ss >> bufferString;
            writablePipelineConfig.attachments.dstColourBlendFactor = (VkBlendFactor) std::stoi(bufferString);
            ss >> bufferString;
            writablePipelineConfig.attachments.srcAlphaBlendFactor = (VkBlendFactor) std::stoi(bufferString);
            ss >> bufferString;
            writablePipelineConfig.attachments.dstAlphaBlendFactor = (VkBlendFactor) std::stoi(bufferString);

            // Colour blend state
            writablePipelineConfig.logicOpEnable = std::stoi(GetNextLine(&readPosition, &buffer));
            writablePipelineConfig.logicOp = (VkLogicOp) std::stoi(GetNextLine(&readPosition, &buffer));

            // 1
            activeData = GetNextLine(&readPosition, &buffer);
            ss = std::stringstream(activeData);
            ss >> bufferString;
            writablePipelineConfig.blendConstants[0] = std::stof(bufferString);
            ss >> bufferString;
            writablePipelineConfig.blendConstants[1] = std::stof(bufferString);
            ss >> bufferString;
            writablePipelineConfig.blendConstants[2] = std::stof(bufferString);
            ss >> bufferString;
            writablePipelineConfig.blendConstants[3] = std::stof(bufferString);
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

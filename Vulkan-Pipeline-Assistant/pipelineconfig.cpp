#include "PipelineConfig.h"

#include <QFile> // used for qMessages (Remove me)

#define WRITE_FILE_LINE(x) out << x << "|\n"

#include <fstream>
#include <string>

//TODO: Move this to some utility class
#include <sstream>

template <typename T>
std::string FloatToString(const T a_value, const int n = 2)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

namespace vpa {
    std::ostream& WritablePipelineConfig::WriteShaderDataToFile(std::ostream& out, const QByteArray* byteArray) const
    {
        out << byteArray->size() << "|";
        const char* data = byteArray->data();
        for(int i = 0; i < byteArray->size(); i++)
        {
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
        config.writablePipelineConfig.WriteShaderDataToFile(out, &(config.writablePipelineConfig.vertShaderBlob));
        config.writablePipelineConfig.WriteShaderDataToFile(out, &(config.writablePipelineConfig.fragShaderBlob));
        config.writablePipelineConfig.WriteShaderDataToFile(out, &(config.writablePipelineConfig.tescShaderBlob));
        config.writablePipelineConfig.WriteShaderDataToFile(out, &(config.writablePipelineConfig.teseShaderBlob));
        config.writablePipelineConfig.WriteShaderDataToFile(out, &(config.writablePipelineConfig.geomShaderBlob));

        ///////////////////////////////////////////////////////
        //// VERTEX INPUT
        ///////////////////////////////////////////////////////
        WRITE_FILE_LINE(config.writablePipelineConfig.vertexBindingCount);
        WRITE_FILE_LINE(config.writablePipelineConfig.vertexAttribCount);
        WRITE_FILE_LINE(config.writablePipelineConfig.vertexBindingDescriptions.binding << ' '
                        << config.writablePipelineConfig.vertexBindingDescriptions.stride << ' '
                        << config.writablePipelineConfig.vertexBindingDescriptions.inputRate);
        WRITE_FILE_LINE(config.writablePipelineConfig.numAttribDescriptions);
        for(int i = 0; i < config.writablePipelineConfig.numAttribDescriptions; i++)
        {
            WRITE_FILE_LINE(config.writablePipelineConfig.vertexAttribDescriptions[i].location << ' '
                            << config.writablePipelineConfig.vertexAttribDescriptions[i].binding << ' '
                            << config.writablePipelineConfig.vertexAttribDescriptions[i].format << ' '
                            << config.writablePipelineConfig.vertexAttribDescriptions[i].offset);
        }
        for(int i = config.writablePipelineConfig.numAttribDescriptions; i < 4; i++)
        {
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



#include <algorithm>
typedef std::vector<char, std::allocator<char>>::iterator FILE_ITERATOR;

    std::string GetNextLine(FILE_ITERATOR* iterator, std::vector<char>* buffer)
    {
        std::vector<char>::iterator it = std::find(*iterator, (*buffer).end(), '|');
        std::string data(*iterator, it);
        *iterator = it + 1;
        return data;
    }

    bool PipelineConfig::LoadConfiguration(std::vector<char>& buffer, const int bufferSize)
    {
        qDebug("Buffer Size: %i", bufferSize);
        // Keep a reference to the current position we are reading from
        auto readPosition = buffer.begin();
        std::string firstLine = GetNextLine(&readPosition, &buffer);
        if(!(firstLine == "VPA_CONFIG_BEGIN"))
        {
            qCritical("VPA_CONFIG_BEGIN format invalid, missing object header.");
            return false;
        }
        try
        {
            // loop these for all shaders

            // vertex shader
            int shaderSize = std::stoi(GetNextLine(&readPosition, &buffer));
            std::string shaderData = GetNextLine(&readPosition, &buffer);
            bool complete = false;
            while(!complete)
            {
                if(shaderSize ==  shaderData.size())
                    complete = true;
                else
                {
                    shaderData += '|';
                    shaderData += GetNextLine(&readPosition, &buffer);
                }
            }

            QByteArray blob(shaderData.data(), shaderData.size());
            writablePipelineConfig.vertShaderBlob = blob;

            // fragment shader
            shaderSize = std::stoi(GetNextLine(&readPosition, &buffer));
            shaderData = GetNextLine(&readPosition, &buffer);

            complete = false;
            while(!complete)
            {
                if(shaderSize ==  shaderData.size())
                    complete = true;
                else
                {
                    shaderData += '|';
                    shaderData += GetNextLine(&readPosition, &buffer);
                }
            }
            QByteArray fragBlob(shaderData.data(), shaderData.size());
            writablePipelineConfig.fragShaderBlob = fragBlob;

        }
        catch (std::invalid_argument const &e)
        {
            qCritical("Bad input: std::invalid_argument was thrown");
        }
        catch (std::out_of_range const &e)
        {
            qCritical("Data Overflow: std::out_of_range was thrown");
        }

        // load in the rest of the data
        return true;
    }

}

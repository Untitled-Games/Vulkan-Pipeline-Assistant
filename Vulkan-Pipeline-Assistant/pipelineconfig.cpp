#include "PipelineConfig.h"

#include <QFile> // used for qMessages (Remove me)

#define WRITE_FILE_LINE(x) out << x << "\n"

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
        out << byteArray->size() << "\n";
        const char* data = byteArray->data();
        for(int i = 0; i < byteArray->size(); i++)
        {
            out << *data;
            ++data;
        }
        out << "\n";
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const PipelineConfig& config) {
        qDebug("Writing file from PipelineConfig.");

        ///////////////////////////////////////////////////////
        //// START OF CONFIG
        ///////////////////////////////////////////////////////
        out <<  "VPA_CONFIG_BEGIN\n";

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
            WRITE_FILE_LINE('0');
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
        out << "VPA_CONFIG_END\n";
        return out;
    }

    std::istream& operator>>(std::istream& in, PipelineConfig& config) {
        qDebug("Reading file from PipelineConfig.");

        std::string line = "";
        std::string lineResult = "";

        ///////////////////////////////////////////////////////
        //// START OF CONFIG
        ///////////////////////////////////////////////////////
        std::getline(in, line);
        if(line != "VPA_CONFIG_BEGIN")
        {
            qCritical("VPA_CONFIG_BEGIN format invalid, missing object header.");
            return in; //TODO: Empty the stream
        }
        static const int TEST_MULTI = 1;

        int vsbSize = 0;
        in >> vsbSize;
        //vsbSize = 96; // byte 97 kills the stream

        //TODO: Abstract into a function which reads all shaders' data.
        // Read in vertex shader data
        char* data = new char[TEST_MULTI * vsbSize];
        in.read(data, TEST_MULTI * vsbSize);

        if(in.fail())
        {
            qDebug("Stream Failed!");
        }

        QByteArray blob(data, TEST_MULTI * vsbSize);
        config.writablePipelineConfig.vertShaderBlob = blob;

        std::ofstream fstream;
        fstream.open("test.1");
        config.writablePipelineConfig.WriteShaderDataToFile(fstream, &(config.writablePipelineConfig.vertShaderBlob));
        fstream.close();

        int vertexBindingCount = 10;
        in >> vertexBindingCount;
        qDebug("%i", vertexBindingCount);


        if(line == "VPA_CONFIG_END")
        {
            return in;
        }

        return in;
    }
}

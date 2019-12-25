#include "PipelineConfig.h"

#include <QFile> // used for qMessages (Remove me)

namespace vpa {
    std::ostream& operator<<(std::ostream& out, const PipelineConfig& config) {
        qDebug("Writing file from PipelineConfig.");

        ///////////////////////////////////////////////////////
        //// START OF CONFIG
        ///////////////////////////////////////////////////////
        out <<  "VPA_CONFIG_BEGIN\n";

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

        ///////////////////////////////////////////////////////
        //// END OF CONFIG
        ///////////////////////////////////////////////////////
        std::getline(in, line);
        if(line != "VPA_CONFIG_END")
        {
            qCritical("VPA_CONFIG_END format invalid, missing object header.");
            return in; //TODO: Empty thestream
        }

        return in;
    }
}

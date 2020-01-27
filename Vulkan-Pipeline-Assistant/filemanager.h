#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

#include "common.h"

namespace vpa {

    template <typename T>
    class FileManager {
    public:
        static VPAError Writer(T& object, const std::string& filename = CONFIGDIR"config.vpa");
        static VPAError Loader(T& object, const std::string& filename = CONFIGDIR"config.vpa");
    private:
        FileManager() = default;
        ~FileManager() = default;
    };


    template<typename T>
    VPAError FileManager<T>::Writer(T& object, const std::string& filename) {
        std::ofstream fstream;
        fstream.open(filename);
        if(!fstream) return VPA_CRITICAL("Failed to open the output file");
        fstream << object;
        fstream.close();
        return VPA_OK;
    }

    template<typename T>
    VPAError FileManager<T>::Loader(T& object, const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate | std::ios_base::skipws);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer = std::vector<char>(size_t(size));
        if (file.read(buffer.data(), size)) {
            if(file.bad()) {
                file.close();
                return VPA_WARN("Input Stream Failed!");
            }
            buffer.erase(std::remove(buffer.begin(), buffer.end(), '\n'), buffer.end());
            size = std::streamsize(buffer.size());
            buffer.shrink_to_fit();
            if(object.LoadConfiguration(buffer, size).level != VPAErrorLevel::Ok) {
                file.close();
                return VPA_CRITICAL("Failed to read configuration, check the file format!");
            }
        }
        file.close();
        return VPA_OK;
    }
}
#endif // FILEMANAGER_H

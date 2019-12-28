#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <QFile> // used for qMessages (Remove me)

namespace vpa {

    template <typename T>
    class FileManager
    {
    public:
        static void Writer(T& object, const std::string& filename = "config.vpa");
        static void Loader(T& object, const std::string& filename = "config.vpa");
    private:
        FileManager() = default;
        ~FileManager() = default;
    };


    template<typename T>
    void FileManager<T>::Writer(T& object, const std::string& filename)
    {
        std::ofstream fstream;
        fstream.open(filename);
        if(!fstream)
        {
            qCritical("Failed to open the output file");
            return;
        }
        fstream << object;
        fstream.close();
    }

    template<typename T>
    void FileManager<T>::Loader(T& object, const std::string& filename)
    {
        std::ifstream file(filename, std::ios::binary | std::ios::ate | std::ios_base::skipws);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size))
        {
            if(file.bad())
            {
                qDebug("Input Stream Failed!");
                file.close();
                return;
            }
            buffer.erase(std::remove(buffer.begin(), buffer.end(), '\n'), buffer.end());
            size = buffer.size();
            buffer.shrink_to_fit();
            if(!object.LoadConfiguration(buffer, size))
            {
                qCritical("Failed to read configuration, check the file format!");
            }
        }
        file.close();        
    }
}
#endif // FILEMANAGER_H

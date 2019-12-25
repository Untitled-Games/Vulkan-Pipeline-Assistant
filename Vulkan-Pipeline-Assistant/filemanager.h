#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <fstream>

namespace vpa {


    template <typename T>
    class FileManager
    {
    public:
        static void Writer(T& object, const std::string& filename = "config.vpa");
        static void Loader(T& object, const std::string& filename = "config.vpa"); //throw (Exceptions::InvalidFileException);
    private:
        FileManager() = default;
        ~FileManager() = default;
    };

    template<typename T>
    void FileManager<T>::Loader(T& object, const std::string& filename) //throw (Exceptions::InvalidFileException)
    {
        //if (filename.empty())
            //throw Exceptions::InvalidFileException("File argument must not be empty!", __FILE__, __LINE__, __FUNCTION__);

        std::string input;
        std::ifstream ifStream;
        ifStream.open(filename);
        if (!ifStream)
        {
            const std::string msg = "Failed to open provided file [" + filename + "]";
            //throw Exceptions::InvalidFileException(msg.c_str(), __FILE__, __LINE__, __FUNCTION__);
        }

        bool isEmpty = true;
        try
        {
            while(!ifStream.eof())
            {
                isEmpty = false;
                ifStream >> object; // loads the data from the file into the specified object.
                break; //todo: remove this continue....
            }
        }catch(...)
        {
            ifStream.close();
            throw;
        }

        ifStream.close();
        //if (isEmpty)
            //throw Exceptions::InvalidFileException("Cannot read data from an empty file!", __FILE__, __LINE__, __FUNCTION__);
    }

    template<typename T>
    void FileManager<T>::Writer(T& object, const std::string& filename)
    {
        std::string input;
        std::ofstream fstream;
        fstream.open(filename);
        if(!fstream)
        {
           // std::cout << "failed to write output file\n";
            return;
        }
        fstream << object;
        fstream.close();
    }

}
#endif // FILEMANAGER_H

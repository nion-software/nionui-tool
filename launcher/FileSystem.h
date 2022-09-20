#ifndef FILESYSTEM_H
#define FILESYSTEM_H

class FileSystem
{
public:
    virtual ~FileSystem() { }
    virtual std::string absoluteFilePath(const std::string &dir, const std::string &fileName) = 0;
    virtual bool exists(const std::string &filePath) = 0;
    virtual std::string toNativeSeparators(const std::string &filePath) = 0;
    virtual bool parseConfigFile(const std::string &filePath, std::string &home, std::string &version) = 0;
    virtual void iterateDirectory(const std::string &directoryPath, const std::list<std::string> &nameFilters, std::list<std::string> &filePaths) = 0;
    virtual std::string directoryName(const std::string &filePath) = 0;
    virtual std::string directory(const std::string &filePath) = 0;
    virtual std::string parentDirectory(const std::string &filePath) = 0;
    virtual void putEnv(const std::string &key, const std::string &value) = 0;
    virtual std::string getEnv(const std::string &key) = 0;
};

#endif

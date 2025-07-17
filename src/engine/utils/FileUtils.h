#pragma once

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

class FileUtils {
public:
    static bool FileExists(const std::string& path);
    static std::string GetFileContents(const std::string& path);
    static std::string GetDirectory(const std::string& path);
    static std::string GetFileName(const std::string& path);
    static std::string GetFileExtension(const std::string& path);
    static std::string GetFileNameWithoutExtension(const std::string& path);
    static std::string GetFileSize(const std::string& path);
    static std::string GetFileLastModified(const std::string& path);
};
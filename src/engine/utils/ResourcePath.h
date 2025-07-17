#pragma once
#include <string>

class ResourcePath {
public:
    static void SetBasePath(const std::string& path);
    static std::string GetBasePath();
    static std::string GetFullPath(const std::string& relativePath);

private:
    static std::string s_BasePath;
};


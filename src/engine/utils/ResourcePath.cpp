#include "ResourcePath.h"

std::string ResourcePath::s_BasePath = "";

void ResourcePath::SetBasePath(const std::string& path) {
    s_BasePath = path;
}

std::string ResourcePath::GetBasePath() {
    return s_BasePath;
}

std::string ResourcePath::GetFullPath(const std::string& relativePath) {
    return s_BasePath + relativePath;
}


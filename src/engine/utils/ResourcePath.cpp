#include "ResourcePath.h"
#include <filesystem>
#include <algorithm>

// Initialize static member
std::string ResourcePath::s_basePath = "";

void ResourcePath::SetBasePath(const std::string& basePath) {
    s_basePath = NormalizePath(basePath);
    
    // Ensure base path ends with a separator
    if (!s_basePath.empty() && s_basePath.back() != '/' && s_basePath.back() != '\\') {
        s_basePath += '/';
    }
}

const std::string& ResourcePath::GetBasePath() {
    return s_basePath;
}

std::string ResourcePath::GetFullPath(const std::string& relativePath) {
    if (relativePath.empty()) {
        return s_basePath;
    }
    
    // If the relative path is already absolute, return it as-is
    std::filesystem::path relPath(relativePath);
    if (relPath.is_absolute()) {
        return NormalizePath(relativePath);
    }
    
    // Combine base path with relative path
    std::filesystem::path fullPath = std::filesystem::path(s_basePath) / relPath;
    return NormalizePath(fullPath.string());
}
bool ResourcePath::Exists(const std::string& relativePath) {
    std::string fullPath = GetFullPath(relativePath);
    return FileUtils::FileExists(fullPath);
}

std::string ResourcePath::NormalizePath(const std::string& path) {
    std::string normalized = path;
    
    // Convert all backslashes to forward slashes for consistency
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    
    // Remove duplicate slashes
    size_t pos = 0;
    while ((pos = normalized.find("//", pos)) != std::string::npos) {
        normalized.replace(pos, 2, "/");
    }
    
    return normalized;
} 
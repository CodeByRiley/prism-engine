#pragma once

#include <string>
#include "FileUtils.h"

class ResourcePath {
public:
    // Set the base path for resources (e.g., "resources/")
    static void SetBasePath(const std::string& basePath);
    
    // Get the full path by combining base path with relative path
    static std::string GetFullPath(const std::string& relativePath);
    
    // Get the current base path
    static const std::string& GetBasePath();
    
    // Check if a resource file exists
    static bool Exists(const std::string& relativePath);

private:
    static std::string s_basePath;
    
    // Helper to normalize path separators
    static std::string NormalizePath(const std::string& path);
}; 
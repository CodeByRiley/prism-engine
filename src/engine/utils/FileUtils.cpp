#include "FileUtils.h"

bool FileUtils::FileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

std::string FileUtils::GetFileContents(const std::string& path) {
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

std::string FileUtils::GetDirectory(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    return path.substr(0, lastSlash);
}
std::string FileUtils::GetFileName(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    return path.substr(lastSlash + 1);
}
std::string FileUtils::GetFileExtension(const std::string& path) {
    size_t lastDot = path.find_last_of('.');
    return path.substr(lastDot + 1);
}
std::string FileUtils::GetFileNameWithoutExtension(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    size_t lastDot = path.find_last_of('.');
    return path.substr(lastSlash + 1, lastDot - lastSlash - 1);
}
std::string FileUtils::GetFileSize(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    return std::to_string(file.tellg());
}
std::string FileUtils::GetFileLastModified(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    return std::to_string(file.tellg());
}


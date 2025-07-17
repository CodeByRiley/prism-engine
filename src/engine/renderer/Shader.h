#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    void Bind() const;
    void Unbind() const;

    // Uniform helpers
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;

    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec2(const std::string& name, const glm::vec2& value) const;
    void SetMat4(const std::string& name, const glm::mat4& value) const;
    void SetBool(const std::string& name, bool value) const;
    glm::vec4 GetVec4(const std::string& name) const;
    glm::vec3 GetVec3(const std::string& name) const;
    glm::vec2 GetVec2(const std::string& name) const;
    bool GetBool(const std::string& name) const;
    unsigned int GetID() const { return ID; }

private:
    unsigned int ID;
    std::string ReadFile(const std::string& path);
    unsigned int CompileShader(unsigned int type, const std::string& source);
    unsigned int CreateProgram(const std::string& vertexSrc, const std::string& fragmentSrc);
};
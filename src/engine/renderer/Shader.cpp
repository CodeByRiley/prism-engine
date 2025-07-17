#include "Shader.h"
#include <glad/glad.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <engine/utils/ResourcePath.h>
#include <engine/utils/Logger.h>

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSrc = ReadFile(ResourcePath::GetFullPath(vertexPath));
    std::string fragmentSrc = ReadFile(ResourcePath::GetFullPath(fragmentPath));
    ID = CreateProgram(vertexSrc, fragmentSrc);
}

Shader::~Shader() {
    glDeleteProgram(ID);
}

void Shader::Bind() const {
    glUseProgram(ID);
    //Logger::Info("Binding shader program: " + std::to_string(ID));
}

void Shader::Unbind() const {
    glUseProgram(0);
    //Logger::Info("Unbinding shader program");
}

void Shader::SetInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::SetVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::SetVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
glm::vec4 Shader::GetVec4(const std::string& name) const {
    glm::vec4 value;
    glGetUniformfv(ID, glGetUniformLocation(ID, name.c_str()), &value[0]);
    return value;
}
glm::vec3 Shader::GetVec3(const std::string& name) const {
    glm::vec3 value;
    glGetUniformfv(ID, glGetUniformLocation(ID, name.c_str()), &value[0]);
    return value;
}
glm::vec2 Shader::GetVec2(const std::string& name) const {
    glm::vec2 value;
    glGetUniformfv(ID, glGetUniformLocation(ID, name.c_str()), &value[0]);
    return value;
}

bool Shader::GetBool(const std::string& name) const {
    GLint value;
    glGetUniformiv(ID, glGetUniformLocation(ID, name.c_str()), &value);
    return value != 0;
}

void Shader::SetBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value ? 1 : 0);
}

void Shader::SetMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &value[0][0]);
}

std::string Shader::ReadFile(const std::string& path) {
    // Check if exists
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Shader file not found: " + path);
    }

    // Check if file is readable
    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("Not a regular file: " + path);
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + path);
    }

    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

unsigned int Shader::CompileShader(unsigned int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, info);
        Logger::Error<std::string>("Shader Compile Error: " + std::string(info));
        throw std::runtime_error("Shader compilation failed: " + std::string(info));
    }
    return shader;
}

unsigned int Shader::CreateProgram(const std::string& vertexSrc, const std::string& fragmentSrc) {
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexSrc);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    unsigned int program = glCreateProgram();

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[1024];
        glGetProgramInfoLog(program, 1024, nullptr, info);
        Logger::Error<std::string>("Shader Link Error: " + std::string(info));
        throw std::runtime_error("Shader linking failed: " + std::string(info));
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}
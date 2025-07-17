#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "Shader.h"
#include "../utils/ResourcePath.h"

#pragma pack(push, 1)
struct QuadInstance {
    glm::vec2 position;
    glm::vec2 size;
    float rotation;
    glm::vec4 color;
    float texIndex;
};
#pragma pack(pop)


class QuadBatch {
public:
    static constexpr size_t MaxQuads = 2048;
    static constexpr size_t MaxTextures = 16;

    QuadBatch();
    ~QuadBatch();

    void Begin(Shader* shader);
    void Add(const QuadInstance& instance);
    void End();
    void Flush();

private:
    std::vector<QuadInstance> m_Instances;
    Shader* m_CurrentShader = nullptr;
    unsigned int m_VAO, m_VBO, m_EBO, m_InstanceVBO;
    void SetupBuffers();
};
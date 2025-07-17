#include "QuadBatch.h"
#include <glad/glad.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <engine/utils/Logger.h>

QuadBatch::QuadBatch() {
    SetupBuffers();
}

QuadBatch::~QuadBatch() {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
    glDeleteBuffers(1, &m_InstanceVBO);
}

void QuadBatch::SetupBuffers() {
    float quadVertices[] = {
        // pos      // tex
        -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, 1.0f, 0.0f,
         0.5f,  0.5f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 1.0f
    };
    unsigned int quadIndices[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    // Vertex attributes for quad
    glEnableVertexAttribArray(0); // pos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); // texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // Instance buffer
    glGenBuffers(1, &m_InstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, MaxQuads * sizeof(QuadInstance), nullptr, GL_DYNAMIC_DRAW);

    // Instance attributes
    size_t offset = 0;
    glEnableVertexAttribArray(2); // iPos
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), (void*)offset);
    glVertexAttribDivisor(2, 1);
    offset += sizeof(glm::vec2);

    glEnableVertexAttribArray(3); // iSize
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), (void*)offset);
    glVertexAttribDivisor(3, 1);
    offset += sizeof(glm::vec2);

    glEnableVertexAttribArray(4); // iRotation
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), (void*)offset);
    glVertexAttribDivisor(4, 1);
    offset += sizeof(float);

    glEnableVertexAttribArray(5); // iColor
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), (void*)offset);
    glVertexAttribDivisor(5, 1);
    offset += sizeof(glm::vec4);

    glEnableVertexAttribArray(6); // iTexIndex
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), (void*)offset);
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}

void QuadBatch::Begin(Shader* shader) {
    // Store the shader for use in Flush
    m_CurrentShader = shader;
    
    // Make sure the shader is bound
    if (m_CurrentShader) {
        m_CurrentShader->Bind();
        //Logger::Info("QuadBatch::Begin - Bound shader ID: " + std::to_string(m_CurrentShader->GetID()));
    } else {
        Logger::Error("QuadBatch::Begin - No shader provided!", this);
    }
    
    glBindVertexArray(m_VAO);
    m_Instances.clear();
}

void QuadBatch::Add(const QuadInstance& instance) {
    m_Instances.push_back(instance);
    if (m_Instances.size() >= MaxQuads)
        Flush();
}

void QuadBatch::End() {
    if (!m_Instances.empty())
        Flush();
    
    glBindVertexArray(0);
    
    // Don't unbind the shader here - let the caller decide when to unbind
    // This ensures uniforms set outside of QuadBatch remain valid
    m_CurrentShader = nullptr;
}

void QuadBatch::Flush() {
    if (!m_CurrentShader) {
        Logger::Error("QuadBatch::Flush - No shader bound!", this);
        return;
    }
    
    // Make sure the shader is still bound
    m_CurrentShader->Bind();
    
    // Update instance data
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_Instances.size() * sizeof(QuadInstance), m_Instances.data());
    
    // Draw the instances
    glBindVertexArray(m_VAO);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, m_Instances.size());
    
    // Clear instances after drawing
    m_Instances.clear();
}


// Renderer2D.cpp
#include "Renderer2D.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>

Renderer2D::Renderer2D(int width, int height)
    : m_WindowWidth(width), m_WindowHeight(height)
{
    m_BaseShader = new Shader("shaders/BaseVertex.vert.glsl", "shaders/BaseFrag.frag.glsl");
    m_QuadBatch = new QuadBatch();
    m_Projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f);
    
    // Initialize texture uniforms for the base shader
    m_BaseShader->Bind();
    m_BaseShader->SetMat4("uProjection", m_Projection);
    
    // Set up texture unit uniforms (for when textures are used)
    for (int i = 0; i < 16; i++) {
        std::string uniformName = "u_Textures[" + std::to_string(i) + "]";
        m_BaseShader->SetInt(uniformName, i);
    }
    
    m_BaseShader->Unbind();
    SetWindowSize(1280, 720);
}

Renderer2D::~Renderer2D() {
    delete m_QuadBatch;
    delete m_BaseShader;
}

void Renderer2D::BeginBatch(Shader* shader) {
    if (!shader) shader = m_BaseShader;
    m_QuadBatch->Begin(shader);
    shader->SetMat4("uProjection", m_Projection);
}

void Renderer2D::EndBatch() {
    m_QuadBatch->End();
}

void Renderer2D::Flush() {
    m_QuadBatch->Flush();
}

void Renderer2D::DrawQuad(const glm::vec2& pos, const glm::vec2& size, float rotation, const glm::vec4& color, Texture2D* texture) {
    QuadInstance instance;
    instance.position = pos;
    instance.size = size;
    instance.rotation = rotation;
    instance.color = color;
    if(texture) {
        instance.texIndex = texture->GetIndex();
    } else {
        instance.texIndex = 0.0f; // For color-only quads
    }
    m_QuadBatch->Add(instance);
}

void Renderer2D::DrawRect(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color) {
    DrawQuad(pos, size, 0.0f, color, nullptr);
}

void Renderer2D::DrawRectRot(const glm::vec2& pos, const glm::vec2& size, float rotation, const glm::vec4& color) {
    DrawQuad(pos, size, rotation, color, nullptr);
}

void Renderer2D::DrawLine(const glm::vec2& p0, const glm::vec2& p1, float thickness, const glm::vec4& color) {
    glm::vec2 delta = p1 - p0;
    float length = glm::length(delta);
    float angle = atan2(delta.y, delta.x);
    glm::vec2 center = (p0 + p1) * 0.5f;
    DrawQuad(center, {length, thickness}, angle, color, nullptr);
}

void Renderer2D::DrawLineRot(const glm::vec2& p0, const glm::vec2& p1, float thickness, float rotation, const glm::vec4& color) {
    glm::vec2 delta = p1 - p0;
    float length = glm::length(delta);
    float angle = atan2(delta.y, delta.x) + rotation;
    glm::vec2 center = (p0 + p1) * 0.5f;
    DrawQuad(center, {length, thickness}, angle, color, nullptr);
}

void Renderer2D::DrawCircle(const glm::vec2& center, float radius, const glm::vec4& color) {
    DrawQuad(center, {radius * 2.0f, radius * 2.0f}, 0.0f, color, nullptr);
}

void Renderer2D::SetProjection(const glm::mat4& proj) {
    m_Projection = proj;
    m_BaseShader->Bind();
    m_BaseShader->SetMat4("uProjection", m_Projection);
    m_BaseShader->Unbind();
}

void Renderer2D::SetWindowSize(int width, int height) {
    m_WindowWidth = width;
    m_WindowHeight = height;
    SetProjection(glm::ortho(0.0f, (float)width, (float)height, 0.0f));
}
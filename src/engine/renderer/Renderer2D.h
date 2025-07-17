#pragma once
#include <glm/glm.hpp>
#include "QuadBatch.h"
#include "Shader.h"
#include "Texture2D.h"

class Renderer2D {
public:
    Renderer2D(int width, int height);
    ~Renderer2D();

    void BeginBatch(Shader* shader = nullptr);
    void EndBatch();
    void Flush();

    void DrawQuad(const glm::vec2& pos, const glm::vec2& size, float rotation, const glm::vec4& color, Texture2D* texture = nullptr);
    void DrawRect(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color);
    void DrawRectRot(const glm::vec2& pos, const glm::vec2& size, float rotation, const glm::vec4& color);
    void DrawLine(const glm::vec2& p0, const glm::vec2& p1, float thickness, const glm::vec4& color);
    void DrawLineRot(const glm::vec2& p0, const glm::vec2& p1, float thickness, float rotation, const glm::vec4& color);
    void DrawCircle(const glm::vec2& center, float radius, const glm::vec4& color);

    void SetProjection(const glm::mat4& proj);
    void SetWindowSize(int width, int height);

    int GetWindowWidth() const { return m_WindowWidth; }
    int GetWindowHeight() const { return m_WindowHeight; }
    Shader* GetBaseShader() const { return m_BaseShader; }

private:
    QuadBatch* m_QuadBatch;
    Shader*    m_BaseShader;
    int        m_WindowWidth, m_WindowHeight;
    glm::mat4  m_Projection;
};
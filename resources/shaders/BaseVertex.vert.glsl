#version 400 core
layout(location = 0) in vec2 aPos; // [-0.5, 0.5] quad local space
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec2 iPos; // instance: world position (pixels)
layout(location = 3) in vec2 iSize;
layout(location = 4) in float iRotation;
layout(location = 5) in vec4 iColor;
layout(location = 6) in float iTexIndex;

out vec2 TexCoord;
out vec4 Color;
flat out float TexIndex;

uniform mat4 uProjection;

void main() {
    float cosR = cos(iRotation);
    float sinR = sin(iRotation);
    mat2 rot = mat2(cosR, -sinR, sinR, cosR);
    vec2 scaled = aPos * iSize;
    vec2 world = iPos + rot * scaled;

    TexCoord = aTexCoord;
    Color = iColor;
    TexIndex = iTexIndex;

    gl_Position = uProjection * vec4(world, 0.0, 1.0);
}
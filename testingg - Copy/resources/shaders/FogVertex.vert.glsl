#version 400 core
layout(location = 0) in vec2 aPos;       // [-0.5, 0.5] quad local space
layout(location = 1) in vec2 aTexCoord;  // texture coordinates (from QuadBatch)
layout(location = 2) in vec2 iPos;       // instance: world position (pixels)
layout(location = 3) in vec2 iSize;      // instance: size
layout(location = 4) in float iRotation; // instance: rotation
layout(location = 5) in vec4 iColor;     // instance: color
layout(location = 6) in float iTexIndex; // instance: texture index

out vec2 vLocalPos;
out vec2 vWorldPos;
out vec2 vTexCoord;

uniform mat4 uProjection;

void main() {
    float cosR = cos(iRotation);
    float sinR = sin(iRotation);
    mat2 rot = mat2(cosR, -sinR, sinR, cosR);
    vec2 scaled = aPos * iSize;
    vec2 world = iPos + rot * scaled;

    vLocalPos = aPos;
    vWorldPos = world;
    vTexCoord = aTexCoord;

    gl_Position = uProjection * vec4(world, 0.0, 1.0);
}
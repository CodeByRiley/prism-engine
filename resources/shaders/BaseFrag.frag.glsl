#version 400 core

// Input from vertex shader
in vec2 TexCoord;
in vec4 Color;
flat in float TexIndex;

// Output color
out vec4 FragColor;

// Uniforms
uniform sampler2D u_Textures[16];  // Array of textures

void main() {
    // Ensure we have a valid color
    vec4 finalColor = Color;
    
    // If we have a texture (TexIndex > 0.5), use it
    if (TexIndex > 0.5) {
        // Convert TexIndex to integer index (1-16 maps to 0-15)
        int texIndex = int(TexIndex - 0.5);
        
        // Clamp texture index to valid range
        texIndex = clamp(texIndex, 0, 15);
        
        // Sample the texture and multiply by color
        vec4 texColor = texture(u_Textures[texIndex], TexCoord);
        finalColor = texColor * Color;
    }
    // If TexIndex <= 0.5, just use the color (no texture)
    
    FragColor = finalColor;
}
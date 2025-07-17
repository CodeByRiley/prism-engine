#include "Texture2D.h"
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

Texture2D::Texture2D(const std::string& path) : Index(0) {
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(path.c_str(), &Width, &Height, &Channels, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(data);
}
Texture2D::~Texture2D() { glDeleteTextures(1, &ID); }
void Texture2D::Bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, ID);
}
void Texture2D::Unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }
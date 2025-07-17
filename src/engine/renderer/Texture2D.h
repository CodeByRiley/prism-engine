#pragma once
#include <string>

class Texture2D {
public:
    unsigned int ID;
    int Width, Height, Channels;
    int Index; // Texture slot index

    Texture2D(const std::string& path);
    ~Texture2D();

    void Bind(unsigned int slot = 0) const;
    void Unbind() const;
    
    // Get the texture index for shader use
    int GetIndex() const { return Index; }
    void SetIndex(int index) { Index = index; }
};
#pragma once

#include "core/ShaderProgram.hpp"

#include <filesystem>

namespace game::core {

class TextureRenderer {
public:
    explicit TextureRenderer(const std::filesystem::path& imagePath);
    ~TextureRenderer();

    TextureRenderer(const TextureRenderer&) = delete;
    TextureRenderer& operator=(const TextureRenderer&) = delete;

    void draw(float x, float y, float width, float height) const;

private:
    ShaderProgram m_Shader;
    GLuint m_Texture = 0;
    GLuint m_VertexArray = 0;
    GLuint m_VertexBuffer = 0;
};

} // namespace game::core

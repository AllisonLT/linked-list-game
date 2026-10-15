#pragma once

#include "core/ShaderProgram.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <filesystem>
#include <string_view>
#include <unordered_map>

namespace game::core {

class TextRenderer {
public:
    TextRenderer(const std::filesystem::path& fontPath, unsigned int pixelHeight);
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    void draw(
        std::string_view text,
        float x,
        float baselineY,
        float red,
        float green,
        float blue,
        float alpha) const;
    [[nodiscard]] float measureWidth(std::string_view text) const;

private:
    struct Glyph {
        GLuint texture = 0;
        int width = 0;
        int height = 0;
        int bearingX = 0;
        int bearingY = 0;
        unsigned int advance = 0;
    };

    void release() noexcept;

    ShaderProgram m_Shader;
    FT_Library m_Library = nullptr;
    FT_Face m_Face = nullptr;
    std::unordered_map<unsigned char, Glyph> m_Glyphs;
    GLuint m_VertexArray = 0;
    GLuint m_VertexBuffer = 0;
};

} // namespace game::core

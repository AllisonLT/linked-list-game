#include "core/TextRenderer.hpp"

#include "core/Window.hpp"

#include <stdexcept>

namespace game::core {
namespace {

constexpr float toNormalizedDeviceX(float canvasX) {
    return (canvasX / Window::TARGET_WIDTH) * 2.0f - 1.0f;
}

constexpr float toNormalizedDeviceY(float canvasY) {
    return 1.0f - (canvasY / Window::TARGET_HEIGHT) * 2.0f;
}

} // namespace

TextRenderer::TextRenderer(const std::filesystem::path& fontPath, unsigned int pixelHeight)
    : m_Shader("assets/shaders/text.vert", "assets/shaders/text.frag") {
    if (FT_Init_FreeType(&m_Library) != 0) {
        throw std::runtime_error("Failed to initialize FreeType");
    }

    if (FT_New_Face(m_Library, fontPath.string().c_str(), 0, &m_Face) != 0) {
        FT_Done_FreeType(m_Library);
        m_Library = nullptr;
        throw std::runtime_error("Failed to load font: " + fontPath.string());
    }

    try {
        if (FT_Set_Pixel_Sizes(m_Face, 0, pixelHeight) != 0) {
            throw std::runtime_error("Failed to set font pixel size");
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        for (unsigned char character = 32; character < 127; ++character) {
            if (FT_Load_Char(m_Face, character, FT_LOAD_RENDER) != 0) {
                throw std::runtime_error("Failed to load font glyph");
            }

            GLuint texture = 0;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                m_Face->glyph->bitmap.width,
                m_Face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                m_Face->glyph->bitmap.buffer);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            m_Glyphs.emplace(
                character,
                Glyph{
                    .texture = texture,
                    .width = static_cast<int>(m_Face->glyph->bitmap.width),
                    .height = static_cast<int>(m_Face->glyph->bitmap.rows),
                    .bearingX = m_Face->glyph->bitmap_left,
                    .bearingY = m_Face->glyph->bitmap_top,
                    .advance = static_cast<unsigned int>(m_Face->glyph->advance.x),
                });
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        glGenVertexArrays(1, &m_VertexArray);
        glGenBuffers(1, &m_VertexBuffer);
        glBindVertexArray(m_VertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_Shader.use();
        m_Shader.setInteger("u_Text", 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } catch (...) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        release();
        throw;
    }
}

TextRenderer::~TextRenderer() {
    release();
}

void TextRenderer::draw(
    std::string_view text,
    float x,
    float baselineY,
    float red,
    float green,
    float blue,
    float alpha) const {
    m_Shader.use();
    m_Shader.setColor(red, green, blue, alpha);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_VertexArray);

    float cursorX = x;
    for (const unsigned char character : text) {
        const auto iterator = m_Glyphs.find(character);
        if (iterator == m_Glyphs.end()) {
            continue;
        }

        const Glyph& glyph = iterator->second;
        const float left = cursorX + static_cast<float>(glyph.bearingX);
        const float top = baselineY - static_cast<float>(glyph.bearingY);
        const float right = left + static_cast<float>(glyph.width);
        const float bottom = top + static_cast<float>(glyph.height);

        const float vertices[] = {
            toNormalizedDeviceX(left), toNormalizedDeviceY(top), 0.0f, 0.0f,
            toNormalizedDeviceX(right), toNormalizedDeviceY(top), 1.0f, 0.0f,
            toNormalizedDeviceX(right), toNormalizedDeviceY(bottom), 1.0f, 1.0f,
            toNormalizedDeviceX(left), toNormalizedDeviceY(top), 0.0f, 0.0f,
            toNormalizedDeviceX(right), toNormalizedDeviceY(bottom), 1.0f, 1.0f,
            toNormalizedDeviceX(left), toNormalizedDeviceY(bottom), 0.0f, 1.0f,
        };

        glBindTexture(GL_TEXTURE_2D, glyph.texture);
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        cursorX += static_cast<float>(glyph.advance >> 6);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}

float TextRenderer::measureWidth(std::string_view text) const {
    float width = 0.0f;
    for (const unsigned char character : text) {
        const auto iterator = m_Glyphs.find(character);
        if (iterator != m_Glyphs.end()) {
            width += static_cast<float>(iterator->second.advance >> 6);
        }
    }
    return width;
}

void TextRenderer::release() noexcept {
    if (m_VertexBuffer != 0) {
        glDeleteBuffers(1, &m_VertexBuffer);
        m_VertexBuffer = 0;
    }
    if (m_VertexArray != 0) {
        glDeleteVertexArrays(1, &m_VertexArray);
        m_VertexArray = 0;
    }
    for (const auto& [character, glyph] : m_Glyphs) {
        glDeleteTextures(1, &glyph.texture);
    }
    m_Glyphs.clear();
    if (m_Face != nullptr) {
        FT_Done_Face(m_Face);
        m_Face = nullptr;
    }
    if (m_Library != nullptr) {
        FT_Done_FreeType(m_Library);
        m_Library = nullptr;
    }
}

} // namespace game::core

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "core/TextureRenderer.hpp"
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

TextureRenderer::TextureRenderer(const std::filesystem::path& imagePath)
    : m_Shader("assets/shaders/text.vert", "assets/shaders/texture.frag") {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(imagePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        throw std::runtime_error(
            "Failed to load texture " + imagePath.string() + ": " + stbi_failure_reason());
    }

    try {
        glGenTextures(1, &m_Texture);
        glBindTexture(GL_TEXTURE_2D, m_Texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

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
        m_Shader.setInteger("u_Texture", 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } catch (...) {
        stbi_image_free(pixels);
        if (m_VertexBuffer != 0) {
            glDeleteBuffers(1, &m_VertexBuffer);
            m_VertexBuffer = 0;
        }
        if (m_VertexArray != 0) {
            glDeleteVertexArrays(1, &m_VertexArray);
            m_VertexArray = 0;
        }
        if (m_Texture != 0) {
            glDeleteTextures(1, &m_Texture);
            m_Texture = 0;
        }
        throw;
    }
    stbi_image_free(pixels);
}

TextureRenderer::~TextureRenderer() {
    if (m_VertexBuffer != 0) {
        glDeleteBuffers(1, &m_VertexBuffer);
    }
    if (m_VertexArray != 0) {
        glDeleteVertexArrays(1, &m_VertexArray);
    }
    if (m_Texture != 0) {
        glDeleteTextures(1, &m_Texture);
    }
}

void TextureRenderer::draw(float x, float y, float width, float height) const {
    const float vertices[] = {
        toNormalizedDeviceX(x), toNormalizedDeviceY(y), 0.0f, 0.0f,
        toNormalizedDeviceX(x + width), toNormalizedDeviceY(y), 1.0f, 0.0f,
        toNormalizedDeviceX(x + width), toNormalizedDeviceY(y + height), 1.0f, 1.0f,
        toNormalizedDeviceX(x), toNormalizedDeviceY(y), 0.0f, 0.0f,
        toNormalizedDeviceX(x + width), toNormalizedDeviceY(y + height), 1.0f, 1.0f,
        toNormalizedDeviceX(x), toNormalizedDeviceY(y + height), 0.0f, 1.0f,
    };

    m_Shader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_Texture);
    glBindVertexArray(m_VertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}

} // namespace game::core

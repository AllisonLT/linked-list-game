#pragma once

#include <glad/glad.h>

#include <filesystem>

namespace game::core {

class ShaderProgram {
public:
    ShaderProgram(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    void use() const;
    void setColor(float red, float green, float blue, float alpha) const;

private:
    static GLuint compileShader(GLenum type, const std::filesystem::path& path);

    GLuint m_Program = 0;
};

} // namespace game::core

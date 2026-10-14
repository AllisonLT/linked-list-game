#include "core/ShaderProgram.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace game::core {
namespace {

std::string readShaderSource(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open shader source: " + path.string());
    }

    std::stringstream source;
    source << file.rdbuf();
    return source.str();
}

std::string shaderInfoLog(GLuint shader) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) {
        return {};
    }

    std::vector<char> log(length);
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    return {log.data()};
}

std::string programInfoLog(GLuint program) {
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) {
        return {};
    }

    std::vector<char> log(length);
    glGetProgramInfoLog(program, length, nullptr, log.data());
    return {log.data()};
}

} // namespace

ShaderProgram::ShaderProgram(
    const std::filesystem::path& vertexPath,
    const std::filesystem::path& fragmentPath) {
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexPath);
    GLuint fragmentShader = 0;

    try {
        fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentPath);
        m_Program = glCreateProgram();
        if (m_Program == 0) {
            throw std::runtime_error("Failed to create shader program");
        }

        glAttachShader(m_Program, vertexShader);
        glAttachShader(m_Program, fragmentShader);
        glLinkProgram(m_Program);

        GLint linked = GL_FALSE;
        glGetProgramiv(m_Program, GL_LINK_STATUS, &linked);

        if (linked == GL_FALSE) {
            throw std::runtime_error("Failed to link shader program: " + programInfoLog(m_Program));
        }
    } catch (...) {
        if (m_Program != 0) {
            glDeleteProgram(m_Program);
            m_Program = 0;
        }
        glDeleteShader(fragmentShader);
        glDeleteShader(vertexShader);
        throw;
    }

    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
}

ShaderProgram::~ShaderProgram() {
    if (m_Program != 0) {
        glDeleteProgram(m_Program);
    }
}

void ShaderProgram::use() const {
    glUseProgram(m_Program);
}

void ShaderProgram::setColor(float red, float green, float blue, float alpha) const {
    const GLint colorLocation = glGetUniformLocation(m_Program, "u_Color");
    glUniform4f(colorLocation, red, green, blue, alpha);
}

GLuint ShaderProgram::compileShader(GLenum type, const std::filesystem::path& path) {
    const std::string source = readShaderSource(path);
    const char* sourcePointer = source.c_str();

    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &sourcePointer, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        const std::string log = shaderInfoLog(shader);
        glDeleteShader(shader);
        throw std::runtime_error("Failed to compile shader " + path.string() + ": " + log);
    }

    return shader;
}

} // namespace game::core

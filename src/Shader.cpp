#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

static std::string readFile(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()){
        std::cerr << "ERROR: Could not open shader file: " << path << '\n';
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static unsigned int compileShader(const char* src, GLenum type) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    int success;
    char log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, log);
        std::cerr << "SHADER COMPILE ERROR ("
                 << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
                 << "):\n" << log << '\n';
    }
    return shader;
}

Shader::Shader(const char* vertexPath, const char* fragmentPath){
    std::string vCode = readFile(vertexPath);
    std::string fCode = readFile(fragmentPath);

    unsigned int vert = compileShader(vCode.c_str(), GL_VERTEX_SHADER);
    unsigned int frag = compileShader(fCode.c_str(), GL_FRAGMENT_SHADER);

    ID = glCreateProgram();
    glAttachShader(ID, vert);
    glAttachShader(ID, frag);
    glLinkProgram(ID);

    int success;
    char log[512];
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, log);
        std::cerr << "SHADER LINK ERROR: \n" << log << '\n';
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
}

Shader::~Shader() {
    glDeleteProgram(ID);
}

void Shader::use() const { glUseProgram(ID); }

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}
void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setVec3(const std::string& name, const glm::vec3& vec) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
}


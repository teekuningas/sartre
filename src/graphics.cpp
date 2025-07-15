#include "graphics.h"

#include <iostream>
#include <vector>

// —— Shader utilities ——
GLuint loadShader(GLenum type, const std::string& source) {
  GLuint shader = glCreateShader(type);
  const char* src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  GLint compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint infoLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    std::vector<GLchar> infoLog(infoLen);
    glGetShaderInfoLog(shader, infoLen, &infoLen, &infoLog[0]);
    std::cerr << "Error compiling shader:\n" << &infoLog[0] << std::endl;
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

void createProgram(const std::string& vertexSource, const std::string& fragmentSource,
                   GLuint& shaderProgram) {
  GLuint vs = loadShader(GL_VERTEX_SHADER, vertexSource);
  GLuint fs = loadShader(GL_FRAGMENT_SHADER, fragmentSource);

  GLuint prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);

  GLint linked = 0;
  glGetProgramiv(prog, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint infoLen = 0;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLen);
    std::vector<GLchar> infoLog(infoLen);
    glGetProgramInfoLog(prog, infoLen, &infoLen, &infoLog[0]);
    std::cerr << "Error linking program:\n" << &infoLog[0] << std::endl;
    glDeleteProgram(prog);
  }
  shaderProgram = prog;
}

void createShaderBuffers(GLuint& VAO, GLuint& VBO) {
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, nullptr, GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

// —— Matrix utilities ——
void createOrthographicMatrix(float left, float right, float bottom, float top, float nearPlane,
                              float farPlane, float* m) {
  std::fill(m, m + 16, 0.0f);
  m[0] = 2.0f / (right - left);
  m[5] = 2.0f / (top - bottom);
  m[10] = -2.0f / (farPlane - nearPlane);
  m[12] = -(right + left) / (right - left);
  m[13] = -(top + bottom) / (top - bottom);
  m[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
  m[15] = 1.0f;
}

void createTranslationMatrix(float tx, float ty, float tz, float* m) {
  std::fill(m, m + 16, 0.0f);
  m[0] = m[5] = m[10] = 1.0f;
  m[12] = tx;
  m[13] = ty;
  m[14] = tz;
  m[15] = 1.0f;
}

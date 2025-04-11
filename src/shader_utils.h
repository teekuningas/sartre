// shader_utils.h
#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include <string>

#include "GL/glew.h"

GLuint loadShader(GLenum type, const std::string& source);

void createProgram(const std::string& vertexSource, const std::string& fragmentSource,
                   GLuint& shaderProgram);

void createShaderBuffers(GLuint& VAO, GLuint& VBO);

#endif

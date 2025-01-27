// shader_utils.h
#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include "GL/glew.h"
#include <string>

GLuint loadShader(GLenum type, const std::string& source);
GLuint createProgram(const std::string& vertexSource, const std::string& fragmentSource);

#endif

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <string>
#include <GL/glew.h>

// Compile a shader (vertex or fragment) from source.
GLuint loadShader(GLenum type, const std::string &source);

// Link a vertex + fragment shader into a program.
void createProgram(const std::string &vertexSource,
                   const std::string &fragmentSource,
                   GLuint &shaderProgram);

// Build a VAO/VBO for dynamic quads (pos + texcoord).
void createShaderBuffers(GLuint &VAO, GLuint &VBO);

// Build a 4×4 column-major translation matrix (tx,ty,tz).
void createTranslationMatrix(float tx, float ty, float tz, float *matrix);

// Build a 4×4 column-major orthographic projection.
void createOrthographicMatrix(float left, float right,
                              float bottom, float top,
                              float nearPlane, float farPlane,
                              float *matrix);

#endif // GRAPHICS_H

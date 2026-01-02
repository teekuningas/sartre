#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_ttf.h>

#include <string>

#include "types.h"

// Compile a shader (vertex or fragment) from source.
GLuint loadShader(GLenum type, const std::string& source);

// Link a vertex + fragment shader into a program.
void createProgram(const std::string& vertexSource, const std::string& fragmentSource,
                   GLuint& shaderProgram);

// Build a VAO/VBO for dynamic quads (pos + texcoord).
void createShaderBuffers(GLuint& VAO, GLuint& VBO);

// Build a 4×4 column-major translation matrix (tx,ty,tz).
void createTranslationMatrix(float tx, float ty, float tz, float* matrix);

// Build a 4×4 column-major orthographic projection.
void createOrthographicMatrix(float left, float right, float bottom, float top, float nearPlane,
                              float farPlane, float* matrix);

SDL_Surface* format_sdl_surface(SDL_Surface* surface);

void create_textures(Textures& textures, const std::string& dataPath);
void create_surfaces(Surfaces& surfaces, const std::string& dataPath);
void load_descriptions(ImageData& imageData, const std::string& dataPath);
void free_textures(Textures& textures);
void free_surfaces(Surfaces& surfaces);

void renderText(RenderContext& context, TTF_Font* font, const std::string& text, SDL_Color color,
                GLuint shader, GLuint VAO, GLuint VBO, float x, float y, int wrapChars = 0);

void getTextSize(TTF_Font* font, const std::string& text, int wrapChars, int& outW, int& outH);

#endif  // GRAPHICS_H

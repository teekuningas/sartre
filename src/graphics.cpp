#include "graphics.h"

#include <iostream>
#include <vector>
#include <SDL_image.h>
#include <cstring>

// --- begin pasted from utils.cpp ---

SDL_Surface* format_sdl_surface(SDL_Surface* surface) {
  if (!surface) {
    printf("Error: SDL surface null.\n");
    return nullptr;
  }
  if (!surface->w || !surface->h || (surface->w & 1) || (surface->h & 1)) {
    printf("Error: Invalid SDL surface.\n");
    return nullptr;
  }

  SDL_Surface* formattedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
  if (!formattedSurface) {
    printf("Error: Could not format a surface: %s\n", SDL_GetError());
    return nullptr;
  }
  return formattedSurface;
}

void create_textures(Textures& textures, const std::string& dataPath) {
  // Sartre
  SDL_Surface* forestSartreImage[2];
  forestSartreImage[0] = IMG_Load((dataPath + "images/sartre.png").c_str());
  if (!forestSartreImage[0]) {
    printf("Error loading image: %s\n", SDL_GetError());
    exit(1);
  }
  forestSartreImage[1] = IMG_Load((dataPath + "images/sartre2.png").c_str());
  if (!forestSartreImage[1]) {
    printf("Error loading image: %s\n", SDL_GetError());
    exit(1);
  }
  glGenTextures(2, textures.forestSartre);
  for (int i = 0; i < 2; i++) {
    SDL_Surface* formattedSurface = format_sdl_surface(forestSartreImage[i]);
    if (!formattedSurface) {
      exit(1);
    }
    glBindTexture(GL_TEXTURE_2D, textures.forestSartre[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formattedSurface->w, formattedSurface->h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, formattedSurface->pixels);
    SDL_FreeSurface(formattedSurface);
    SDL_FreeSurface(forestSartreImage[i]);
  }

  // Pages
  SDL_Surface* forestPageImage;
  forestPageImage = IMG_Load((dataPath + "images/objects/page.png").c_str());
  if (!forestPageImage) {
    printf("Error loading image: %s\n", SDL_GetError());
    exit(1);
  }
  glGenTextures(1, &textures.forestPage);
  SDL_Surface* formattedPageSurface = format_sdl_surface(forestPageImage);
  if (!formattedPageSurface) {
    exit(1);
  }
  glBindTexture(GL_TEXTURE_2D, textures.forestPage);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formattedPageSurface->w, formattedPageSurface->h, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, formattedPageSurface->pixels);
  SDL_FreeSurface(formattedPageSurface);
  SDL_FreeSurface(forestPageImage);

  // Chestnut
  SDL_Surface* forestChestnutImage;
  forestChestnutImage = IMG_Load((dataPath + "images/objects/chestnut.png").c_str());
  if (!forestChestnutImage) {
    printf("Error loading image: %s\n", SDL_GetError());
    exit(1);
  }
  glGenTextures(1, &textures.forestChestnut);
  SDL_Surface* formattedChestnutSurface = format_sdl_surface(forestChestnutImage);
  if (!formattedChestnutSurface) {
    exit(1);
  }
  glBindTexture(GL_TEXTURE_2D, textures.forestChestnut);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formattedChestnutSurface->w, formattedChestnutSurface->h,
               0, GL_RGBA, GL_UNSIGNED_BYTE, formattedChestnutSurface->pixels);
  SDL_FreeSurface(formattedChestnutSurface);
  SDL_FreeSurface(forestChestnutImage);

  // Pipe
  SDL_Surface* forestPipeImage;
  forestPipeImage = IMG_Load((dataPath + "images/objects/pipe.png").c_str());
  if (!forestPipeImage) {
    printf("Error loading image: %s\n", SDL_GetError());
    exit(1);
  }
  glGenTextures(1, &textures.forestPipe);
  SDL_Surface* formattedPipeSurface = format_sdl_surface(forestPipeImage);
  if (!formattedPipeSurface) {
    exit(1);
  }
  glBindTexture(GL_TEXTURE_2D, textures.forestPipe);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formattedPipeSurface->w, formattedPipeSurface->h, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, formattedPipeSurface->pixels);
  SDL_FreeSurface(formattedPipeSurface);
  SDL_FreeSurface(forestPipeImage);

  // Background
  SDL_Surface* forestTaustaImage;
  forestTaustaImage = IMG_Load((dataPath + "images/lehto.png").c_str());
  if (!forestTaustaImage) {
    printf("Error loading image: %s\n", SDL_GetError());
    exit(1);
  }
  SDL_Surface* formattedSurface = format_sdl_surface(forestTaustaImage);
  if (!formattedSurface) {
    exit(1);
  }
  glGenTextures(1, textures.forestTausta);
  glBindTexture(GL_TEXTURE_2D, textures.forestTausta[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formattedSurface->w, formattedSurface->h, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, formattedSurface->pixels);
  SDL_FreeSurface(formattedSurface);
  SDL_FreeSurface(forestTaustaImage);
}

void create_surfaces(Surfaces& surfaces, const std::string& dataPath) {
  // Load collision map
  surfaces.forestCollisionMap =
      format_sdl_surface(IMG_Load((dataPath + "images/lehto_platforms.png").c_str()));
  if (!surfaces.forestCollisionMap) {
    printf("Error loading collision map: %s\n", SDL_GetError());
    exit(1);
  }
}

void free_textures(Textures& textures) {
  for (int a = 0; a < 2; a++) {
    glDeleteTextures(1, &textures.forestSartre[a]);
  }
  glDeleteTextures(1, &textures.forestPage);
  glDeleteTextures(1, &textures.forestChestnut);
  glDeleteTextures(1, &textures.forestPipe);
  glDeleteTextures(1, &textures.forestTausta[0]);
}

void free_surfaces(Surfaces& surfaces) { SDL_FreeSurface(surfaces.forestCollisionMap); }

void renderText(TTF_Font* font, const std::string& text, SDL_Color color, GLuint shader, GLuint VAO,
                GLuint VBO, float x, float y) {
  // Create an SDL surface with the text
  SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
  if (!surface) {
    printf("Failed to render text surface: %s\n", TTF_GetError());
    return;
  }

  // Ensure the surface has the expected format
  if (surface->format->BytesPerPixel != 4) {
    printf("Unexpected surface format: %d bytes per pixel\n", surface->format->BytesPerPixel);
    SDL_FreeSurface(surface);
    return;
  }

  // Create OpenGL texture and upload data
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  // Use glPixelStorei to set unpack alignment
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // Manually buffer pixel data to handle pitch (row alignment) issues
  int mode = GL_RGBA;
  const int pitch = surface->pitch;  // The bytes per row in the surface
  const int width = surface->w;
  const int height = surface->h;

  // Allocate buffer for tightly packed pixel data
  std::vector<unsigned char> pixels(width * height * 4);  // 4 bytes per pixel for RGBA

  // Copy each row from surface->pixels to the new buffer
  for (int y = 0; y < height; ++y) {
    std::memcpy(&pixels[y * width * 4],                                    // Target
                static_cast<unsigned char*>(surface->pixels) + y * pitch,  // Source
                width * 4  // Number of bytes to copy
    );
  }

  // Upload to OpenGL
  glTexImage2D(GL_TEXTURE_2D, 0, mode, width, height, 0, mode, GL_UNSIGNED_BYTE, pixels.data());

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Set the texture uniform and text color uniform
  GLint textColorLoc = glGetUniformLocation(shader, "textColor");
  glUniform4f(textColorLoc, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

  // Define the vertices and texture coordinates for a quad
  float w = static_cast<float>(width);
  float h = static_cast<float>(height);
  float vertices[] = {x,     y,     0.0f, 0.0f, x + w, y,     1.0f, 0.0f,
                      x + w, y - h, 1.0f, 1.0f, x,     y - h, 0.0f, 1.0f};

  // Blended font needs this
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Bind the text VAO and update buffer data
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

  // Use the uploaded texture in your shader
  glBindTexture(GL_TEXTURE_2D, texture);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);  // Drawing the quad

  // Unbind the VAO and texture
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Cleanup
  glDeleteTextures(1, &texture);
  SDL_FreeSurface(surface);

  glDisable(GL_BLEND);
}

// --- end pasted from utils.cpp ---

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

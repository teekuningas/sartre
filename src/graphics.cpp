#include "graphics.h"

#include <SDL_image.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "types.h"

static SDL_Surface* format_sdl_surface(SDL_Surface* surface) {
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

static GLuint load_texture(const std::string& path) {
  SDL_Surface* surface = IMG_Load(path.c_str());
  if (!surface) {
    printf("Error loading image %s: %s\n", path.c_str(), SDL_GetError());
    exit(1);
  }

  SDL_Surface* formatted = format_sdl_surface(surface);
  SDL_FreeSurface(surface);
  if (!formatted) {
    exit(1);
  }

  GLuint textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formatted->w, formatted->h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               formatted->pixels);

  SDL_FreeSurface(formatted);
  return textureID;
}

void create_textures(Textures& textures, const std::string& dataPath) {
  // Sartre
  textures.forestSartre[0] = load_texture(dataPath + "images/sartre.png");
  textures.forestSartre[1] = load_texture(dataPath + "images/sartre2.png");

  // Pages
  textures.forestPage = load_texture(dataPath + "images/objects/page.png");

  // Chestnut
  textures.forestChestnut = load_texture(dataPath + "images/objects/chestnut.png");

  // Pipe
  textures.forestPipe = load_texture(dataPath + "images/objects/pipe.png");

  // Beer
  textures.forestBeer = load_texture(dataPath + "images/objects/beer.png");

  // Clock
  textures.forestClock = load_texture(dataPath + "images/objects/clock.png");

  // Background
  textures.forestTausta[0] = load_texture(dataPath + "images/lehto.png");
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

static std::string readFile(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return "";
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

void load_descriptions(ImageData& imageData, const std::string& dataPath) {
  std::string descPath = dataPath + "descriptions/";

  imageData.itemDescriptions[CHESTNUT] = readFile(descPath + "chestnut.txt");
  imageData.itemDescriptions[PIPE] = readFile(descPath + "pipe.txt");
  imageData.itemDescriptions[BEER] = readFile(descPath + "beer.txt");
  imageData.itemDescriptions[CLOCK] = readFile(descPath + "clock.txt");

  for (int i = 0; i <= 3; ++i) {
    std::string content = readFile(descPath + "page" + std::to_string(i) + ".txt");
    if (!content.empty()) {
      imageData.pageDescriptions.push_back(content);
    }
  }
}

void free_textures(Textures& textures) {
  for (int a = 0; a < 2; a++) {
    glDeleteTextures(1, &textures.forestSartre[a]);
  }
  glDeleteTextures(1, &textures.forestPage);
  glDeleteTextures(1, &textures.forestChestnut);
  glDeleteTextures(1, &textures.forestPipe);
  glDeleteTextures(1, &textures.forestBeer);
  glDeleteTextures(1, &textures.forestClock);
  glDeleteTextures(1, &textures.forestTausta[0]);
}

void free_surfaces(Surfaces& surfaces) { SDL_FreeSurface(surfaces.forestCollisionMap); }

void renderText(RenderContext& context, TTF_Font* font, const std::string& text, SDL_Color color,
                GLuint shader, GLuint VAO, GLuint VBO, float x, float y, int wrapChars, int style) {
  // 0) if wrapping requested, split into words & lines, then recurse without wrapping
  if (wrapChars > 0) {
    std::istringstream iss(text);
    std::string word, line;
    std::vector<std::string> lines;
    while (iss >> word) {
      if (!line.empty() && line.size() + 1 + word.size() > (size_t)wrapChars) {
        lines.push_back(line);
        line = word;
      } else {
        if (!line.empty()) line += ' ';
        line += word;
      }
    }
    if (!line.empty()) lines.push_back(line);
    int lineSkip = TTF_FontLineSkip(font);
    for (size_t i = 0; i < lines.size(); ++i) {
      // each line is treated as wrapChars==0
      renderText(context, font, lines[i], color, shader, VAO, VBO, x, y - i * lineSkip, 0, style);
    }
    return;
  }

  // 1) build a cache key (now wrapChars==0)
  std::string key = text + "#" + std::to_string(wrapChars) + "#" + std::to_string(style);
  auto it = context.textCache.find(key);
  RenderContext::TextCacheEntry e;
  if (it == context.textCache.end()) {
    // 2) create an SDL_Surface with no wrapping
    // Always render as white so we can tint it with uniform later
    SDL_Color white = {255, 255, 255, 255};

    // Set style before rendering
    TTF_SetFontStyle(font, style);

    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text.c_str(), white);

    // Reset to normal just in case (optional, but good practice if font is shared)
    TTF_SetFontStyle(font, TTF_STYLE_NORMAL);

    if (!surf) {
      printf("TTF error: %s\n", TTF_GetError());
      return;
    }
    // 3) convert to RGBA32
    SDL_Surface* fmt = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surf);
    if (!fmt) {
      printf("Surface‐format error\n");
      return;
    }
    // 4) upload once
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    // restore the old surface-alignment (our pitch is width*4)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // WebGL requires NPOT → clamp-to-edge
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // linear filtering (no mipmaps)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fmt->w, fmt->h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 fmt->pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    e = {tex, fmt->w, fmt->h};
    context.textCache.emplace(key, e);
    SDL_FreeSurface(fmt);
  } else {
    e = it->second;
  }

  // 5) draw that quad
  glUseProgram(shader);
  // — set textColor uniform from SDL_Color (r,g,b,a in [0..255])
  {
    GLfloat fr = color.r / 255.0f;
    GLfloat fg = color.g / 255.0f;
    GLfloat fb = color.b / 255.0f;
    GLfloat fa = color.a / 255.0f;
    GLint loc = glGetUniformLocation(shader, "textColor");
    glUniform4f(loc, fr, fg, fb, fa);
  }
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, e.texture);
  float verts[16] = {x,       y,       0.0f, 0.0f, x + e.w, y,       1.0f, 0.0f,
                     x + e.w, y - e.h, 1.0f, 1.0f, x,       y - e.h, 0.0f, 1.0f};
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_BLEND);
}

void getTextSize(TTF_Font* font, const std::string& text, int wrapChars, int& outW, int& outH) {
  if (wrapChars <= 0) {
    TTF_SizeUTF8(font, text.c_str(), &outW, &outH);
    return;
  }

  std::istringstream iss(text);
  std::string word, line;
  std::vector<std::string> lines;
  while (iss >> word) {
    if (!line.empty() && line.size() + 1 + word.size() > (size_t)wrapChars) {
      lines.push_back(line);
      line = word;
    } else {
      if (!line.empty()) line += ' ';
      line += word;
    }
  }
  if (!line.empty()) lines.push_back(line);

  int maxW = 0;
  int totalH = 0;
  int lineSkip = TTF_FontLineSkip(font);

  for (size_t i = 0; i < lines.size(); ++i) {
    int w, h;
    TTF_SizeUTF8(font, lines[i].c_str(), &w, &h);
    if (w > maxW) maxW = w;
    if (i == lines.size() - 1) {
      totalH += h;
    } else {
      totalH += lineSkip;
    }
  }
  outW = maxW;
  outH = totalH;
}

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

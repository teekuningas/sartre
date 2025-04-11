#ifndef UTILS_H
#define UTILS_H

#include <string>

#include "types.h"

WindowParams compute_window_params(bool fullscreen);

SDL_Surface *format_sdl_surface(SDL_Surface *surface);

void create_textures(Textures &textures, std::string dataPath);
void create_surfaces(Surfaces &surfaces, std::string dataPath);
void free_textures(Textures &textures);
void free_surfaces(Surfaces &surfaces);

bool isPixelBlack(SDL_Surface *surface, int x, int y);

void renderText(TTF_Font *font, const std::string &text, SDL_Color color, GLuint shader, GLuint VAO,
                GLuint VBO, float x, float y);

#endif

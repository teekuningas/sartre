#ifndef GAME_H
#define GAME_H

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_ttf.h>

#include "types.h"

struct GameLoopData;

// Drives one frame: update, event‐handling, draw & exit logic.
void main_loop_iteration(GameLoopData& data);

// --- Menu state ---
void menu_init(GameStateMenu& state);
void menu_update(GameStateMenu& state, Uint32 totalElapsed, float deltaTime, Surfaces& surfaces,
                 InputResult& result);
void menu_draw(TTF_Font* font, GLuint shaderProgram, GLuint VAO, GLuint VBO);

// --- Forest state ---
void forest_init(GameStateForest& state);
void forest_update(GameStateForest& state, Uint32 totalElapsed, float deltaTime, Surfaces& surfaces,
                   InputResult& result);
void forest_draw(GameStateForest& state, Textures& textures, RenderContext& context,
                 GLuint shaderProgram, GLuint VAO, GLuint VBO);

// --- Results state ---
void results_init(GameStateResults& state);
void results_update(GameStateResults& state, Uint32 totalElapsed, float deltaTime,
                    Surfaces& surfaces, InputResult& result);
void results_draw(TTF_Font* font, GLuint shaderProgram, GLuint VAO, GLuint VBO);

// --- Input and transitions ---
void handle_events(GameMode& gameMode, bool fullscreen, InputResult& result);

#endif  // GAME_H

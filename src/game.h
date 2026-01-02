#ifndef GAME_H
#define GAME_H

#include "types.h"

struct GameLoopData;

// Called once per tick to do update, event‐handling, draw & state switches.
void run_game_frame(GameLoopData& data);

// --- Menu state ---
void menu_init(GameStateMenu& state);
void menu_update(GameStateMenu& state, Uint32 totalElapsed, float deltaTime, Surfaces& surfaces,
                 InputResult& result);
void menu_draw(RenderContext& context, Textures& textures, GameStateMenu const& state);

// --- Forest state ---
void forest_init(GameStateForest& state, ImageData const& imageData, bool fastMode);
void forest_update(GameStateForest& state, RenderContext& context, Uint32 totalElapsed,
                   float deltaTime, ImageData& imageData, InputResult& result);
void forest_draw(GameStateForest& state, Textures& textures, RenderContext& context,
                 GLuint shaderProgram, GLuint VAO, GLuint VBO);

// --- Input and transitions ---
void handle_events(GameLoopData& data, InputResult& result);

#endif  // GAME_H

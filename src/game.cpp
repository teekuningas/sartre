#include "game.h"
#include "constants.h"
#include "engine.h"
#include "graphics.h"
#include "resources.h"
#include "types.h"
#include "utils.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <GL/glew.h>
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>

GameLoopData gameLoopData;

void main_loop_iteration(GameLoopData& data) {
  /* Initialization and cleanup are kept within the loop function
   * for the sake of webgl context which would not be
   * automatically active here if initialized outside.
   */

  if (data.shouldExit) {
    if (data.initialized) {
      free_textures(data.imageData.textures);
      free_surfaces(data.imageData.surfaces);
    }
    shutdownEngine(data.context);
    exit(0);
  }

  if (!data.initialized) {
    std::string dataPath = getResourcePath();

    if (!initEngine(data.context, dataPath, data.fullscreen)) {
      data.shouldExit = true;
      return;
    }

    // Compile shader program and create VAO and VBO for text rendering
    createProgram(textVertexShaderSource, textFragmentShaderSource,
                  data.context.textShaderProgram);
    if (!data.context.textShaderProgram) {
      data.shouldExit = true;
      return;
    }
    createShaderBuffers(data.context.textVAO, data.context.textVBO);

    // Compile shader program and create VAO and VBO for forest rendering
    createProgram(forestVertexShaderSource, forestFragmentShaderSource,
                  data.context.forestShaderProgram);
    if (!data.context.forestShaderProgram) {
      data.shouldExit = true;
      return;
    }
    createShaderBuffers(data.context.forestVAO, data.context.forestVBO);

    WindowParams windowParams = compute_window_params(data.fullscreen);
    glViewport((windowParams.windowWidth - windowParams.viewportSize) / 2,
               (windowParams.windowHeight - windowParams.viewportSize) / 2,
               windowParams.viewportSize, windowParams.viewportSize);

    create_textures(data.imageData.textures, dataPath);
    create_surfaces(data.imageData.surfaces, dataPath);

    data.gameMode = MENU;
    data.lastTick = SDL_GetTicks();
    data.totalElapsed = 0;

    // Do not come here anymore
    data.initialized = true;
  }

  // --- Main Loop Logic ---

  InputResult inputResult;
  inputResult.transition = false;  // Initialize for this frame

  // Calculate delta time
  data.currentTick = SDL_GetTicks();
  float deltaTime = (data.currentTick - data.lastTick) / 1000.0f;
  data.totalElapsed += data.currentTick - data.lastTick;
  data.lastTick = data.currentTick;

  // 1. Update current game state (can potentially set inputResult.transition)
  switch (data.gameMode) {
    case MENU:
      menu_update(data.gameStateMenu, data.totalElapsed, deltaTime,
                  data.imageData.surfaces, inputResult);
      break;
    case FOREST:
      forest_update(data.gameStateForest, data.totalElapsed, deltaTime,
                    data.imageData.surfaces, inputResult);
      break;
    case RESULTS:
      results_update(data.gameStateResults, data.totalElapsed, deltaTime,
                     data.imageData.surfaces, inputResult);
      break;
    default:
      break;
  }

  // 2. Handle user events (can also set inputResult.transition)
  handle_events(data.gameMode, data.fullscreen, inputResult);

  // 3. Check if a transition is requested (either by update or events)
  if (inputResult.transition) {
    if (inputResult.transitionTo == EXIT) {
      data.shouldExit = true;
      return;
    }
    if (inputResult.transitionTo == FOREST) {
      // reset clock so each run starts fresh
      data.totalElapsed = 0;
      data.lastTick = SDL_GetTicks();
      if (Mix_PlayMusic(data.context.backgroundMusic, -1) == -1) {
        printf("Failed to play background music! SDL_mixer Error: %s\n", Mix_GetError());
        data.shouldExit = true;
      }
      forest_init(data.gameStateForest);
    } else {
      Mix_HaltMusic();
    }
    if (inputResult.transitionTo == MENU) {
      menu_init(data.gameStateMenu);
    }
    if (inputResult.transitionTo == RESULTS) {
      results_init(data.gameStateResults);
    }
    data.gameMode = inputResult.transitionTo;
    return;
  }

  // 4. Draw the current state
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  switch (data.gameMode) {
    case MENU:
      menu_draw(data.context.font, data.context.textShaderProgram,
                data.context.textVAO, data.context.textVBO);
      break;
    case FOREST:
      forest_draw(data.gameStateForest, data.imageData.textures,
                  data.context, data.context.forestShaderProgram,
                  data.context.forestVAO, data.context.forestVBO);
      break;
    case RESULTS:
      results_draw(data.context.font, data.context.textShaderProgram,
                   data.context.textVAO, data.context.textVBO);
      break;
    default:
      break;
  }

  SDL_GL_SwapWindow(data.context.window);
#ifndef __EMSCRIPTEN__
  SDL_Delay(1);
#endif
}

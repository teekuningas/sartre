#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include <cstdlib>
#include <ctime>
#define SDL_MAIN_HANDLED
#include <cstring>  // for strcmp
#include <iostream>

#include "constants.h"  // textVertexShaderSource, forestVertexShaderSource, …
#include "engine.h"     // initEngine, shutdownEngine
#include "game.h"       // run_game_frame
#include "graphics.h"   // createProgram, createShaderBuffers, textVertex/fragmentSource
#include "resources.h"  // getResourcePath()
#include "utils.h"      // create_textures, create_surfaces, free_*

// ------------------------------------------------------------------------
void main_loop_iteration(GameLoopData* pdata) {
  auto& data = *pdata;

  if (data.shouldExit) {
    if (data.initialized) {
      free_textures(data.imageData.textures);
      free_surfaces(data.imageData.surfaces);
    }
    shutdownEngine(data.context);
    exit(0);
  }

  if (!data.initialized) {
    // 1) compile & link shaders & make VAOs/VBOs
    createProgram(textVertexShaderSource, textFragmentShaderSource, data.context.textShaderProgram);
    createShaderBuffers(data.context.textVAO, data.context.textVBO);

    createProgram(forestVertexShaderSource, forestFragmentShaderSource,
                  data.context.forestShaderProgram);
    createShaderBuffers(data.context.forestVAO, data.context.forestVBO);

    // 2) once‐only GL setup & load textures/surfaces
    WindowParams wp = compute_window_params(data.fullscreen);
    glViewport((wp.windowWidth - wp.viewportSize) / 2, (wp.windowHeight - wp.viewportSize) / 2,
               wp.viewportSize, wp.viewportSize);

    create_textures(data.imageData.textures, data.dataPath);
    create_surfaces(data.imageData.surfaces, data.dataPath);

    data.gameMode = MENU;
    data.lastTick = SDL_GetTicks();
    data.totalElapsed = 0;
    data.initialized = true;
  }

  // 3) one frame of update & draw
  run_game_frame(data);
}

int main(int argc, char** argv) {
  srand(time(NULL));
  if (argc > 1 && std::strcmp(argv[1], "--smoke") == 0) {
    std::cout << "Smoketest ran fine!\n";
    return 0;
  }

  GameLoopData data{};
  data.fullscreen = (argc > 1 && std::strcmp(argv[1], "--fullscreen") == 0);
  data.shouldExit = false;
  data.initialized = false;

  // fetch resources directory once
  data.dataPath = getResourcePath();

  if (!initEngine(data.context, data.dataPath, data.fullscreen)) {
    return 1;
  }

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg([](void* d) { main_loop_iteration(static_cast<GameLoopData*>(d)); },
                               &data, 0, true);
#else
  while (!data.shouldExit) {
    main_loop_iteration(&data);
  }
#endif

  // clean up everything
  shutdownEngine(data.context);
  return 0;
}

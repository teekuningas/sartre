#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include <cstdlib>
#include <ctime>
#define SDL_MAIN_HANDLED
#include <cstring>  // for strcmp
#include <iostream>

#include "constants.h"  // MAP_WIDTH, etc.
#include "engine.h"     // initEngine, shutdownEngine
#include "game.h"       // run_game_frame
#include "graphics.h"   // now carries all of those routines
#include "shaders.h"    // Shader source code

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <limits.h>  // for PATH_MAX
#endif

static std::string getResourcePath() {
  const char* envPath = std::getenv("SARTRE_DATA_PATH");
  if (envPath) {
    printf("Reading data from path: %s\n", envPath);
    return std::string(envPath) + "/";
  }
#ifdef __APPLE__
  CFBundleRef mainBundle = CFBundleGetMainBundle();
  if (mainBundle) {
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8*)path, PATH_MAX)) {
      CFRelease(resourcesURL);
      return std::string(path) + "/data/";
    }
    CFRelease(resourcesURL);
  }
  return "./data/";
#else
  return "./data/";
#endif
}

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

    createProgram(forestVertexShaderSource, forestFragmentShaderSource,
                  data.context.forestShaderProgram);
    // cache forest‐shader uniforms
    data.context.forestLocProjection =
        glGetUniformLocation(data.context.forestShaderProgram, "projection");
    data.context.forestLocModel = glGetUniformLocation(data.context.forestShaderProgram, "model");
    data.context.forestLocOurTexture =
        glGetUniformLocation(data.context.forestShaderProgram, "ourTexture");
    data.context.forestLocNausea =
        glGetUniformLocation(data.context.forestShaderProgram, "u_nausea");
    data.context.forestLocBliss = glGetUniformLocation(data.context.forestShaderProgram, "u_bliss");
    data.context.forestLocBeams = glGetUniformLocation(data.context.forestShaderProgram, "u_beams");
    data.context.forestLocWarpTime =
        glGetUniformLocation(data.context.forestShaderProgram, "u_warpTime");
    data.context.forestLocBeamTime =
        glGetUniformLocation(data.context.forestShaderProgram, "u_beamTime");
    createShaderBuffers(data.context.forestVAO, data.context.forestVBO);

    createProgram(textVertexShaderSource, textFragmentShaderSource, data.context.textShaderProgram);
    data.context.textLocProjection =
        glGetUniformLocation(data.context.textShaderProgram, "projection");
    // we render text always on texture unit 0, so fix the sampler here:
    glUseProgram(data.context.textShaderProgram);
    glUniform1i(glGetUniformLocation(data.context.textShaderProgram, "textTexture"), 0);
    createShaderBuffers(data.context.textVAO, data.context.textVBO);

    // 2) once‐only GL setup & load textures/surfaces
    WindowParams wp = compute_window_params(data.fullscreen);
    glViewport((wp.windowWidth - wp.viewportSize) / 2, (wp.windowHeight - wp.viewportSize) / 2,
               wp.viewportSize, wp.viewportSize);

    create_textures(data.imageData.textures, data.dataPath);
    create_surfaces(data.imageData.surfaces, data.dataPath);
    load_descriptions(data.imageData, data.dataPath);

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
  data.fullscreen = false;
  data.fastMode = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--fullscreen") == 0) data.fullscreen = true;
    if (std::strcmp(argv[i], "--fast") == 0) data.fastMode = true;
  }
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

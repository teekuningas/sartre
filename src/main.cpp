#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#define SDL_MAIN_HANDLED
#include <cstring>  // for strcmp
#include <iostream>

#include "engine.h"     // initEngine, shutdownEngine
#include "game.h"       // main_loop_iteration
#include "resources.h"  // getResourcePath()
#include "types.h"      // only for GameLoopData in main()

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

  // initialize SDL, GL, TTF, Mixer, window, font, music...
  if (!initEngine(data.context, data.dataPath, data.fullscreen)) {
    return 1;
  }

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg([](void* d) { main_loop_iteration(*static_cast<GameLoopData*>(d)); },
                               &data, 0, true);
#else
  while (!data.shouldExit) {
    main_loop_iteration(data);
  }
#endif

  // clean up everything
  shutdownEngine(data.context);
  return 0;
}

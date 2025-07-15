#define SDL_MAIN_HANDLED
#include <iostream>
#include <cstring>           // for strcmp
#include "engine.h"         // initEngine, shutdownEngine
#include "game.h"           // main_loop_iteration
#include "types.h"          // only for GameLoopData in main()

int main(int argc, char** argv) {
  srand(time(NULL));
  if (argc>1 && std::strcmp(argv[1],"--smoke")==0) {
    std::cout<<"Smoketest ran fine!\n";
    return 0;
  }

  GameLoopData data{};
  data.fullscreen = (argc>1 && std::strcmp(argv[1],"--fullscreen")==0);
  data.initialized = data.shouldExit = false;

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(
    [](void* d){ main_loop_iteration(*static_cast<GameLoopData*>(d)); },
    &data, 0, 0);
#else
  while (true) {
    main_loop_iteration(data);
  }
#endif
  return 0;
}

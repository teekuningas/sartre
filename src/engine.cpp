#include "engine.h"

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <iostream>

WindowParams compute_window_params(bool fullscreen) {
  SDL_DisplayMode DM;
  SDL_GetCurrentDisplayMode(0, &DM);
  int screenW = DM.w, screenH = DM.h;
  int vp = fullscreen ? std::min(screenW, screenH) : (std::min(screenW, screenH) * 3 / 4);
  WindowParams wp;
  wp.viewportSize = vp;
  wp.windowWidth = fullscreen ? screenW : vp;
  wp.windowHeight = fullscreen ? screenH : vp;
  return wp;
}

void shutdownEngine(RenderContext& context) {
  if (context.backgroundMusic) {
    Mix_FreeMusic(context.backgroundMusic);
  }
  Mix_CloseAudio();
  Mix_Quit();

  if (context.font) {
    TTF_CloseFont(context.font);
  }
  TTF_Quit();

  glDeleteVertexArrays(1, &context.forestVAO);
  glDeleteBuffers(1, &context.forestVBO);
  glDeleteVertexArrays(1, &context.textVAO);
  glDeleteBuffers(1, &context.textVBO);
  glDeleteProgram(context.forestShaderProgram);
  glDeleteProgram(context.textShaderProgram);

  if (context.glContext) {
    SDL_GL_DeleteContext(context.glContext);
  }
  if (context.window) {
    SDL_DestroyWindow(context.window);
  }

  SDL_Quit();
}

bool initEngine(RenderContext& context, const std::string& dataPath, bool fullscreen) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    printf("Error: SDL_Init: %s\n", SDL_GetError());
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  WindowParams wp = compute_window_params(fullscreen);
  int windowWidth = wp.windowWidth;
  int windowHeight = wp.windowHeight;

  Uint32 windowFlags = SDL_WINDOW_OPENGL;
  if (fullscreen) {
    windowFlags |= SDL_WINDOW_FULLSCREEN;
  }

  SDL_Window* window =
      SDL_CreateWindow("Sartre lehdossa inhottavien asioiden ympäroimänä", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, windowFlags);

  if (!window) {
    printf("Error: SDL_CreateWindow: %s\n", SDL_GetError());
    return false;
  }
  context.window = window;

  SDL_GLContext glContext = SDL_GL_CreateContext(window);
  if (!glContext) {
    printf("Error: SDL_GL_CreateContext: %s\n", SDL_GetError());
    return false;
  }
  context.glContext = glContext;

#ifndef __EMSCRIPTEN__
  GLenum glewStatus = glewInit();
  if (glewStatus != GLEW_OK) {
    printf("Error: glewInit failed: %s\n", glewGetErrorString(glewStatus));
    return false;
  }
  printf("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

  printf("Status: OpenGL version supported by this platform (%s)\n", glGetString(GL_VERSION));

  if (TTF_Init() == -1) {
    printf("SDL could not initialize! SDL_Error: %s\n", TTF_GetError());
    return false;
  }

  TTF_Font* font = TTF_OpenFont((dataPath + "fonts/Roboto-Regular.ttf").c_str(), 50);
  if (font == nullptr) {
    printf("Fonts could not be initialized. TTF_OpenFont Error: %s\n", TTF_GetError());
    TTF_Quit();
    return false;
  }
  context.font = font;

  // Music
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return false;
  }

  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
    return false;
  }

  Mix_Music* backgroundMusic = Mix_LoadMUS((dataPath + "music/music.ogg").c_str());
  if (backgroundMusic == NULL) {
    printf("Failed to load background music! SDL_mixer Error: %s\n", Mix_GetError());
    return false;
  }
  context.backgroundMusic = backgroundMusic;

  return true;
}

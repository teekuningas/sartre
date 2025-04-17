#ifndef TYPES_H
#define TYPES_H

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <vector>

enum GameMode { MENU, FOREST, RESULTS, EXIT };

struct PageObject {
  GLfloat x;
  GLfloat y;
  GLfloat vx;
  GLfloat ymid;
  GLfloat width;
  GLfloat height;
  GLfloat phase;
  GLfloat amplitude;
  GLfloat frequency;
  int animIdx;
  int animSize;
  bool collected;  // Added flag
};

struct Sartre {
  GLfloat x;
  GLfloat y;
  GLfloat width;
  GLfloat height;
  GLfloat vy;
  int animIdx;
  int animSize;
  bool jump;
};

struct GameStateForest {
  Sartre sartre;
  std::vector<PageObject> pages;
  int collectedPages;
  int totalPages;
};

struct GameStateMenu {};

struct GameStateResults {};

struct InputResult {
  bool transition;
  GameMode transitionTo;
};

struct Textures {
  GLuint forestSartre[2];
  GLuint forestPage[2];
  GLuint forestTausta[1];
};

struct Surfaces {
  SDL_Surface* forestCollisionMap;
};

struct ImageData {
  Textures textures;
  Surfaces surfaces;
};

struct RenderContext {
  SDL_Window* window = nullptr;
  SDL_GLContext glContext = nullptr;
  TTF_Font* font = nullptr;
  Mix_Music* backgroundMusic = nullptr;

  GLuint forestVAO;
  GLuint forestVBO;
  GLuint forestShaderProgram;
  GLuint textVAO;
  GLuint textVBO;
  GLuint textShaderProgram;
};

struct WindowParams {
  int windowHeight;
  int windowWidth;
  int viewportSize;
};

struct GameLoopData {
  GameMode gameMode;
  RenderContext context;
  GameStateMenu gameStateMenu;
  GameStateForest gameStateForest;
  GameStateResults gameStateResults;
  ImageData imageData;
  Uint32 lastTick;
  Uint32 currentTick;
  Uint32 totalElapsed;
  bool fullscreen;
  bool shouldExit;
  bool initialized;
};

#endif

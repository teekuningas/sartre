#ifndef TYPES_H
#define TYPES_H

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <string>
#include <vector>

enum GameMode { MENU, FOREST, RESULTS, EXIT };

enum GameObjectType { PAGE, CHESTNUT, PIPE };

struct GameObject {
  GLfloat x;
  GLfloat y;
  GLfloat vx;
  GLfloat ymid;
  GLfloat width;
  GLfloat height;
  GLfloat phase;
  GLfloat amplitude;
  GLfloat frequency;
  bool collected;
  Uint32 collectedAt;  // <— time when we picked it up, used to delay respawn
  GameObjectType type;
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
  std::vector<GameObject> objects;
  int pages_collected;
  int nausea_hits;
};

struct GameStateMenu {};

struct GameStateResults {};

struct InputResult {
  bool transition;
  GameMode transitionTo;
};

struct Textures {
  GLuint forestSartre[2];
  GLuint forestPage;
  GLuint forestChestnut;
  GLuint forestPipe;
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
  Mix_Chunk* scribbleSound = nullptr;
  Mix_Chunk* nauseaSound = nullptr;

  GLuint forestVAO;
  GLuint forestVBO;

  // ---- forest shader (cached)
  GLuint  forestShaderProgram;
  GLint   forestLocProjection;
  GLint   forestLocModel;
  GLint   forestLocOurTexture;
  GLint   forestLocNausea;
  GLint   forestLocTime;

  GLuint textVAO;
  GLuint textVBO;

  // ---- text shader (cached)
  GLuint  textShaderProgram;
  GLint   textLocProjection;
  GLint   textLocTextTexture;
  GLint   textLocTextColor;

  // ---- dynamic text textures (only two)
  GLuint  textTexturePages   = 0;
  GLuint  textTextureNausea  = 0;
  int     lastPagesRendered  = -1;
  int     lastNauseaRendered = -1;
  int     textW_pages        = 0, textH_pages = 0;
  int     textW_nausea       = 0, textH_nausea = 0;
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
  std::string dataPath;
};

#endif

#ifndef TYPES_H
#define TYPES_H

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

enum GameMode { MENU, FOREST, EXIT };

enum GameObjectType { PAGE, CHESTNUT, PIPE, BEER, CLOCK };

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

enum DescriptionEventType { TEXT, WAIT };

struct DescriptionEvent {
  DescriptionEventType type;
  std::string text;
  Uint32 duration;  // milliseconds
};

struct GameStateForest {
  Sartre sartre;
  std::vector<GameObject> objects;
  int pages_collected;
  int nausea_hits;
  float warpTime;
  float beamTime;
  GLfloat speedFactor;
  bool fastMode;

  // Description queue system
  std::queue<DescriptionEvent> descriptionQueue;
  std::string activeDescription;
  Uint32 descriptionEndTime;
  Uint32 descriptionStartTime;
  float textAlpha;
  std::unordered_map<int, bool> itemSeen;
  int lastPageMilestone;

  int pageGoal;
  int milestoneStep;

  // Ending sequence
  bool endingMode;
  bool endingSuccess;
  Uint32 endingStartTime;
  Sartre endingStartPos;
};

struct GameStateMenu {
  std::string cheatSequence;
  Uint32 lastCheatTime;
  bool cheatActive;
};

struct InputResult {
  bool transition;
  GameMode transitionTo;
  bool confirm;
};

struct Textures {
  GLuint forestSartre[2];
  GLuint forestPage;
  GLuint forestChestnut;
  GLuint forestPipe;
  GLuint forestBeer;
  GLuint forestClock;
  GLuint forestTausta[1];
};

struct Surfaces {
  SDL_Surface* forestCollisionMap;
};

struct ImageData {
  Textures textures;
  Surfaces surfaces;
  std::unordered_map<int, std::string> itemDescriptions;
  std::vector<std::string> pageDescriptions;
};

struct RenderContext {
  SDL_Window* window = nullptr;
  SDL_GLContext glContext = nullptr;
  TTF_Font* font = nullptr;
  Mix_Chunk* scribbleSound = nullptr;
  Mix_Chunk* nauseaSound = nullptr;

  GLuint forestVAO;
  GLuint forestVBO;

  // ---- forest shader (cached)
  GLuint forestShaderProgram;
  GLint forestLocProjection;
  GLint forestLocModel;
  GLint forestLocOurTexture;
  GLint forestLocNausea;
  GLint forestLocBliss;
  GLint forestLocBeams;
  GLint forestLocWarpTime;
  GLint forestLocBeamTime;

  GLuint textVAO;
  GLuint textVBO;

  // ---- text shader (cached)
  GLuint textShaderProgram;
  GLint textLocProjection;

  struct TextCacheEntry {
    GLuint texture;
    int w, h;
  };
  std::unordered_map<std::string, TextCacheEntry> textCache;
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
  ImageData imageData;
  Uint32 lastTick;
  Uint32 currentTick;
  Uint32 totalElapsed;
  bool fullscreen;
  bool fastMode;
  bool shouldExit;
  bool initialized;
  std::string dataPath;
};

#endif

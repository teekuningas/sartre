#include "game.h"

#include <cmath>  // for fabs()

// --- AABB overlap test with optional extra margin on both axes ---
static bool aabbOverlap(GLfloat x1, GLfloat y1, GLfloat w1, GLfloat h1, GLfloat x2, GLfloat y2,
                        GLfloat w2, GLfloat h2, float extra = 0.0f) {
  float halfX = w1 * 0.5f + w2 * 0.5f + extra;
  float halfY = h1 * 0.5f + h2 * 0.5f + extra;
  return fabs(x1 - x2) < halfX && fabs(y1 - y2) < halfY;
}

#include "constants.h"
#include "engine.h"
#include "graphics.h"

// collision-map helper (forward-declare so forest_update can call it)
static bool isPixelBlack(SDL_Surface *surface, int x, int y);

// forward‐declared so forest_update can see it
static bool update_game_object(GameObject &obj, Sartre &sartre, GameStateForest &gameStateForest,
                               RenderContext &context, ImageData &imageData, Uint32 totalElapsed,
                               std::vector<GameObject> &newObjects);

// --- update loop for the FOREST state ---
void forest_update(GameStateForest &gameStateForest, RenderContext &context, Uint32 totalElapsed,
                   float deltaTime, ImageData &imageData, InputResult &inputResult) {
  // advance our warp‐phase at 2 radians/sec, keep it in [0,2π)
  gameStateForest.warpTime += deltaTime * 2.0f;
  if (gameStateForest.warpTime >= 6.28318530718f) gameStateForest.warpTime -= 6.28318530718f;

  // 1) tick all objects (they may play SFX on collision):
  std::vector<GameObject> newObjects;
  Sartre &sartre = gameStateForest.sartre;
  for (auto it = gameStateForest.objects.begin(); it != gameStateForest.objects.end();) {
    if (update_game_object(*it, sartre, gameStateForest, context, imageData, totalElapsed,
                           newObjects)) {
      it = gameStateForest.objects.erase(it);
    } else {
      ++it;
    }
  }
  gameStateForest.objects.insert(gameStateForest.objects.end(), newObjects.begin(),
                                 newObjects.end());

  // 2) advance Sartre’s animation
  sartre.animIdx = (totalElapsed % 1000) / (1000 / sartre.animSize);

  // 3) read keyboard for left/right/jump
  const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  if (keystate[SDL_SCANCODE_RIGHT] && sartre.x < MAP_WIDTH / 2 - sartre.width / 2) {
    sartre.x += deltaTime * SARTRE_VX;
  }
  if (keystate[SDL_SCANCODE_LEFT] && sartre.x > -MAP_WIDTH / 2 + sartre.width / 2) {
    sartre.x -= deltaTime * SARTRE_VX;
  }
  if (!sartre.jump && keystate[SDL_SCANCODE_UP]) {
    sartre.jump = true;
    sartre.vy = SARTRE_JUMP_VELOCITY;
  }

  // 4) gravity & platform collision
  float predictedY = sartre.y + deltaTime * sartre.vy;
  int px = int(sartre.x + MAP_WIDTH / 2);
  int py = int(sartre.y - sartre.height / 2 + sartre.height / 8);
  int pyp = int(predictedY - sartre.height / 2 + sartre.height / 8 - 2);

  // hit the ground?
  if (sartre.y >= sartre.height / 2 + EARTH_HEIGHT &&
      predictedY < sartre.height / 2 + EARTH_HEIGHT) {
    sartre.jump = false;
    sartre.vy = 0;
  }
  // hit a platform from above?
  else if (predictedY < sartre.y &&
           !isPixelBlack(imageData.surfaces.forestCollisionMap, px, MAP_HEIGHT - py) &&
           isPixelBlack(imageData.surfaces.forestCollisionMap, px, MAP_HEIGHT - pyp)) {
    sartre.jump = false;
    sartre.vy = 0;
  } else {
    sartre.y = predictedY;
    sartre.vy = sartre.vy - deltaTime * SARTRE_G;
  }

  // 5) end‐of‐frame: transition if too many nasty collisions OR enough pages
  if (gameStateForest.nausea_hits >= NUM_NAUSEA_LIMIT ||
      gameStateForest.pages_collected >= gameStateForest.pageGoal) {
    inputResult.transition = true;
    inputResult.transitionTo = RESULTS;
  }
}

// A minimal in‐file helper, used by forest_update for collision‐map lookups:
static bool isPixelBlack(SDL_Surface *surface, int x, int y) {
  const Uint8 threshold = 50;
  if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) {
    return false;
  }
  Uint32 offset = y * surface->pitch + x * 4;
  Uint8 *pixel = static_cast<Uint8 *>(surface->pixels) + offset;
  return pixel[0] < threshold && pixel[1] < threshold && pixel[2] < threshold;
}

//
// now takes currentElapsed (in ms) so we can align the sine‐wave
static void spawn_object_avoiding_sartre(GameObject &obj, Sartre const &sartre, GameObjectType type,
                                         Uint32 currentElapsed, GLfloat speedFactor) {
  obj.type = type;
  // we’ll keep trying random x|ymid|phase until the *actual* first‐frame y
  // (ymid + A·sin(ω*(t+phase))) does not overlap Sartre.
  const float spawnMargin = obj.width * 0.5f;
  float candX, candYmid, candVx, candPhase, candY;
  do {
    // 1) pick a midpoint
    candX = (GLfloat)((rand() % (MAP_WIDTH - (int)obj.width)) - (MAP_WIDTH / 2) + obj.width / 2);
    candYmid = (GLfloat)((rand() % (MAP_HEIGHT - (int)obj.height - (MAP_HEIGHT / 4))) +
                         (MAP_HEIGHT / 8) + obj.height / 2);
    // 2) pick speed & sine phase
    candVx =
        ((((float)(rand() % 1000)) / 1000.0f) * 1.5f + 0.5f) * 0.3f * (rand() % 2 ? 1.0f : -1.0f);
    candPhase = (((float)(rand() % 1000)) / 1000.0f) * 2.0f * 3.14159265f;
    // 3) compute where it *will* actually draw on this frame
    candY = candYmid + obj.amplitude * sinf(obj.frequency * (currentElapsed / 1000.0f + candPhase));
  } while (aabbOverlap(candX, candY, obj.width, obj.height, sartre.x, sartre.y, sartre.width,
                       sartre.height, spawnMargin));
  // now commit
  obj.x = candX;
  obj.ymid = candYmid;
  obj.vx = candVx;
  obj.phase = candPhase;
  obj.y = candY;
  obj.collected = false;
  obj.collectedAt = 0;
}

void run_game_frame(GameLoopData &data) {
  Uint32 now = SDL_GetTicks();
  float delta = (now - data.lastTick) / 1000.0f;
  data.totalElapsed += (now - data.lastTick);
  data.lastTick = now;

  InputResult inputResult{false, data.gameMode};
  switch (data.gameMode) {
    case MENU:
      menu_update(data.gameStateMenu, data.totalElapsed, delta, data.imageData.surfaces,
                  inputResult);
      break;
    case FOREST:
      forest_update(data.gameStateForest, data.context, data.totalElapsed, delta, data.imageData,
                    inputResult);
      break;
    case RESULTS:
      results_update(data.gameStateResults, data.totalElapsed, delta, data.imageData.surfaces,
                     inputResult);
      break;
    default:
      break;
  }

  handle_events(data.gameMode, data.fullscreen, inputResult);

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
      forest_init(data.gameStateForest, data.imageData, data.fastMode);
    } else {
      Mix_HaltMusic();
    }
    if (inputResult.transitionTo == MENU) {
      menu_init(data.gameStateMenu);
    }
    if (inputResult.transitionTo == RESULTS) {
      data.gameStateResults.pages_collected = data.gameStateForest.pages_collected;
      data.gameStateResults.success =
          (data.gameStateResults.pages_collected >= data.gameStateForest.pageGoal);
      results_init(data.gameStateResults);
    }
    data.gameMode = inputResult.transitionTo;
    return;  // do not draw this frame if you just transitioned
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Re-set the GL viewport each mode so MENU/RESULTS fill the window
  int winW, winH;
  SDL_GetWindowSize(data.context.window, &winW, &winH);
  switch (data.gameMode) {
    case MENU:
      // full-screen for the menu
      glViewport(0, 0, winW, winH);
      menu_draw(data.context, data.imageData.textures);
      break;

    case FOREST: {
      // letter-box a centered square for the 2D forest
      int side = std::min(winW, winH);
      glViewport((winW - side) / 2, (winH - side) / 2, side, side);
      forest_draw(data.gameStateForest, data.imageData.textures, data.context,
                  data.context.forestShaderProgram, data.context.forestVAO, data.context.forestVBO);
      break;
    }

    case RESULTS:
      // full-screen for the results screen
      glViewport(0, 0, winW, winH);
      results_draw(data.context, data.imageData.textures, data.totalElapsed, data.gameStateResults);
      break;

    default:
      break;
  }

  SDL_GL_SwapWindow(data.context.window);
#ifndef __EMSCRIPTEN__
  SDL_Delay(1);
#endif
}

void handle_events(GameMode &gameMode, bool fullscreen, InputResult &inputResult) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        inputResult.transitionTo = EXIT;
        inputResult.transition = true;
        break;

      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          if (!fullscreen) {
            int windowWidth = event.window.data1;
            int windowHeight = event.window.data2;
            int viewportSize = std::min(windowWidth, windowHeight);
            glViewport((windowWidth - viewportSize) / 2, (windowHeight - viewportSize) / 2,
                       viewportSize, viewportSize);
          }
        }
        break;
      case SDL_KEYUP:
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          if (gameMode == FOREST) {
            inputResult.transitionTo = RESULTS;
            inputResult.transition = true;
          } else {
            inputResult.transitionTo = EXIT;
            inputResult.transition = true;
          }
        } else if (event.key.keysym.sym == SDLK_RETURN) {
          if (gameMode == MENU) {
            inputResult.transitionTo = FOREST;
            inputResult.transition = true;
          } else if (gameMode == RESULTS) {
            inputResult.transitionTo = MENU;
            inputResult.transition = true;
          }
        }
        break;
    }
  }
}

void init_game_object(GameObject &obj, GameObjectType type) {
  obj.width = GAME_OBJECT_WIDTH;
  obj.height = GAME_OBJECT_HEIGHT;
  obj.x =
      (GLfloat)((rand() % (MAP_WIDTH - (int)obj.width)) - (MAP_WIDTH / 2) + (int)(obj.width / 2));
  obj.y = (GLfloat)((rand() % (MAP_HEIGHT - (int)obj.height - (MAP_HEIGHT / 4))) +
                    (MAP_HEIGHT / 8) + (int)(obj.height / 2));
  obj.vx =
      ((((GLfloat)(rand() % 1000)) / 1000.0f) * 1.5f + 0.5f) * 0.3f * (rand() % 2 == 0 ? 1 : -1);
  obj.ymid = obj.y;
  obj.amplitude = GAME_OBJECT_AMPLITUDE;
  obj.frequency = GAME_OBJECT_FREQUENCY;
  obj.phase = (((GLfloat)(rand() % 1000)) / 1000.0f) * 3.141 * 2;
  obj.collected = false;
  obj.type = type;
}

void forest_init(GameStateForest &gameStateForest, ImageData const &imageData, bool fastMode) {
  Sartre &sartre = gameStateForest.sartre;
  sartre.width = SARTRE_WIDTH;
  sartre.height = SARTRE_HEIGHT;
  sartre.x = 0.0;
  sartre.y = sartre.height / 2 + EARTH_HEIGHT;
  sartre.animIdx = 0;
  sartre.animSize = 2;
  sartre.jump = 0;
  sartre.vy = 0.0f;

  gameStateForest.pageGoal = fastMode ? (PAGE_GOAL / 5) : PAGE_GOAL;
  gameStateForest.milestoneStep = fastMode ? (60 / 5) : 60;
  gameStateForest.fastMode = fastMode;

  gameStateForest.objects.resize(NUM_PAGES + INITIAL_NUM_NAUSEOUS_OBJECTS);
  gameStateForest.pages_collected = 0;
  gameStateForest.nausea_hits = 0;
  gameStateForest.warpTime = 0.0f;  // ← init here
  gameStateForest.speedFactor = INITIAL_SPEED_FACTOR;

  gameStateForest.itemSeen.clear();
  gameStateForest.activeDescription = "";
  gameStateForest.descriptionEndTime = 0;
  gameStateForest.lastPageMilestone = 0;

  // Show first page text at start
  if (!imageData.pageDescriptions.empty()) {
    gameStateForest.activeDescription = imageData.pageDescriptions[0];
    gameStateForest.descriptionEndTime = SDL_GetTicks() + 8000;  // Show for 8 seconds
  }

  for (int i = 0; i < NUM_PAGES; ++i) {
    init_game_object(gameStateForest.objects[i], PAGE);
    spawn_object_avoiding_sartre(gameStateForest.objects[i], gameStateForest.sartre, PAGE,
                                 /* currentElapsed = */ 0u, gameStateForest.speedFactor);
  }
  for (int i = NUM_PAGES; i < NUM_PAGES + INITIAL_NUM_NAUSEOUS_OBJECTS; ++i) {
    GameObjectType types[] = {CHESTNUT, PIPE, BEER, CLOCK};
    GameObjectType t = types[rand() % 4];
    init_game_object(gameStateForest.objects[i], t);
    spawn_object_avoiding_sartre(gameStateForest.objects[i], gameStateForest.sartre, t,
                                 /* currentElapsed = */ 0u, gameStateForest.speedFactor);
  }
}

void draw_textured_quad(float x, float y, float width, float height, GLuint texture,
                        GLuint modelLoc, GLuint VBO) {
  glBindTexture(GL_TEXTURE_2D, texture);

  float translationMatrix[16];
  createTranslationMatrix(x, y, 0.1f, translationMatrix);
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, translationMatrix);

  float vertices[] = {-width / 2, height / 2,  0.01f,     -0.99f,      width / 2, height / 2,
                      0.99f,      -0.99f,      width / 2, -height / 2, 0.99f,     0.01f,
                      -width / 2, -height / 2, 0.01f,     0.01f};
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void forest_draw(GameStateForest &gameStateForest, Textures &textures, RenderContext &context,
                 GLuint shaderProgram, GLuint VAO, GLuint VBO) {
  // Use the forest shader program
  glUseProgram(context.forestShaderProgram);

  // Set up the orthographic projection
  float orthoMatrix[16];
  createOrthographicMatrix(-MAP_WIDTH / 2, MAP_WIDTH / 2, 0.0f, MAP_HEIGHT, -100.0f, 100.0f,
                           orthoMatrix);

  // 1) upload projection & model only via cached locations
  glUniformMatrix4fv(context.forestLocProjection, 1, GL_FALSE, orthoMatrix);
  // model will be set per‐quad in draw_textured_quad

  // 2) texture unit & nausea/time
  glUniform1i(context.forestLocOurTexture, 0);
  float nauseaLevel = float(gameStateForest.nausea_hits) / float(NUM_NAUSEA_LIMIT);
  glUniform1f(context.forestLocNausea, nauseaLevel);
  float progress = float(gameStateForest.pages_collected) / float(gameStateForest.pageGoal);
  float blissLevel = progress;
  glUniform1f(context.forestLocBliss, blissLevel);
  glUniform1f(context.forestLocTime, gameStateForest.warpTime);

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  // Bind the VAO
  glBindVertexArray(context.forestVAO);

  // Draw the Sartre character
  Sartre &sartre = gameStateForest.sartre;
  glUniform1f(context.forestLocBliss, blissLevel);
  GLuint sartreTexture = textures.forestSartre[sartre.animIdx];
  draw_textured_quad(sartre.x, sartre.y, sartre.width, sartre.height, sartreTexture,
                     context.forestLocModel, context.forestVBO);

  // Draw items with 0 bliss
  glUniform1f(context.forestLocBliss, 0.0f);
  for (auto &obj : gameStateForest.objects) {
    if (obj.collected) {
      continue;
    }

    GLuint objTexture;
    switch (obj.type) {
      case PAGE:
        objTexture = textures.forestPage;
        break;
      case CHESTNUT:
        objTexture = textures.forestChestnut;
        break;
      case PIPE:
        objTexture = textures.forestPipe;
        break;
      case BEER:
        objTexture = textures.forestBeer;
        break;
      case CLOCK:
        objTexture = textures.forestClock;
        break;
    }
    draw_textured_quad(obj.x, obj.y, obj.width, obj.height, objTexture, context.forestLocModel,
                       context.forestVBO);
  }

  // Draw the Background
  float translationMatrix[16];
  createTranslationMatrix(0.0f, 0.0f, 0.0f, translationMatrix);
  glUniformMatrix4fv(context.forestLocModel, 1, GL_FALSE, translationMatrix);

  glBindTexture(GL_TEXTURE_2D, textures.forestTausta[0]);
  float backgroundVertices[] = {
      -MAP_WIDTH / 2, MAP_HEIGHT, 0.0f, -1.0f, MAP_WIDTH / 2,  MAP_HEIGHT, 1.0f, -1.0f,
      MAP_WIDTH / 2,  0.0f,       1.0f, 0.0f,  -MAP_WIDTH / 2, 0.0f,       0.0f, 0.0f};
  glBindBuffer(GL_ARRAY_BUFFER, context.forestVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(backgroundVertices), backgroundVertices);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  // Unbind the VAO and texture
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Disable depth test not to distract others
  glDisable(GL_DEPTH_TEST);

  // --- Render Page Count Text and Nausea Text (using our standard text pipeline) ---
  float textOrtho[16];
  createOrthographicMatrix(0.0f, MAP_WIDTH, 0.0f, MAP_HEIGHT, -1.0f, 1.0f, textOrtho);
  glUseProgram(context.textShaderProgram);
  glUniformMatrix4fv(context.textLocProjection, 1, GL_FALSE, textOrtho);

  // Draw dark backgrounds for text
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GLint textColorLoc = glGetUniformLocation(context.textShaderProgram, "textColor");
  glBindVertexArray(context.textVAO);
  glBindBuffer(GL_ARRAY_BUFFER, context.textVBO);

  // Background for page count
  int tw, th;
  std::string pageCountText =
      std::string("Kirjoitettuja sivuja: ") + std::to_string(gameStateForest.pages_collected);
  getTextSize(context.font, pageCountText, 0, tw, th);
  float padding = 20.0f;

  glUniform4f(textColorLoc, 0.0f, 0.0f, 0.0f, 0.5f);
  float bgPageCount[] = {50.0f - padding,      MAP_HEIGHT - 50.0f + padding,      0.0f, 0.0f,
                         50.0f + tw + padding, MAP_HEIGHT - 50.0f + padding,      1.0f, 0.0f,
                         50.0f + tw + padding, MAP_HEIGHT - 50.0f - th - padding, 1.0f, 1.0f,
                         50.0f - padding,      MAP_HEIGHT - 50.0f - th - padding, 0.0f, 1.0f};
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bgPageCount), bgPageCount);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  // Background for description (if active)
  if (!gameStateForest.activeDescription.empty() &&
      SDL_GetTicks() < gameStateForest.descriptionEndTime) {
    int dw, dh;
    getTextSize(context.font, gameStateForest.activeDescription, 60, dw, dh);

    glUniform4f(textColorLoc, 0.0f, 0.0f, 0.0f, 0.6f);
    float bgDesc[] = {100.0f - padding,      MAP_HEIGHT - 250.0f + padding,      0.0f, 0.0f,
                      100.0f + dw + padding, MAP_HEIGHT - 250.0f + padding,      1.0f, 0.0f,
                      100.0f + dw + padding, MAP_HEIGHT - 250.0f - dh - padding, 1.0f, 1.0f,
                      100.0f - padding,      MAP_HEIGHT - 250.0f - dh - padding, 0.0f, 1.0f};
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bgDesc), bgDesc);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  }
  glDisable(GL_BLEND);

  SDL_Color white = {255, 255, 255, 255};
  renderText(context, context.font, pageCountText, white, context.textShaderProgram,
             context.textVAO, context.textVBO, 50.0f, MAP_HEIGHT - 50.0f, 0);

  if (!gameStateForest.activeDescription.empty() &&
      SDL_GetTicks() < gameStateForest.descriptionEndTime) {
    renderText(context, context.font, gameStateForest.activeDescription, white,
               context.textShaderProgram, context.textVAO, context.textVBO, 100.0f,
               MAP_HEIGHT - 250.0f, 60);
  }
}

static bool update_game_object(GameObject &obj, Sartre &sartre, GameStateForest &gameStateForest,
                               RenderContext &context, ImageData &imageData, Uint32 totalElapsed,
                               std::vector<GameObject> &newObjects) {
  // if it's already collected, wait 1 second then respawn:
  if (obj.collected) {
    if (totalElapsed - obj.collectedAt >= 1000) {
      // only pages respawn
      if (obj.type == PAGE) {
        spawn_object_avoiding_sartre(obj, sartre, PAGE, totalElapsed, gameStateForest.speedFactor);
      } else {
        return true;  // remove from game
      }
    }
    return false;  // do nothing yet
  }

  // 1) move along the sine-wave trajectory
  obj.x += obj.vx * gameStateForest.speedFactor;
  obj.y = obj.ymid + obj.amplitude * sinf(obj.frequency * (totalElapsed / 1000.0f + obj.phase)) *
                         gameStateForest.speedFactor;

  // 2) wrap‐around logic …
  if (obj.x > MAP_WIDTH / 2 + obj.width && obj.vx > 0)
    obj.x = -MAP_WIDTH / 2 - obj.width / 2;
  else if (obj.x < -MAP_WIDTH / 2 - obj.width && obj.vx < 0)
    obj.x = MAP_WIDTH / 2 + obj.width / 2;

  // 3) collision test
  // unified AABB test, no extra margin now that we’re detecting real collisions
  if (aabbOverlap(obj.x, obj.y, obj.width, obj.height, sartre.x, sartre.y, sartre.width,
                  sartre.height)) {
    obj.collected = true;
    obj.collectedAt = totalElapsed;  // start 1 second timer

    // ── play the appropriate collision sound
    if (obj.type == PAGE) {
      Mix_PlayChannel(-1, context.scribbleSound, 0);
    } else {
      Mix_PlayChannel(-1, context.nauseaSound, 0);
    }

    if (obj.type == PAGE) {
      gameStateForest.pages_collected++;
      gameStateForest.speedFactor += SPEED_INCREMENT_PER_PAGE;

      // Page milestones for descriptions
      int milestone = gameStateForest.pages_collected / gameStateForest.milestoneStep;
      if (milestone > gameStateForest.lastPageMilestone &&
          milestone < (int)imageData.pageDescriptions.size()) {
        gameStateForest.activeDescription = imageData.pageDescriptions[milestone];
        gameStateForest.descriptionEndTime = SDL_GetTicks() + 8000;
        gameStateForest.lastPageMilestone = milestone;
      }

      float baseProbability = gameStateForest.fastMode ? NAUSEA_SPAWN_PROBABILITY_FAST : NAUSEA_SPAWN_PROBABILITY_NORMAL;
      if (((float)rand() / RAND_MAX) < baseProbability) {
        GameObject newNauseousObject;
        GameObjectType types[] = {CHESTNUT, PIPE, BEER, CLOCK};
        init_game_object(newNauseousObject, types[rand() % 4]);
        spawn_object_avoiding_sartre(newNauseousObject, sartre, newNauseousObject.type,
                                     totalElapsed, gameStateForest.speedFactor);
        newObjects.push_back(newNauseousObject);
      }
    } else {
      gameStateForest.nausea_hits++;

      // First time hit description
      if (!gameStateForest.itemSeen[obj.type]) {
        if (imageData.itemDescriptions.count(obj.type)) {
          gameStateForest.activeDescription = imageData.itemDescriptions[obj.type];
          gameStateForest.descriptionEndTime = SDL_GetTicks() + 8000;
        }
        gameStateForest.itemSeen[obj.type] = true;
      }

      return true;  // remove from game
    }
  }
  return false;  // keep in game
}

void results_init(GameStateResults &gameStateResults) {}

void results_draw(RenderContext &context, Textures &textures, Uint32 totalElapsed,
                  GameStateResults const &state) {
  // 0) one orthographic for full-screen quads
  float ortho[16];
  createOrthographicMatrix(-MAP_WIDTH / 2, MAP_WIDTH / 2, 0.0f, MAP_HEIGHT, -100.0f, 100.0f, ortho);

  // Add a 0..MAP_WIDTH, 0..MAP_HEIGHT ortho for text and overlays
  float textOrtho[16];
  createOrthographicMatrix(0.0f, MAP_WIDTH, 0.0f, MAP_HEIGHT, -1.0f, 1.0f, textOrtho);

  //
  // 1) Background: forest shader with warp on failure, calm on success
  //
  glUseProgram(context.forestShaderProgram);
  glUniformMatrix4fv(context.forestLocProjection, 1, GL_FALSE, ortho);
  glUniform1i(context.forestLocOurTexture, 0);
  glUniform1f(context.forestLocNausea, state.success ? 0.0f : 1.0f);
  glUniform1f(context.forestLocBliss, 0.0f);
  // wrap the time in [0,2π) to keep sin() fast & precise
  {
    const float TWO_PI = 6.28318530718f;
    float t = fmodf((totalElapsed / 1000.0f), TWO_PI);
    glUniform1f(context.forestLocTime, t);
  }
  // reset model to identity so our full‐screen quad really spans 0..MAP_HEIGHT
  {
    float identityModel[16];
    createTranslationMatrix(0.0f, 0.0f, 0.0f, identityModel);
    glUniformMatrix4fv(context.forestLocModel, 1, GL_FALSE, identityModel);
  }
  glBindVertexArray(context.forestVAO);
  glBindTexture(GL_TEXTURE_2D, textures.forestTausta[0]);
  float bgVerts[] = {-MAP_WIDTH / 2, MAP_HEIGHT, 0.0f,          -1.0f, MAP_WIDTH / 2, MAP_HEIGHT,
                     1.0f,           -1.0f,      MAP_WIDTH / 2, 0.0f,  1.0f,          0.0f,
                     -MAP_WIDTH / 2, 0.0f,       0.0f,          0.0f};
  glBindBuffer(GL_ARRAY_BUFFER, context.forestVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bgVerts), bgVerts);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glBindVertexArray(0);

  // ── DIMMER BACKGROUND ──
  // reuse text shader to draw a black, alpha = 0.3 full-screen quad
  glUseProgram(context.textShaderProgram);
  glUniformMatrix4fv(context.textLocProjection, 1, GL_FALSE, textOrtho);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GLint textColorLoc = glGetUniformLocation(context.textShaderProgram, "textColor");
  glUniform4f(textColorLoc, 0.0f, 0.0f, 0.0f, 0.3f);
  glBindVertexArray(context.textVAO);
  float dimQuad[] = {0.0f,      MAP_HEIGHT, 0.0f, 0.0f, MAP_WIDTH, MAP_HEIGHT, 1.0f, 0.0f,
                     MAP_WIDTH, 0.0f,       1.0f, 1.0f, 0.0f,      0.0f,       0.0f, 1.0f};
  glBindBuffer(GL_ARRAY_BUFFER, context.textVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(dimQuad), dimQuad);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glDisable(GL_BLEND);
  glBindVertexArray(0);

  // ensure no leftover depth test
  glDisable(GL_DEPTH_TEST);

  //
  // 2) Semi‐transparent tint: green overlay if success, red if failure
  //
  glUseProgram(context.textShaderProgram);
  glUniformMatrix4fv(context.textLocProjection, 1, GL_FALSE, textOrtho);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GLint clrLoc = glGetUniformLocation(context.textShaderProgram, "textColor");
  if (state.success) {
    glUniform4f(clrLoc, 0.0f, 0.5f, 0.0f, 0.2f);
  } else {
    glUniform4f(clrLoc, 0.5f, 0.0f, 0.0f, 0.2f);
  }
  glBindVertexArray(context.textVAO);
  float cover[] = {0.0f,      MAP_HEIGHT, 0.0f, 0.0f, MAP_WIDTH, MAP_HEIGHT, 1.0f, 0.0f,
                   MAP_WIDTH, 0.0f,       1.0f, 1.0f, 0.0f,      0.0f,       0.0f, 1.0f};
  glBindBuffer(GL_ARRAY_BUFFER, context.textVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cover), cover);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glDisable(GL_BLEND);
  glBindVertexArray(0);

  //
  // 3) Draw a static Sartre sprite at the bottom‐center (only on success)
  //
  if (state.success) {
    glUseProgram(context.forestShaderProgram);
    glUniformMatrix4fv(context.forestLocProjection, 1, GL_FALSE, ortho);
    glUniform1f(context.forestLocNausea, 0.0f);
    glUniform1f(context.forestLocBliss, 1.0f);
    glUniform1f(context.forestLocTime, 0.0f);
    glBindVertexArray(context.forestVAO);

    float model[16];
    createTranslationMatrix(0.0f, MAP_HEIGHT / 4.0f, 0.0f, model);
    glUniformMatrix4fv(context.forestLocModel, 1, GL_FALSE, model);
    glBindTexture(GL_TEXTURE_2D, textures.forestSartre[0]);
    // standard‐size quad
    float quad[] = {-SARTRE_WIDTH / 2.0f, SARTRE_HEIGHT / 2.0f,  0.0f, 0.0f,
                    SARTRE_WIDTH / 2.0f,  SARTRE_HEIGHT / 2.0f,  1.0f, 0.0f,
                    SARTRE_WIDTH / 2.0f,  -SARTRE_HEIGHT / 2.0f, 1.0f, 1.0f,
                    -SARTRE_WIDTH / 2.0f, -SARTRE_HEIGHT / 2.0f, 0.0f, 1.0f};
    glBindBuffer(GL_ARRAY_BUFFER, context.forestVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
  }

  //
  // 4) Finally render your two result‐texts
  //
  glUseProgram(context.textShaderProgram);
  glUniformMatrix4fv(context.textLocProjection, 1, GL_FALSE, textOrtho);
  SDL_Color white{255, 255, 255, 255};

  // Top summary line with actual page‐count
  std::string summary = "Sartre onnistuu kirjoittamaan " + std::to_string(state.pages_collected) +
                        " sivua ennen kuin inhottavat asiat lopulta saavat hänet kiinni.";
  if (!state.success) {
    renderText(context, context.font, summary, white, context.textShaderProgram, context.textVAO,
               context.textVBO, 200.0f, MAP_HEIGHT - 500.0f, 40);
  } else {
    renderText(context, context.font,
               "Kaikista maailman inhottavista asioista huolimatta kirja tulee valmiiksi. "
               "251-sivuinen Inho julkaistaan vuonna 1938.",
               white, context.textShaderProgram, context.textVAO, context.textVBO, 300.0f,
               MAP_HEIGHT - 500.0f, 40);
  }
}

void results_update(GameStateResults &gameStateResults, Uint32 totalElapsed, float deltaTime,
                    Surfaces &surfaces, InputResult &inputResult) {}

void menu_init(GameStateMenu &gameStateMenu) {}

void menu_draw(RenderContext &context, Textures &textures) {
  // 1. Draw the forest background using the forest shader (no warp, no nausea)
  glUseProgram(context.forestShaderProgram);

  float orthoMatrix[16];
  createOrthographicMatrix(-MAP_WIDTH / 2, MAP_WIDTH / 2, 0.0f, MAP_HEIGHT, -100.0f, 100.0f,
                           orthoMatrix);
  glUniformMatrix4fv(context.forestLocProjection, 1, GL_FALSE, orthoMatrix);

  // Model matrix: identity (background at origin)
  float translationMatrix[16];
  createTranslationMatrix(0.0f, 0.0f, 0.0f, translationMatrix);
  glUniformMatrix4fv(context.forestLocModel, 1, GL_FALSE, translationMatrix);

  // Texture unit and uniforms: no warp, no nausea
  glUniform1i(context.forestLocOurTexture, 0);
  glUniform1f(context.forestLocNausea, 0.0f);
  glUniform1f(context.forestLocBliss, 0.0f);
  glUniform1f(context.forestLocTime, 0.0f);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  glBindVertexArray(context.forestVAO);

  glBindTexture(GL_TEXTURE_2D, textures.forestTausta[0]);
  float backgroundVertices[] = {
      -MAP_WIDTH / 2, MAP_HEIGHT, 0.0f, -1.0f, MAP_WIDTH / 2,  MAP_HEIGHT, 1.0f, -1.0f,
      MAP_WIDTH / 2,  0.0f,       1.0f, 0.0f,  -MAP_WIDTH / 2, 0.0f,       0.0f, 0.0f};
  glBindBuffer(GL_ARRAY_BUFFER, context.forestVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(backgroundVertices), backgroundVertices);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_DEPTH_TEST);

  // 2. Draw a translucent black quad over the whole screen using the text shader
  glUseProgram(context.textShaderProgram);

  float textOrtho[16];
  createOrthographicMatrix(0.0f, MAP_WIDTH, 0.0f, MAP_HEIGHT, -1.0f, 1.0f, textOrtho);
  glUniformMatrix4fv(context.textLocProjection, 1, GL_FALSE, textOrtho);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textures.forestTausta[0]);  // still bound, but not sampled

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Set textColor uniform to black with 70% opacity
  GLint textColorLoc = glGetUniformLocation(context.textShaderProgram, "textColor");
  glUniform4f(textColorLoc, 0.0f, 0.0f, 0.0f, 0.7f);

  float blackQuadVerts[] = {0.0f,      MAP_HEIGHT, 0.0f, 0.0f, MAP_WIDTH, MAP_HEIGHT, 1.0f, 0.0f,
                            MAP_WIDTH, 0.0f,       1.0f, 1.0f, 0.0f,      0.0f,       0.0f, 1.0f};
  glBindVertexArray(context.textVAO);
  glBindBuffer(GL_ARRAY_BUFFER, context.textVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(blackQuadVerts), blackQuadVerts);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  glBindVertexArray(0);
  glDisable(GL_BLEND);

  // 3. Draw the menu text as before
  SDL_Color white = {255, 255, 255, 255};
  const std::string intro =
      "Jean-Paul Sartre istuu metsän keskellä, lehtien kahistessa ympärillään, "
      "ja kirjoittaa kirjaansa, kun äkkiä metsän syvyyksistä alkaa hiipiä "
      "häiritseviä varjoja..";
  renderText(context, context.font, intro, white, context.textShaderProgram, context.textVAO,
             context.textVBO, 300.0f, 1300.0f, 40);

  renderText(context, context.font, "Jatka näpsäyttämällä entteriä", white,
             context.textShaderProgram, context.textVAO, context.textVBO, 600.0f, 500.0f, 0);
}

void menu_update(GameStateMenu &gameStateMenu, Uint32 totalElapsed, float deltaTime,
                 Surfaces &surfaces, InputResult &inputResult) {}

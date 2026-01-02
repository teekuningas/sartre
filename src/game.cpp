#include "game.h"

#include <algorithm>  // for std::min, std::max
#include <cmath>      // for fabs()

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

  // advance beamTime, wrap at 20PI (approx 62.83) to match shader speed 0.1
  gameStateForest.beamTime += deltaTime * 2.0f;
  if (gameStateForest.beamTime >= 62.8318530718f) gameStateForest.beamTime -= 62.8318530718f;

  if (gameStateForest.endingMode) {
    Uint32 now = SDL_GetTicks();

    // Accelerate ending if Enter is pressed
    if (inputResult.confirm) {
      float currentT = (now - gameStateForest.endingStartTime) / 1000.0f;
      float nextTarget = 1000.0f;  // Far future

      if (gameStateForest.endingSuccess) {
        if (currentT < 1.0f)
          nextTarget = 1.0f;  // Skip Wait
        else if (currentT < 16.0f)
          nextTarget = 16.0f;  // Skip Text 1
        else {
          // Exit to menu
          inputResult.transition = true;
          inputResult.transitionTo = MENU;
          return;
        }
      } else {
        if (currentT < 1.0f)
          nextTarget = 1.0f;  // Skip Wait
        else if (currentT < 11.0f)
          nextTarget = 11.0f;  // Skip Text
        else {
          inputResult.transition = true;
          inputResult.transitionTo = MENU;
          return;
        }
      }

      gameStateForest.endingStartTime = now - (Uint32)(nextTarget * 1000.0f);
    }

    float t = (now - gameStateForest.endingStartTime) / 1000.0f;

    if (gameStateForest.endingSuccess) {
      // Text Logic
      if (t < 1.0f) {
        gameStateForest.activeDescription = "";  // 1s Wait
        gameStateForest.activeIsQuote = false;
        gameStateForest.descriptionEndTime = now + 100;
      } else if (t < 16.0f) {
        gameStateForest.activeDescription =
            "Hienoa työtä! Kaikista inhottavista asioista huolimatta Sartre saa kirjansa valmiiksi "
            "ja sinä hetkenä saapuu taivaallinen valoilmiö joka valaisee vieläkin niiden tietä "
            "jotka kirjoittavat kirjaansa metsässä loputtomasti vaeltaen.";
        gameStateForest.activeIsQuote = false;
        gameStateForest.descriptionEndTime = now + 100;
      } else {
        // Wait for exit
        gameStateForest.activeDescription = "";
        gameStateForest.activeIsQuote = false;
        gameStateForest.descriptionEndTime = now + 100;
      }

      // Movement Logic
      if (t < 5.0f) {
        // 0-5s: Move to start
        float alpha = t / 5.0f;
        float targetX = 0.0f;
        float targetY = SARTRE_HEIGHT / 2.0f + EARTH_HEIGHT;
        gameStateForest.sartre.x =
            gameStateForest.endingStartPos.x * (1.0f - alpha) + targetX * alpha;
        gameStateForest.sartre.y =
            gameStateForest.endingStartPos.y * (1.0f - alpha) + targetY * alpha;
        gameStateForest.sartre.vy = 0;
      } else if (t < 10.0f) {
        // 5-10s: Stay at start
        gameStateForest.sartre.x = 0.0f;
        gameStateForest.sartre.y = SARTRE_HEIGHT / 2.0f + EARTH_HEIGHT;
      } else if (t < 15.0f) {
        // 10-15s: Transcend
        gameStateForest.sartre.x = 0.0f;
        float startY = SARTRE_HEIGHT / 2.0f + EARTH_HEIGHT;
        float ascendHeight = 3000.0f;  // Move up significantly
        float alpha = (t - 10.0f) / 5.0f;
        gameStateForest.sartre.y = startY + ascendHeight * alpha;
      } else {
        // >15s: Keep floating up at same speed
        gameStateForest.sartre.x = 0.0f;
        gameStateForest.sartre.y += deltaTime * 600.0f;  // 3000/5 = 600
      }
      gameStateForest.sartre.animIdx = 0;
    } else {
      // Bad Ending Sequence
      if (t < 1.0f) {
        gameStateForest.activeDescription = "";  // 1s Wait
        gameStateForest.activeIsQuote = false;
        gameStateForest.descriptionEndTime = now + 100;
      } else if (t < 11.0f) {
        std::string summary = "Sartre onnistuu kirjoittamaan " +
                              std::to_string(gameStateForest.pages_collected) +
                              " sivua ennen kuin inhottavat asiat lopulta saavat hänet kiinni.";
        gameStateForest.activeDescription = summary;
        gameStateForest.activeIsQuote = false;
        gameStateForest.descriptionEndTime = now + 100;
      } else {
        gameStateForest.activeDescription = "";
        gameStateForest.activeIsQuote = false;
        gameStateForest.descriptionEndTime = now + 100;
      }
      // Freeze animation on bad ending
      gameStateForest.sartre.animIdx = 0;
    }

    // Calculate textAlpha for ending mode
    float alpha = 0.0f;
    if (gameStateForest.endingSuccess) {
      if (t >= 1.0f && t < 16.0f) {
        float fadeIn = (t - 1.0f) / 0.3f;
        float fadeOut = (16.0f - t) / 0.3f;
        alpha = std::min(fadeIn, fadeOut);
      }
    } else {
      if (t >= 1.0f && t < 11.0f) {
        float fadeIn = (t - 1.0f) / 0.3f;
        float fadeOut = (11.0f - t) / 0.3f;
        alpha = std::min(fadeIn, fadeOut);
      }
    }
    gameStateForest.textAlpha = std::max(0.0f, std::min(alpha, 1.0f));

    return;  // Skip normal update
  }

  // Process description queue
  Uint32 currentTime = SDL_GetTicks();

  // Skip current text/wait if Enter is pressed
  if (inputResult.confirm) {
    gameStateForest.descriptionEndTime = 0;
  }

  if (currentTime >= gameStateForest.descriptionEndTime &&
      !gameStateForest.descriptionQueue.empty()) {
    DescriptionEvent event = gameStateForest.descriptionQueue.front();
    gameStateForest.descriptionQueue.pop();

    if (event.type == TEXT) {
      gameStateForest.activeDescription = event.text;
      gameStateForest.activeIsQuote = event.isQuote;
      gameStateForest.descriptionStartTime = currentTime;
      gameStateForest.descriptionEndTime = currentTime + event.duration;
    } else {  // WAIT
      gameStateForest.activeDescription = "";
      gameStateForest.activeIsQuote = false;
      gameStateForest.descriptionEndTime = currentTime + event.duration;
    }
  }

  // Calculate textAlpha for normal gameplay
  if (!gameStateForest.activeDescription.empty()) {
    float alpha = 1.0f;
    float elapsed = (currentTime - gameStateForest.descriptionStartTime) / 1000.0f;
    float remaining = (gameStateForest.descriptionEndTime - currentTime) / 1000.0f;

    if (elapsed < 0.3f)
      alpha = elapsed / 0.3f;
    else if (remaining < 0.3f)
      alpha = remaining / 0.3f;

    gameStateForest.textAlpha = std::max(0.0f, std::min(alpha, 1.0f));
  } else {
    gameStateForest.textAlpha = 0.0f;
  }

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
  if (gameStateForest.nausea_hits >= NUM_NAUSEA_LIMIT) {
    gameStateForest.endingMode = true;
    gameStateForest.endingSuccess = false;
    gameStateForest.endingStartTime = SDL_GetTicks();
    gameStateForest.activeDescription = "";
    while (!gameStateForest.descriptionQueue.empty()) gameStateForest.descriptionQueue.pop();
  } else if (gameStateForest.pages_collected >= gameStateForest.pageGoal) {
    // Start ending sequence
    gameStateForest.endingMode = true;
    gameStateForest.endingSuccess = true;
    gameStateForest.endingStartTime = SDL_GetTicks();
    gameStateForest.endingStartPos = sartre;
    gameStateForest.activeDescription = "";
    while (!gameStateForest.descriptionQueue.empty()) gameStateForest.descriptionQueue.pop();
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

  InputResult inputResult{false, data.gameMode, false};

  handle_events(data, inputResult);

  switch (data.gameMode) {
    case MENU:
      menu_update(data.gameStateMenu, data.totalElapsed, delta, data.imageData.surfaces,
                  inputResult);
      break;
    case FOREST:
      forest_update(data.gameStateForest, data.context, data.totalElapsed, delta, data.imageData,
                    inputResult);
      break;
    default:
      break;
  }

  if (inputResult.transition) {
    if (inputResult.transitionTo == EXIT) {
      data.shouldExit = true;
      return;
    }
    if (inputResult.transitionTo == FOREST) {
      // reset clock so each run starts fresh
      data.totalElapsed = 0;
      data.lastTick = SDL_GetTicks();
      forest_init(data.gameStateForest, data.imageData, data.fastMode);
    }
    if (inputResult.transitionTo == MENU) {
      menu_init(data.gameStateMenu);
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
      menu_draw(data.context, data.imageData.textures, data.gameStateMenu);
      break;

    case FOREST: {
      // letter-box a centered square for the 2D forest
      int side = std::min(winW, winH);
      glViewport((winW - side) / 2, (winH - side) / 2, side, side);
      forest_draw(data.gameStateForest, data.imageData.textures, data.context,
                  data.context.forestShaderProgram, data.context.forestVAO, data.context.forestVBO);
      break;
    }

    default:
      break;
  }

  SDL_GL_SwapWindow(data.context.window);
#ifndef __EMSCRIPTEN__
  SDL_Delay(1);
#endif
}

void handle_events(GameLoopData &data, InputResult &inputResult) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        inputResult.transitionTo = EXIT;
        inputResult.transition = true;
        break;

      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          if (!data.fullscreen) {
            int windowWidth = event.window.data1;
            int windowHeight = event.window.data2;
            int viewportSize = std::min(windowWidth, windowHeight);
            glViewport((windowWidth - viewportSize) / 2, (windowHeight - viewportSize) / 2,
                       viewportSize, viewportSize);
          }
        }
        break;

      case SDL_KEYDOWN:
        if (data.gameMode == MENU) {
          Uint32 now = SDL_GetTicks();
          if (now - data.gameStateMenu.lastCheatTime > 1000) {
            data.gameStateMenu.cheatSequence = "";
          }
          data.gameStateMenu.lastCheatTime = now;

          char key = 0;
          if (event.key.keysym.sym == SDLK_i)
            key = 'i';
          else if (event.key.keysym.sym == SDLK_d)
            key = 'd';
          else if (event.key.keysym.sym == SDLK_k)
            key = 'k';
          else if (event.key.keysym.sym == SDLK_f)
            key = 'f';
          else if (event.key.keysym.sym == SDLK_a)
            key = 'a';

          if (key) {
            data.gameStateMenu.cheatSequence += key;
            if (data.gameStateMenu.cheatSequence.find("idkfa") != std::string::npos) {
              data.gameStateMenu.cheatActive = !data.gameStateMenu.cheatActive;
              data.fastMode = data.gameStateMenu.cheatActive;
              data.gameStateMenu.cheatSequence = "";  // Reset
            }
          } else {
            // Reset on wrong key? Maybe not strictly necessary if we just check substring
            // But "sequence of keys" implies contiguous.
            // let's just keep appending valid chars and clearing on timeout or success.
            // If I press 'x', it resets? User said "gap between presses > 1s".
            // If I press 'i', 'x', 'd'... it breaks the sequence.
            data.gameStateMenu.cheatSequence = "";
          }
        }
        break;

      case SDL_KEYUP:
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          if (data.gameMode == FOREST) {
            inputResult.transitionTo = MENU;
            inputResult.transition = true;
          } else {
            inputResult.transitionTo = EXIT;
            inputResult.transition = true;
          }
        } else if (event.key.keysym.sym == SDLK_RETURN) {
          inputResult.confirm = true;
          if (data.gameMode == MENU) {
            inputResult.transitionTo = FOREST;
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
  gameStateForest.fastMode = fastMode;

  gameStateForest.endingMode = false;
  gameStateForest.endingSuccess = false;
  gameStateForest.endingStartTime = 0;

  gameStateForest.objects.resize(NUM_PAGES + INITIAL_NUM_NAUSEOUS_OBJECTS);
  gameStateForest.pages_collected = 0;
  gameStateForest.nausea_hits = 0;
  gameStateForest.warpTime = 0.0f;  // ← init here
  gameStateForest.beamTime = 0.0f;
  gameStateForest.speedFactor = INITIAL_SPEED_FACTOR;

  gameStateForest.itemSeen.clear();
  gameStateForest.activeDescription = "";
  gameStateForest.descriptionEndTime = 0;
  gameStateForest.textAlpha = 0.0f;
  gameStateForest.lastPageMilestone = -1;
  while (!gameStateForest.descriptionQueue.empty()) {
    gameStateForest.descriptionQueue.pop();
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

  // Bliss only starts after 80%
  float blissLevel = 0.0f;
  const float BLISS_THRESHOLD = 0.5f;
  if (progress > BLISS_THRESHOLD) {
    blissLevel = (progress - BLISS_THRESHOLD) / (1.0f - BLISS_THRESHOLD);
  }
  // Beams start after 50%
  float beamsLevel = 0.0f;
  const float BEAM_THRESHOLD = 0.5f;
  if (progress > BEAM_THRESHOLD) {
    beamsLevel = (progress - BEAM_THRESHOLD) / (1.0f - BEAM_THRESHOLD);
  }

  if (gameStateForest.endingMode) {
    if (gameStateForest.endingSuccess) {
      blissLevel = 1.0f;
      beamsLevel = 1.0f;
      nauseaLevel = 0.0f;
    } else {
      blissLevel = 0.0f;
      beamsLevel = 0.0f;
      nauseaLevel = 1.0f;
    }
  }

  glUniform1f(context.forestLocNausea, nauseaLevel);
  glUniform1f(context.forestLocBliss, blissLevel);
  glUniform1f(context.forestLocBeams, beamsLevel);
  glUniform1f(context.forestLocWarpTime, gameStateForest.warpTime);
  glUniform1f(context.forestLocBeamTime, gameStateForest.beamTime);

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  // Bind the VAO
  glBindVertexArray(context.forestVAO);

  // Draw the Sartre character with brightness only
  Sartre &sartre = gameStateForest.sartre;
  glUniform1f(context.forestLocBliss, blissLevel);
  glUniform1f(context.forestLocBeams, 0.0f);
  GLuint sartreTexture = textures.forestSartre[sartre.animIdx];
  draw_textured_quad(sartre.x, sartre.y, sartre.width, sartre.height, sartreTexture,
                     context.forestLocModel, context.forestVBO);

  // Draw items with no effects
  if (!gameStateForest.endingMode) {
    glUniform1f(context.forestLocBliss, 0.0f);
    glUniform1f(context.forestLocBeams, 0.0f);
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
  }

  // Draw the Background with beams only
  glUniform1f(context.forestLocBliss, 0.0f);
  glUniform1f(context.forestLocBeams, beamsLevel);
  float translationMatrix[16];
  createTranslationMatrix(0.0f, 0.0f, 0.0f, translationMatrix);
  glUniformMatrix4fv(context.forestLocModel, 1, GL_FALSE, translationMatrix);

  glBindTexture(GL_TEXTURE_2D, textures.forestTausta[0]);
  float backgroundVertices[] = {
      -MAP_WIDTH / 2, MAP_HEIGHT, 0.0f, 0.0f, MAP_WIDTH / 2,  MAP_HEIGHT, 1.0f, 0.0f,
      MAP_WIDTH / 2,  0.0f,       1.0f, 1.0f, -MAP_WIDTH / 2, 0.0f,       0.0f, 1.0f};
  glBindBuffer(GL_ARRAY_BUFFER, context.forestVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(backgroundVertices), backgroundVertices);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  // Unbind the VAO and texture
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Disable depth test not to distract others
  glDisable(GL_DEPTH_TEST);

  // If bad ending, draw full-screen dimmer
  if (gameStateForest.endingMode && !gameStateForest.endingSuccess) {
    float textOrtho[16];
    createOrthographicMatrix(0.0f, MAP_WIDTH, 0.0f, MAP_HEIGHT, -1.0f, 1.0f, textOrtho);

    glUseProgram(context.textShaderProgram);
    glUniformMatrix4fv(context.textLocProjection, 1, GL_FALSE, textOrtho);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    GLint textColorLoc = glGetUniformLocation(context.textShaderProgram, "textColor");
    glUniform4f(textColorLoc, 0.0f, 0.0f, 0.0f, 0.3f);  // Black, 30% alpha

    glBindVertexArray(context.textVAO);
    float dimQuad[] = {0.0f,      MAP_HEIGHT, 0.0f, 0.0f, MAP_WIDTH, MAP_HEIGHT, 1.0f, 0.0f,
                       MAP_WIDTH, 0.0f,       1.0f, 1.0f, 0.0f,      0.0f,       0.0f, 1.0f};
    glBindBuffer(GL_ARRAY_BUFFER, context.textVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(dimQuad), dimQuad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisable(GL_BLEND);
  }

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
  float padding = UI_PADDING;

  glUniform4f(textColorLoc, 0.0f, 0.0f, 0.0f, 0.5f);
  float bgPageCount[] = {
      UI_MARGIN - padding,      MAP_HEIGHT - UI_MARGIN + padding,      0.0f, 0.0f,
      UI_MARGIN + tw + padding, MAP_HEIGHT - UI_MARGIN + padding,      1.0f, 0.0f,
      UI_MARGIN + tw + padding, MAP_HEIGHT - UI_MARGIN - th - padding, 1.0f, 1.0f,
      UI_MARGIN - padding,      MAP_HEIGHT - UI_MARGIN - th - padding, 0.0f, 1.0f};
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bgPageCount), bgPageCount);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  // Background for description (if active)
  if (!gameStateForest.activeDescription.empty() &&
      SDL_GetTicks() < gameStateForest.descriptionEndTime) {
    int dw, dh;
    getTextSize(context.font, gameStateForest.activeDescription, 60, dw, dh);

    glUniform4f(textColorLoc, 0.0f, 0.0f, 0.0f, 0.6f * gameStateForest.textAlpha);
    float bgDesc[] = {
        UI_DESC_X_OFFSET - padding,      MAP_HEIGHT - UI_DESC_Y_OFFSET + padding,      0.0f, 0.0f,
        UI_DESC_X_OFFSET + dw + padding, MAP_HEIGHT - UI_DESC_Y_OFFSET + padding,      1.0f, 0.0f,
        UI_DESC_X_OFFSET + dw + padding, MAP_HEIGHT - UI_DESC_Y_OFFSET - dh - padding, 1.0f, 1.0f,
        UI_DESC_X_OFFSET - padding,      MAP_HEIGHT - UI_DESC_Y_OFFSET - dh - padding, 0.0f, 1.0f};
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bgDesc), bgDesc);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  }
  glDisable(GL_BLEND);

  SDL_Color white = {255, 255, 255, 255};
  renderText(context, context.font, pageCountText, white, context.textShaderProgram,
             context.textVAO, context.textVBO, UI_MARGIN, MAP_HEIGHT - UI_MARGIN, 0);

  if (!gameStateForest.activeDescription.empty() &&
      SDL_GetTicks() < gameStateForest.descriptionEndTime) {
    SDL_Color descColor = {255, 255, 255, (Uint8)(255 * gameStateForest.textAlpha)};
    int style = gameStateForest.activeIsQuote ? TTF_STYLE_ITALIC : TTF_STYLE_NORMAL;
    renderText(context, context.font, gameStateForest.activeDescription, descColor,
               context.textShaderProgram, context.textVAO, context.textVBO, UI_DESC_X_OFFSET,
               MAP_HEIGHT - UI_DESC_Y_OFFSET, 60, style);
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

      // Page milestones: 5 pages, then 25%, 50%, 75%
      int milestone = -1;
      if (gameStateForest.pages_collected == 5) {
        milestone = 0;
      } else {
        float progress = float(gameStateForest.pages_collected) / float(gameStateForest.pageGoal);
        if (progress >= 0.25f && gameStateForest.lastPageMilestone < 1)
          milestone = 1;
        else if (progress >= 0.50f && gameStateForest.lastPageMilestone < 2)
          milestone = 2;
        else if (progress >= 0.75f && gameStateForest.lastPageMilestone < 3)
          milestone = 3;
      }

      if (milestone >= 0 && milestone < (int)imageData.pageDescriptions.size()) {
        gameStateForest.descriptionQueue.push(
            {TEXT, imageData.pageDescriptions[milestone], 5000, true});
        gameStateForest.descriptionQueue.push({WAIT, "", 1000, false});
        gameStateForest.lastPageMilestone = milestone;
      }

      float baseProbability = gameStateForest.fastMode ? NAUSEA_SPAWN_PROBABILITY_FAST
                                                       : NAUSEA_SPAWN_PROBABILITY_NORMAL;
      if (((float)rand() / RAND_MAX) < baseProbability) {
        GameObject newNauseousObject;

        const GameObjectType candidates[] = {CHESTNUT, PIPE, BEER, CLOCK};
        const int NUM_CANDIDATES = 4;

        // Count existing instances directly using enum as index
        // Max enum is CLOCK=4, so size 5 is sufficient
        int typeCounts[5] = {0};

        auto countObj = [&](const GameObject &o) {
          if (o.type >= 0 && o.type < 5) {
            typeCounts[o.type]++;
          }
        };

        for (const auto &o : gameStateForest.objects) countObj(o);
        for (const auto &o : newObjects) countObj(o);

        int weights[NUM_CANDIDATES];
        int totalWeight = 0;

        for (int i = 0; i < NUM_CANDIDATES; ++i) {
          GameObjectType t = candidates[i];
          int count = typeCounts[t];

          // Scalable exponential decay: 10000 -> 1000 -> 100 -> 10 -> 1 ...
          // Heavily discourages higher counts recursively (factor of 10)
          int w = 10000;
          for (int k = 0; k < count && w > 1; ++k) {
            w /= 10;
          }
          weights[i] = w;
          totalWeight += weights[i];
        }

        int pick = rand() % totalWeight;
        int current = 0;
        int selectedIndex = 0;

        for (int i = 0; i < NUM_CANDIDATES; ++i) {
          current += weights[i];
          if (pick < current) {
            selectedIndex = i;
            break;
          }
        }

        init_game_object(newNauseousObject, candidates[selectedIndex]);
        spawn_object_avoiding_sartre(newNauseousObject, sartre, newNauseousObject.type,
                                     totalElapsed, gameStateForest.speedFactor);
        newObjects.push_back(newNauseousObject);
      }
    } else {
      gameStateForest.nausea_hits++;

      // First time hit description
      if (!gameStateForest.itemSeen[obj.type]) {
        if (imageData.itemDescriptions.count(obj.type)) {
          gameStateForest.descriptionQueue.push(
              {TEXT, imageData.itemDescriptions[obj.type], 5000, true});
          gameStateForest.descriptionQueue.push({WAIT, "", 1000, false});
        }
        gameStateForest.itemSeen[obj.type] = true;
      }

      return true;  // remove from game
    }
  }
  return false;  // keep in game
}

void menu_init(GameStateMenu &gameStateMenu) {
  gameStateMenu.cheatSequence = "";
  gameStateMenu.lastCheatTime = 0;
}

void menu_draw(RenderContext &context, Textures &textures, GameStateMenu const &state) {
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
  glUniform1f(context.forestLocBeams, 0.0f);
  glUniform1f(context.forestLocWarpTime, 0.0f);
  glUniform1f(context.forestLocBeamTime, 0.0f);

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

  if (state.cheatActive) {
    renderText(context, context.font, "PIIPPU LADATTU", white, context.textShaderProgram,
               context.textVAO, context.textVBO, MAP_WIDTH - 400.0f, MAP_HEIGHT - 25.0f, 0);
  }
}

void menu_update(GameStateMenu &gameStateMenu, Uint32 totalElapsed, float deltaTime,
                 Surfaces &surfaces, InputResult &inputResult) {}

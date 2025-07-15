#include "game.h"

#include "constants.h"
#include "graphics.h"  // now provides format_sdl_surface, create_textures, renderText, etc.

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
      forest_update(data.gameStateForest, data.totalElapsed, delta, data.imageData.surfaces,
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

  // 4) handle transitions
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
      forest_init(data.gameStateForest);
    } else {
      Mix_HaltMusic();
    }
    if (inputResult.transitionTo == MENU) {
      menu_init(data.gameStateMenu);
    }
    if (inputResult.transitionTo == RESULTS) {
      results_init(data.gameStateResults);
    }
    data.gameMode = inputResult.transitionTo;
    return;  // do not draw this frame if you just transitioned
  }

  // 5) draw
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  switch (data.gameMode) {
    case MENU:
      menu_draw(data.context.font, data.context.textShaderProgram, data.context.textVAO,
                data.context.textVBO);
      break;
    case FOREST:
      forest_draw(data.gameStateForest, data.imageData.textures, data.context,
                  data.context.forestShaderProgram, data.context.forestVAO, data.context.forestVBO);
      break;
    case RESULTS:
      results_draw(data.context.font, data.context.textShaderProgram, data.context.textVAO,
                   data.context.textVBO);
      break;
    default:
      break;
  }

  // 6) present
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

void forest_init(GameStateForest &gameStateForest) {
  Sartre &sartre = gameStateForest.sartre;
  sartre.width = SARTRE_WIDTH;
  sartre.height = SARTRE_HEIGHT;
  sartre.x = 0.0;
  sartre.y = sartre.height / 2 + EARTH_HEIGHT;
  sartre.animIdx = 0;
  sartre.animSize = 2;
  sartre.jump = 0;
  sartre.vy = 0.0f;

  gameStateForest.objects.resize(TOTAL_GAME_OBJECTS);
  gameStateForest.pages_collected = 0;
  gameStateForest.nausea_hits = 0;

  for (int i = 0; i < NUM_PAGES; ++i) {
    init_game_object(gameStateForest.objects[i], PAGE);
  }

  for (int i = NUM_PAGES; i < TOTAL_GAME_OBJECTS; ++i) {
    init_game_object(gameStateForest.objects[i], (rand() % 2 == 0) ? CHESTNUT : PIPE);
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
  // Use the shader program
  glUseProgram(shaderProgram);

  // Set up the orthographic projection
  float orthoMatrix[16];
  createOrthographicMatrix(-MAP_WIDTH / 2, MAP_WIDTH / 2, 0.0f, MAP_HEIGHT, -100.0f, 100.0f,
                           orthoMatrix);
  GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
  glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, orthoMatrix);

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  // Bind the VAO
  glBindVertexArray(VAO);

  // Draw the Sartre character
  Sartre &sartre = gameStateForest.sartre;
  GLuint sartreTexture = textures.forestSartre[sartre.animIdx];
  GLint ourTextureLoc = glGetUniformLocation(shaderProgram, "ourTexture");
  glUniform1i(ourTextureLoc, 0);
  GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
  draw_textured_quad(sartre.x, sartre.y, sartre.width, sartre.height, sartreTexture, modelLoc, VBO);

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
    }
    draw_textured_quad(obj.x, obj.y, obj.width, obj.height, objTexture, modelLoc, VBO);
  }

  // Draw the Background
  float translationMatrix[16];
  createTranslationMatrix(0.0f, 0.0f, 0.0f, translationMatrix);
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, translationMatrix);

  glBindTexture(GL_TEXTURE_2D, textures.forestTausta[0]);
  float backgroundVertices[] = {
      -MAP_WIDTH / 2, MAP_HEIGHT, 0.0f, -1.0f, MAP_WIDTH / 2,  MAP_HEIGHT, 1.0f, -1.0f,
      MAP_WIDTH / 2,  0.0f,       1.0f, 0.0f,  -MAP_WIDTH / 2, 0.0f,       0.0f, 0.0f};
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(backgroundVertices), backgroundVertices);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  // Unbind the VAO and texture
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Disable depth test not to distract others
  glDisable(GL_DEPTH_TEST);

  // --- Render Page Count Text ---

  // Set up the orthographic projection for the text rendering (top-left corner)
  float textOrthoMatrix[16];
  // Using screen pixel coordinates for simplicity, assuming KARTTA dimensions match viewport
  // roughly
  createOrthographicMatrix(0.0f, MAP_WIDTH, 0.0f, MAP_HEIGHT, -1.0f, 1.0f, textOrthoMatrix);

  // Use the text shader program
  glUseProgram(context.textShaderProgram);

  // Pass the projection matrix to the text shader
  GLuint textProjectionLoc = glGetUniformLocation(context.textShaderProgram, "projection");
  glUniformMatrix4fv(textProjectionLoc, 1, GL_FALSE, textOrthoMatrix);

  // Prepare text and color
  std::string pageText = "SIVUJA: " + std::to_string(gameStateForest.pages_collected);
  std::string nauseaText = "INHOA: " + std::to_string(gameStateForest.nausea_hits);
  SDL_Color white = {255, 255, 255, 255};
  renderText(context.font, pageText.c_str(), white, context.textShaderProgram, context.textVAO,
             context.textVBO, 50.0f, MAP_HEIGHT - 50.0f);  // Position near top-left
  renderText(context.font, nauseaText.c_str(), white, context.textShaderProgram, context.textVAO,
             context.textVBO, 50.0f, MAP_HEIGHT - 100.0f);  // Position near top-left
}

void update_game_object(GameObject &obj, Sartre &sartre, GameStateForest &gameStateForest,
                        Uint32 totalElapsed) {
  if (!obj.collected) {
    // Change position for sinelike trajectory
    obj.x = obj.x + obj.vx;
    obj.y =
        obj.ymid + obj.amplitude * sin(obj.frequency * ((float)totalElapsed / 1000 + obj.phase));

    // If goes outside the window, come out from the other direction
    if ((obj.x > MAP_WIDTH / 2 + obj.width) && (obj.vx > 0)) {
      obj.x = -(obj.width / 2) - MAP_WIDTH / 2;
    } else if ((obj.x < -MAP_WIDTH / 2 - obj.width) && (obj.vx < 0)) {
      obj.x = MAP_WIDTH / 2 + obj.width / 2;
    }

    // Check for collision with Sartre (AABB collision detection)
    bool collisionX = sartre.x + sartre.width / 2 >= obj.x - obj.width / 2 &&
                      obj.x + obj.width / 2 >= sartre.x - sartre.width / 2;
    bool collisionY = sartre.y + sartre.height / 2 >= obj.y - obj.height / 2 &&
                      obj.y + obj.height / 2 >= sartre.y - sartre.height / 2;

    if (collisionX && collisionY) {
      obj.collected = true;
      if (obj.type == PAGE) {
        gameStateForest.pages_collected++;
      } else {
        gameStateForest.nausea_hits++;
      }

      // Respawn the object at a new random location
      obj.x = (GLfloat)((rand() % (MAP_WIDTH - (int)obj.width)) - (MAP_WIDTH / 2) +
                        (int)(obj.width / 2));
      obj.y = (GLfloat)((rand() % (MAP_HEIGHT - (int)obj.height - (MAP_HEIGHT / 4))) +
                        (MAP_HEIGHT / 8) + (int)(obj.height / 2));
      obj.vx = ((((GLfloat)(rand() % 1000)) / 1000.0f) * 1.5f + 0.5f) * 0.3f *
               (rand() % 2 == 0 ? 1 : -1);
      obj.ymid = obj.y;
      obj.phase = (((GLfloat)(rand() % 1000)) / 1000.0f) * 3.141 * 2;
      if (obj.type != PAGE) {
        obj.type = (rand() % 2 == 0) ? CHESTNUT : PIPE;
      }
      obj.collected = false;
    }
  }
}

void forest_update(GameStateForest &gameStateForest, Uint32 totalElapsed, float deltaTime,
                   Surfaces &surfaces, InputResult &inputResult) {
  Sartre &sartre = gameStateForest.sartre;

  const Uint8 *keystate = SDL_GetKeyboardState(NULL);

  for (auto &obj : gameStateForest.objects) {
    update_game_object(obj, sartre, gameStateForest, totalElapsed);
  }

  // Update sartre animation
  sartre.animIdx = (totalElapsed % 1000) / (1000 / sartre.animSize);

  // Update location and velocity based on
  if (keystate[SDL_SCANCODE_RIGHT]) {
    if (sartre.x < MAP_WIDTH / 2 - sartre.width / 2) {
      sartre.x = sartre.x + deltaTime * SARTRE_VX;
    }
  }

  if (keystate[SDL_SCANCODE_LEFT]) {
    if (sartre.x > -MAP_WIDTH / 2 + sartre.width / 2) {
      sartre.x = sartre.x - deltaTime * SARTRE_VX;
    }
  }

  if (sartre.jump == 0 && keystate[SDL_SCANCODE_UP]) {
    sartre.jump = 1;
    sartre.vy = SARTRE_JUMP_VELOCITY;
  }

  // Handle intricacies related to falling down
  GLfloat predictedY = sartre.y + deltaTime * sartre.vy;
  int sartreXPixels = (int)(sartre.x + MAP_WIDTH / 2);
  int commonExtra = sartre.height / 8;
  int padding = 2;  // if the platform is not exactly exactly straight
  int sartreYPixels = (int)(sartre.y - sartre.height / 2 + commonExtra);
  int predictedYPixels = (int)(predictedY - sartre.height / 2 + commonExtra - padding);

  if (sartre.y >= sartre.height / 2 + EARTH_HEIGHT &&
      predictedY < sartre.height / 2 + EARTH_HEIGHT) {
    // We hit the ground, so set vertical speed to zero.
    sartre.jump = 0;
    sartre.vy = 0;
  } else if (predictedY < sartre.y &&
             !isPixelBlack(surfaces.forestCollisionMap, sartreXPixels,
                           MAP_HEIGHT - sartreYPixels) &&
             isPixelBlack(surfaces.forestCollisionMap, sartreXPixels,
                          MAP_HEIGHT - predictedYPixels)) {
    // We hit a non-ground surface, like a treetop.
    sartre.jump = 0;
    sartre.vy = 0;
  } else {
    // We just fall.
    sartre.y = predictedY;
    sartre.vy = sartre.vy - deltaTime * SARTRE_G;
  }

  // Transition to RESULTS state if all pages have been collected.
  if (gameStateForest.nausea_hits >= 3) {
    inputResult.transition = true;
    inputResult.transitionTo = RESULTS;
  }
}

void results_init(GameStateResults &gameStateResults) {}

void results_draw(TTF_Font *font, GLuint textShaderProgram, GLuint VAO, GLuint VBO) {
  // Set up the orthographic projection for the text rendering
  float orthoMatrix[16];
  createOrthographicMatrix(0.0f, MAP_WIDTH, 0.0f, MAP_HEIGHT, -1.0f, 1.0f, orthoMatrix);

  // Use the text shader program
  glUseProgram(textShaderProgram);

  // Pass the projection matrix to the shader
  GLuint projectionLoc = glGetUniformLocation(textShaderProgram, "projection");
  glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, orthoMatrix);

  SDL_Color white = {255, 255, 255, 255};
  renderText(font, "Ei ole kirjailijan työ aina helppoa!", white, textShaderProgram, VAO, VBO,
             300.0f, 1000.0f);
  renderText(font, "Jatka näpsäyttämällä entteriä", white, textShaderProgram, VAO, VBO, 600.0f,
             500.0f);
}

void results_update(GameStateResults &gameStateResults, Uint32 totalElapsed, float deltaTime,
                    Surfaces &surfaces, InputResult &inputResult) {}

void menu_init(GameStateMenu &gameStateMenu) {}

void menu_draw(TTF_Font *font, GLuint textShaderProgram, GLuint VAO, GLuint VBO) {
  // Set up the orthographic projection for the text rendering
  float orthoMatrix[16];
  createOrthographicMatrix(0.0f, MAP_WIDTH, 0.0f, MAP_HEIGHT, -1.0f, 1.0f, orthoMatrix);

  // Use the text shader program
  glUseProgram(textShaderProgram);

  // Pass the projection matrix to the shader
  GLuint projectionLoc = glGetUniformLocation(textShaderProgram, "projection");
  glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, orthoMatrix);

  SDL_Color white = {255, 255, 255, 255};

  renderText(font, "Jean-Paul Sartre istui metsän keskellä, ", white, textShaderProgram, VAO, VBO,
             300.0f, 1300.0f);
  renderText(font, "lehtien kahistessa ympärillään, ja kirjoitti uutta kirjaansa,", white,
             textShaderProgram, VAO, VBO, 300.0f, 1200.0f);
  renderText(font, "kun äkkiä metsän syvyyksistä alkoi hiipiä häiritseviä varjoja, ", white,
             textShaderProgram, VAO, VBO, 300.0f, 1100.0f);
  renderText(font, "jotka uhkasivat keskeyttää hänen luomisprosessinsa.", white, textShaderProgram,
             VAO, VBO, 300.0f, 1000.0f);
  renderText(font, "Jatka näpsäyttämällä entteriä", white, textShaderProgram, VAO, VBO, 600.0f,
             500.0f);
}

void menu_update(GameStateMenu &gameStateMenu, Uint32 totalElapsed, float deltaTime,
                 Surfaces &surfaces, InputResult &inputResult) {}

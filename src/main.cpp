#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <cstring>
#include <string>  // Required for std::to_string

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include <GL/glew.h>

#include "constants.h"
#include "matrix.h"
#include "render_context.h"
#include "resources.h"
#include "shader_utils.h"
#include "types.h"
#include "utils.h"

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

void forest_init(GameStateForest &gameStateForest) {
  Sartre &sartre = gameStateForest.sartre;
  sartre.width = 256;
  sartre.height = 256;
  sartre.x = 0.0;
  sartre.y = sartre.height / 2 + MAA_KORKEUS;
  sartre.animIdx = 0;
  sartre.animSize = 2;
  sartre.jump = 0;

  size_t numObjects = 5;
  gameStateForest.pages.resize(numObjects);
  gameStateForest.collectedPages = 0;       // Initialize collected pages
  gameStateForest.totalPages = numObjects;  // Initialize total pages

  for (auto &obj : gameStateForest.pages) {
    obj.width = 128;
    obj.height = 128;

    obj.x = (GLfloat)((rand() % (KARTTA_LEVEYS - (int)obj.width)) - (KARTTA_LEVEYS / 2) +
                      (int)(obj.width / 2));
    obj.y = (GLfloat)((rand() % (KARTTA_KORKEUS - (int)obj.height - (KARTTA_KORKEUS / 4))) +
                      (KARTTA_KORKEUS / 8) + (int)(obj.height / 2));

    obj.vx = 0.3;

    obj.ymid = obj.y;
    obj.amplitude = 200;
    obj.frequency = 1.5;
    obj.phase = (((GLfloat)(rand() % 1000)) / 1000.0f) * 3.141 * 2;

    obj.collected = false;  // Initialize as not collected
    obj.animIdx = 0;
    obj.animSize = 1;
  }
}

void forest_draw(GameStateForest &gameStateForest, Textures &textures, RenderContext &context,
                 GLuint shaderProgram, GLuint VAO, GLuint VBO) {
  // Use the shader program
  glUseProgram(shaderProgram);

  // Set up the orthographic projection
  float orthoMatrix[16];
  createOrthographicMatrix(-KARTTA_LEVEYS / 2, KARTTA_LEVEYS / 2, 0.0f, KARTTA_KORKEUS, -100.0f,
                           100.0f, orthoMatrix);
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
  glBindTexture(GL_TEXTURE_2D, sartreTexture);

  // Specify the texture uniform
  GLint ourTextureLoc = glGetUniformLocation(shaderProgram, "ourTexture");
  glUniform1i(ourTextureLoc, 0);

  // Set model matrix for Sartre
  float translationMatrix[16];
  createTranslationMatrix(sartre.x, sartre.y, 0.1f, translationMatrix);
  GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, translationMatrix);

  // Define and draw the quad vertices for Sartre
  float sartreVertices[] = {-sartre.width / 2, sartre.height / 2,  0.01f, -0.99f,
                            sartre.width / 2,  sartre.height / 2,  0.99f, -0.99f,
                            sartre.width / 2,  -sartre.height / 2, 0.99f, 0.01f,
                            -sartre.width / 2, -sartre.height / 2, 0.01f, 0.01f};
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sartreVertices), sartreVertices);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  for (auto &obj : gameStateForest.pages) {
    // Skip drawing if collected
    if (obj.collected) {
      continue;
    }

    GLuint objTexture = textures.forestPage[obj.animIdx];

    // Bind object texture
    glBindTexture(GL_TEXTURE_2D, objTexture);

    // Set model matrix for the GameObject
    createTranslationMatrix(obj.x, obj.y, 0.1f, translationMatrix);  // Adjust depth if needed
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, translationMatrix);

    // Define and draw the quad vertices for the Page
    float objectVertices[] = {-obj.width / 2, obj.height / 2,  0.01f, -0.99f,
                              obj.width / 2,  obj.height / 2,  0.99f, -0.99f,
                              obj.width / 2,  -obj.height / 2, 0.99f, 0.01f,
                              -obj.width / 2, -obj.height / 2, 0.01f, 0.01f};
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(objectVertices), objectVertices);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  }

  // Draw the Background
  createTranslationMatrix(0.0f, 0.0f, 0.0f, translationMatrix);
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, translationMatrix);

  glBindTexture(GL_TEXTURE_2D, textures.forestTausta[0]);
  float backgroundVertices[] = {-KARTTA_LEVEYS / 2, KARTTA_KORKEUS, 0.0f, -1.0f,
                                KARTTA_LEVEYS / 2,  KARTTA_KORKEUS, 1.0f, -1.0f,
                                KARTTA_LEVEYS / 2,  0.0f,           1.0f, 0.0f,
                                -KARTTA_LEVEYS / 2, 0.0f,           0.0f, 0.0f};
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
  createOrthographicMatrix(0.0f, KARTTA_LEVEYS, 0.0f, KARTTA_KORKEUS, -1.0f, 1.0f, textOrthoMatrix);

  // Use the text shader program
  glUseProgram(context.textShaderProgram);

  // Pass the projection matrix to the text shader
  GLuint textProjectionLoc = glGetUniformLocation(context.textShaderProgram, "projection");
  glUniformMatrix4fv(textProjectionLoc, 1, GL_FALSE, textOrthoMatrix);

  // Prepare text and color
  std::string pageText = "Pages: " + std::to_string(gameStateForest.collectedPages) + " / " +
                         std::to_string(gameStateForest.totalPages);
  SDL_Color white = {255, 255, 255, 255};
  renderText(context.font, pageText.c_str(), white, context.textShaderProgram, context.textVAO,
             context.textVBO, 50.0f, KARTTA_KORKEUS - 50.0f);  // Position near top-left
}

void forest_update(GameStateForest &gameStateForest, Uint32 totalElapsed, float deltaTime,
                   Surfaces &surfaces, InputResult &inputResult) {
  Sartre &sartre = gameStateForest.sartre;

  const Uint8 *keystate = SDL_GetKeyboardState(NULL);

  // Update page objects
  for (auto &obj : gameStateForest.pages) {
    // Only update and check collision for visible pages
    if (!obj.collected) {
      // Update animation
      obj.animIdx = (totalElapsed % 1000) / (1000 / obj.animSize);

      // Change position for sinelike trajectory
      obj.x = obj.x + obj.vx;
      obj.y =
          obj.ymid + obj.amplitude * sin(obj.frequency * ((float)totalElapsed / 1000 + obj.phase));

      // If goes outside the window, come out from the other direction
      if ((obj.x > KARTTA_LEVEYS / 2 + obj.width) && (obj.vx > 0)) {
        obj.x = -(obj.width / 2) - KARTTA_LEVEYS / 2;
      } else if ((obj.x < -KARTTA_LEVEYS / 2 - obj.width) && (obj.vx < 0)) {
        obj.x = KARTTA_LEVEYS / 2 + obj.width / 2;
      }

      // Check for collision with Sartre (AABB collision detection)
      bool collisionX = sartre.x + sartre.width / 2 >= obj.x - obj.width / 2 &&
                        obj.x + obj.width / 2 >= sartre.x - sartre.width / 2;
      bool collisionY = sartre.y + sartre.height / 2 >= obj.y - obj.height / 2 &&
                        obj.y + obj.height / 2 >= sartre.y - sartre.height / 2;

      if (collisionX && collisionY) {
        obj.collected = true;
        gameStateForest.collectedPages++;
        // Optional: Add sound effect or visual feedback here
      }
    }
  }

  // Update sartre animation
  sartre.animIdx = (totalElapsed % 1000) / (1000 / sartre.animSize);

  // Update location and velocity based on
  if (keystate[SDL_SCANCODE_RIGHT]) {
    if (sartre.x < KARTTA_LEVEYS / 2 - sartre.width / 2) {
      sartre.x = sartre.x + deltaTime * HAHMO_VX;
    }
  }

  if (keystate[SDL_SCANCODE_LEFT]) {
    if (sartre.x > -KARTTA_LEVEYS / 2 + sartre.width / 2) {
      sartre.x = sartre.x - deltaTime * HAHMO_VX;
    }
  }

  if (sartre.jump == 0 && keystate[SDL_SCANCODE_UP]) {
    sartre.jump = 1;
    sartre.vy = HAHMO_HYPPYNOPEUS;
  }

  // Handle intricacies related to falling down
  GLfloat predictedY = sartre.y + deltaTime * sartre.vy;
  int sartreXPixels = (int)(sartre.x + KARTTA_LEVEYS / 2);
  int commonExtra = sartre.height / 8;
  int padding = 2;  // if the platform is not exactly exactly straight
  int sartreYPixels = (int)(sartre.y - sartre.height / 2 + commonExtra);
  int predictedYPixels = (int)(predictedY - sartre.height / 2 + commonExtra - padding);

  if (sartre.y >= sartre.height / 2 + MAA_KORKEUS && predictedY < sartre.height / 2 + MAA_KORKEUS) {
    // We hit the ground, so set vertical speed to zero.
    sartre.jump = 0;
    sartre.vy = 0;
  } else if (predictedY < sartre.y &&
             !isPixelBlack(surfaces.forestCollisionMap, sartreXPixels,
                           KARTTA_KORKEUS - sartreYPixels) &&
             isPixelBlack(surfaces.forestCollisionMap, sartreXPixels,
                          KARTTA_KORKEUS - predictedYPixels)) {
    // We hit a non-ground surface, like a treetop.
    sartre.jump = 0;
    sartre.vy = 0;
  } else {
    // We just fall.
    sartre.y = predictedY;
    sartre.vy = sartre.vy - deltaTime * HAHMO_G;
  }

  // Transition to RESULTS state if all pages have been collected.
  if (gameStateForest.collectedPages == gameStateForest.totalPages) {
    inputResult.transition = true;
    inputResult.transitionTo = RESULTS;
  }
}

void results_init(GameStateResults &gameStateResults) {}

void results_draw(TTF_Font *font, GLuint textShaderProgram, GLuint VAO, GLuint VBO) {
  // Set up the orthographic projection for the text rendering
  float orthoMatrix[16];
  createOrthographicMatrix(0.0f, KARTTA_LEVEYS, 0.0f, KARTTA_KORKEUS, -1.0f, 1.0f, orthoMatrix);

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
  createOrthographicMatrix(0.0f, KARTTA_LEVEYS, 0.0f, KARTTA_KORKEUS, -1.0f, 1.0f, orthoMatrix);

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

GameLoopData gameLoopData;

void main_loop_iteration() {
  /* Initialization and cleanup are kept within the loop function
   * for the sake of webgl context which would not be
   * automatically active here if initialized outside.
   */

  if (gameLoopData.shouldExit) {
    free_textures(gameLoopData.imageData.textures);
    free_surfaces(gameLoopData.imageData.surfaces);
    cleanup_render_context(gameLoopData.context);
    exit(0);
  }

  if (!gameLoopData.initialized) {
    std::string dataPath = getResourcePath();

    if (!initialize_render_context(gameLoopData.context, dataPath, gameLoopData.fullscreen)) {
      gameLoopData.shouldExit = true;
      return;
    }

    // Compile shader program and create VAO and VBO for text rendering
    createProgram(textVertexShaderSource, textFragmentShaderSource,
                  gameLoopData.context.textShaderProgram);
    if (!gameLoopData.context.textShaderProgram) {
      gameLoopData.shouldExit = true;
      return;
    }
    createShaderBuffers(gameLoopData.context.textVAO, gameLoopData.context.textVBO);

    // Compile shader program and create VAO and VBO for forest rendering
    createProgram(forestVertexShaderSource, forestFragmentShaderSource,
                  gameLoopData.context.forestShaderProgram);
    if (!gameLoopData.context.forestShaderProgram) {
      gameLoopData.shouldExit = true;
      return;
    }
    createShaderBuffers(gameLoopData.context.forestVAO, gameLoopData.context.forestVBO);

    WindowParams windowParams = compute_window_params(gameLoopData.fullscreen);
    glViewport((windowParams.windowWidth - windowParams.viewportSize) / 2,
               (windowParams.windowHeight - windowParams.viewportSize) / 2,
               windowParams.viewportSize, windowParams.viewportSize);

    create_textures(gameLoopData.imageData.textures, dataPath);
    create_surfaces(gameLoopData.imageData.surfaces, dataPath);

    gameLoopData.gameMode = MENU;
    gameLoopData.lastTick = SDL_GetTicks();
    gameLoopData.totalElapsed = 0;

    // Do not come here anymore
    gameLoopData.initialized = true;
  }

  // --- Main Loop Logic ---

  InputResult inputResult;
  inputResult.transition = false;  // Initialize for this frame

  // Calculate delta time
  gameLoopData.currentTick = SDL_GetTicks();
  float deltaTime = (gameLoopData.currentTick - gameLoopData.lastTick) / 1000.0f;
  gameLoopData.totalElapsed += gameLoopData.currentTick - gameLoopData.lastTick;
  gameLoopData.lastTick = gameLoopData.currentTick;

  // 1. Update current game state (can potentially set inputResult.transition)
  switch (gameLoopData.gameMode) {
    case MENU:
      menu_update(gameLoopData.gameStateMenu, gameLoopData.totalElapsed, deltaTime,
                  gameLoopData.imageData.surfaces, inputResult);
      break;
    case FOREST:
      forest_update(gameLoopData.gameStateForest, gameLoopData.totalElapsed, deltaTime,
                    gameLoopData.imageData.surfaces, inputResult);
      break;
    case RESULTS:
      results_update(gameLoopData.gameStateResults, gameLoopData.totalElapsed, deltaTime,
                     gameLoopData.imageData.surfaces, inputResult);
      break;
    default:
      break;
  }

  // 2. Handle user events (can also set inputResult.transition)
  handle_events(gameLoopData.gameMode, gameLoopData.fullscreen, inputResult);

  // 3. Check if a transition is requested (either by update or events)
  if (inputResult.transition) {
    if (inputResult.transitionTo == EXIT) {
      gameLoopData.shouldExit = true;
      return;
    }
    if (inputResult.transitionTo == FOREST) {
      if (Mix_PlayMusic(gameLoopData.context.backgroundMusic, -1) == -1) {
        printf("Failed to play background music! SDL_mixer Error: %s\n", Mix_GetError());
        gameLoopData.shouldExit = true;
      }
      forest_init(gameLoopData.gameStateForest);
    } else {
      Mix_HaltMusic();
    }
    if (inputResult.transitionTo == MENU) {
      menu_init(gameLoopData.gameStateMenu);
    }
    if (inputResult.transitionTo == RESULTS) {
      results_init(gameLoopData.gameStateResults);
    }
    gameLoopData.gameMode = inputResult.transitionTo;
    return;
  }

  // 4. Draw the current state
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  switch (gameLoopData.gameMode) {
    case MENU:
      menu_draw(gameLoopData.context.font, gameLoopData.context.textShaderProgram,
                gameLoopData.context.textVAO, gameLoopData.context.textVBO);
      break;
    case FOREST:
      forest_draw(gameLoopData.gameStateForest, gameLoopData.imageData.textures,
                  gameLoopData.context, gameLoopData.context.forestShaderProgram,
                  gameLoopData.context.forestVAO, gameLoopData.context.forestVBO);
      break;
    case RESULTS:
      results_draw(gameLoopData.context.font, gameLoopData.context.textShaderProgram,
                   gameLoopData.context.textVAO, gameLoopData.context.textVBO);
      break;
    default:
      break;
  }

  SDL_GL_SwapWindow(gameLoopData.context.window);
#ifndef __EMSCRIPTEN__
  SDL_Delay(1);
#endif
}

int main(int argc, char **argv) {
  srand(time(NULL));

  if (argc > 1 && std::strcmp(argv[1], "--smoke") == 0) {
    std::cout << "Smoketest ran fine!" << std::endl;
    return 0;
  }

  gameLoopData.fullscreen = (argc > 1 && std::strcmp(argv[1], "--fullscreen") == 0);

  gameLoopData.initialized = false;
  gameLoopData.shouldExit = false;
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(main_loop_iteration, 0, 0);
#else
  while (true) {
    main_loop_iteration();
  }
#endif

  return 0;
}

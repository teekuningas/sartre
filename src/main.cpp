#define SDL_MAIN_HANDLED

#include <cstring>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include <GL/glew.h>

#include "matrix.h"
#include "resources.h"
#include "shader_utils.h"
#include "types.h"
#include "constants.h"
#include "render_context.h"
#include "utils.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

InputResult handle_events(GameMode &gameMode, bool fullscreen)
{
	SDL_Event event;

	InputResult inputResult;
	inputResult.transition = false;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			inputResult.transitionTo = EXIT;
			inputResult.transition = true;
			break;

		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
				if(!fullscreen) {
					int windowWidth = event.window.data1;
					int windowHeight = event.window.data2;
					int viewportSize = std::min(windowWidth, windowHeight);
					glViewport((windowWidth - viewportSize) / 2, (windowHeight - viewportSize) / 2, viewportSize, viewportSize);
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
	return inputResult;
}

void forest_init(GameStateForest &gameStateForest)
{
	Sartre &sartre = gameStateForest.sartre;
	sartre.x = 0.0;
	sartre.y = HAHMO_KORKEUS / 2 + MAA_KORKEUS;
	sartre.hahmo = 0;
	sartre.hyppy = 0;
}

void forest_draw(GameStateForest &gameStateForest, Textures &textures, GLuint shaderProgram, GLuint VAO, GLuint VBO)
{
	Sartre &sartre = gameStateForest.sartre;

	// Use the shader program
	glUseProgram(shaderProgram);

	// Set up the orthographic projection
	float orthoMatrix[16];
	createOrthographicMatrix(-KARTTA_LEVEYS / 2, KARTTA_LEVEYS / 2, 0.0f, KARTTA_KORKEUS, -100.0f, 100.0f, orthoMatrix);
	GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, orthoMatrix);

	float translationMatrix[16];
	createTranslationMatrix(sartre.x, sartre.y, 0.1f, translationMatrix);
	GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, translationMatrix);

	// Enable depth test to get sartre visible
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Bind the VAO
	glBindVertexArray(VAO);

	// Sartre Character
	GLuint sartreTexture = (sartre.hahmo == 0) ? textures.forestSartre[0] : textures.forestSartre[1];
	glBindTexture(GL_TEXTURE_2D, sartreTexture);

	// Specify the texture uniform
	GLint ourTextureLoc = glGetUniformLocation(shaderProgram, "ourTexture");
	glUniform1i(ourTextureLoc, 0);

	// Define the quad vertices and texture coordinates for Sartre
	float sartreVertices[] = {
		-HAHMO_LEVEYS/2, HAHMO_KORKEUS/2, 0.01f, -0.99f,
		    HAHMO_LEVEYS/2, HAHMO_KORKEUS/2, 0.99f, -0.99f,
		    HAHMO_LEVEYS/2, -HAHMO_KORKEUS/2, 0.99f, 0.01f,
		    -HAHMO_LEVEYS/2, -HAHMO_KORKEUS/2, 0.01f, 0.01f
	    };
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sartreVertices), sartreVertices);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	// Forest Background
	createTranslationMatrix(0.0f, 0.0f, 0.0f, translationMatrix);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, translationMatrix);

	glBindTexture(GL_TEXTURE_2D, textures.forestTausta[0]);
	float backgroundVertices[] = {
		-KARTTA_LEVEYS / 2, KARTTA_KORKEUS, 0.0f, -1.0f,
		    KARTTA_LEVEYS / 2, KARTTA_KORKEUS, 1.0f, -1.0f,
		    KARTTA_LEVEYS / 2, 0.0f, 1.0f, 0.0f,
		    -KARTTA_LEVEYS / 2, 0.0f, 0.0f, 0.0f
	    };
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(backgroundVertices), backgroundVertices);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	// Unbind the VAO and texture
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Disable depth test not distract others
	glDisable(GL_DEPTH_TEST);
}

InputResult forest_update(GameStateForest &gameStateForest, Uint32 totalElapsed, float deltaTime, Surfaces &surfaces)
{
	InputResult inputResult;
	inputResult.transition = false;

	Sartre &sartre = gameStateForest.sartre;

	const Uint8 *keystate = SDL_GetKeyboardState(NULL);

	// Vaihda hahmoa
	if (totalElapsed % 1000 <= 500) {
		sartre.hahmo = 0;
	} else {
		sartre.hahmo = 1;
	}

	if (keystate[SDL_SCANCODE_RIGHT]) {
		if (sartre.x < KARTTA_LEVEYS / 2 - HAHMO_LEVEYS / 2) {
			sartre.x = sartre.x + deltaTime*HAHMO_VX;
		}
	}

	if (keystate[SDL_SCANCODE_LEFT]) {
		if (sartre.x > -KARTTA_LEVEYS / 2 + HAHMO_LEVEYS / 2) {
			sartre.x = sartre.x - deltaTime*HAHMO_VX;
		}
	}

	if (sartre.hyppy == 0 && keystate[SDL_SCANCODE_UP]) {
		sartre.hyppy = 1;
		sartre.vy = HAHMO_HYPPYNOPEUS;
	}

	GLfloat predictedY = sartre.y + deltaTime*sartre.vy;

	int sartreXPixels = (int)(sartre.x + KARTTA_LEVEYS / 2);
	int commonExtra = HAHMO_KORKEUS / 8;
	int padding = 2; // if the platform is not exactly exactly straight
	int sartreYPixels = (int)(sartre.y - HAHMO_KORKEUS / 2 + commonExtra);
	int predictedYPixels = (int)(predictedY - HAHMO_KORKEUS / 2 + commonExtra - padding);

	if (sartre.y >= HAHMO_KORKEUS / 2 + MAA_KORKEUS && predictedY < HAHMO_KORKEUS / 2 + MAA_KORKEUS) {
		sartre.hyppy = 0;
		sartre.vy = 0;
	} else if (
	    predictedY < sartre.y &&
	    !isPixelBlack(surfaces.forestCollisionMap, sartreXPixels, KARTTA_KORKEUS - sartreYPixels) &&
	    isPixelBlack(surfaces.forestCollisionMap, sartreXPixels, KARTTA_KORKEUS - predictedYPixels)
	) {
		sartre.hyppy = 0;
		sartre.vy = 0;
	} else {
		sartre.y = sartre.y + deltaTime*sartre.vy;
		sartre.vy = sartre.vy - deltaTime*HAHMO_G;
	}

	return inputResult;
}

void results_init(GameStateResults &gameStateResults)
{

}

void results_draw(TTF_Font* font, GLuint textShaderProgram, GLuint VAO, GLuint VBO)
{
	// Set up the orthographic projection for the text rendering
	float orthoMatrix[16];
	createOrthographicMatrix(0.0f, KARTTA_LEVEYS, 0.0f, KARTTA_KORKEUS, -1.0f, 1.0f, orthoMatrix);

	// Use the text shader program
	glUseProgram(textShaderProgram);

	// Pass the projection matrix to the shader
	GLuint projectionLoc = glGetUniformLocation(textShaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, orthoMatrix);


	SDL_Color white = {255, 255, 255, 255};
	renderText(font, "Ei ole kirjailijan työ aina helppoa!", white, textShaderProgram, VAO, VBO, 300.0f, 1000.0f);
	renderText(font, "Jatka näpsäyttämällä entteriä", white, textShaderProgram, VAO, VBO, 600.0f, 500.0f);
}

InputResult results_update(GameStateResults &gameStateResults, Uint32 totalElapsed, float deltaTime, Surfaces &surfaces)
{
	InputResult inputResult;
	return inputResult;
}

void menu_init(GameStateMenu &gameStateMenu)
{

}

void menu_draw(TTF_Font* font, GLuint textShaderProgram, GLuint VAO, GLuint VBO)
{
	// Set up the orthographic projection for the text rendering
	float orthoMatrix[16];
	createOrthographicMatrix(0.0f, KARTTA_LEVEYS, 0.0f, KARTTA_KORKEUS, -1.0f, 1.0f, orthoMatrix);

	// Use the text shader program
	glUseProgram(textShaderProgram);

	// Pass the projection matrix to the shader
	GLuint projectionLoc = glGetUniformLocation(textShaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, orthoMatrix);

	SDL_Color white = {255, 255, 255, 255};

	renderText(font, "Jean-Paul Sartre istui metsän keskellä, ", white, textShaderProgram, VAO, VBO, 300.0f, 1300.0f);
	renderText(font, "lehtien kahistessa ympärillään, ja kirjoitti uutta kirjaansa,", white, textShaderProgram, VAO, VBO, 300.0f, 1200.0f);
	renderText(font, "kun äkkiä metsän syvyyksistä alkoi hiipiä häiritseviä varjoja, ", white, textShaderProgram, VAO, VBO, 300.0f, 1100.0f);
	renderText(font, "jotka uhkasivat keskeyttää hänen luomisprosessinsa.", white, textShaderProgram, VAO, VBO, 300.0f, 1000.0f);
	renderText(font, "Jatka näpsäyttämällä entteriä", white, textShaderProgram, VAO, VBO, 600.0f, 500.0f);
}

InputResult menu_update(GameStateMenu &gameStateMenu, Uint32 totalElapsed, float deltaTime, Surfaces &surfaces)
{
	InputResult inputResult;
	return inputResult;
}

GameLoopData gameLoopData;

void main_loop_iteration()
{
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
		createProgram(textVertexShaderSource, textFragmentShaderSource, gameLoopData.context.textShaderProgram);
		if (!gameLoopData.context.textShaderProgram) {
			gameLoopData.shouldExit = true;
			return;
		}
		createShaderBuffers(gameLoopData.context.textVAO, gameLoopData.context.textVBO);

		// Compile shader program and create VAO and VBO for forest rendering
		createProgram(forestVertexShaderSource, forestFragmentShaderSource, gameLoopData.context.forestShaderProgram);
		if (!gameLoopData.context.forestShaderProgram) {
			gameLoopData.shouldExit = true;
			return;
		}
		createShaderBuffers(gameLoopData.context.forestVAO, gameLoopData.context.forestVBO);

		WindowParams windowParams = compute_window_params(gameLoopData.fullscreen);
		glViewport((windowParams.windowWidth - windowParams.viewportSize) / 2,
			   (windowParams.windowHeight - windowParams.viewportSize) / 2,
			   windowParams.viewportSize,
			   windowParams.viewportSize);

		create_textures(gameLoopData.imageData.textures, dataPath);
		create_surfaces(gameLoopData.imageData.surfaces, dataPath);

		gameLoopData.gameMode = MENU;
		gameLoopData.lastTick = SDL_GetTicks();
		gameLoopData.totalElapsed = 0;

		// Do not come here anymore
		gameLoopData.initialized = true;
	}

	InputResult inputResult;
	inputResult = handle_events(gameLoopData.gameMode, gameLoopData.fullscreen);

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

	gameLoopData.currentTick = SDL_GetTicks();
	float deltaTime = (gameLoopData.currentTick - gameLoopData.lastTick) / 1000.0f;
	gameLoopData.totalElapsed += gameLoopData.currentTick - gameLoopData.lastTick;
	gameLoopData.lastTick = gameLoopData.currentTick;

	switch (gameLoopData.gameMode) {
	case MENU:
		inputResult = menu_update(gameLoopData.gameStateMenu, gameLoopData.totalElapsed, deltaTime, gameLoopData.imageData.surfaces);
		break;
	case FOREST:
		inputResult = forest_update(gameLoopData.gameStateForest, gameLoopData.totalElapsed, deltaTime, gameLoopData.imageData.surfaces);
		break;
	case RESULTS:
		inputResult = results_update(gameLoopData.gameStateResults, gameLoopData.totalElapsed, deltaTime, gameLoopData.imageData.surfaces);
		break;
	default:
		break;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	switch (gameLoopData.gameMode) {
	case MENU:
		menu_draw(gameLoopData.context.font, gameLoopData.context.textShaderProgram, gameLoopData.context.textVAO, gameLoopData.context.textVBO);
		break;
	case FOREST:
		forest_draw(gameLoopData.gameStateForest, gameLoopData.imageData.textures, gameLoopData.context.forestShaderProgram, gameLoopData.context.forestVAO, gameLoopData.context.forestVBO);
		break;
	case RESULTS:
		results_draw(gameLoopData.context.font, gameLoopData.context.textShaderProgram, gameLoopData.context.textVAO, gameLoopData.context.textVBO);
		break;
	default:
		break;
	}

	SDL_GL_SwapWindow(gameLoopData.context.window);
#ifndef __EMSCRIPTEN__
	SDL_Delay(1);
#endif

}

int main(int argc, char **argv)
{
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
	while(true) {
		main_loop_iteration();
	}
#endif

	return 0;
}

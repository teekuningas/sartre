#define SDL_MAIN_HANDLED

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <vector>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#else
#include <GL/glew.h>
#endif

#include "shader_utils.h"
#include "matrix.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

// Vertex Shader Source for Text Rendering
const char* textVertexShaderSource =
    "#version 100\n"
    "attribute vec2 position;\n"
    "attribute vec2 texCoord;\n"
    "varying vec2 fragTexCoord;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(position, 0.0, 1.0);\n"
    "    fragTexCoord = texCoord;\n"
    "}\n";

// Fragment Shader Source for Text Rendering
const char* textFragmentShaderSource =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 fragTexCoord;\n"
    "uniform sampler2D textTexture;\n"
    "uniform vec4 textColor;\n"
    "void main() {\n"
    "    vec4 sampled = texture2D(textTexture, fragTexCoord);\n"
    "    gl_FragColor = textColor * sampled;\n"
    "}\n";

// Vertex Shader Source for Forest Rendering
const char* forestVertexShaderSource =
    "#version 100\n"
    "attribute vec2 position;\n"
    "attribute vec2 texCoord;\n"
    "varying vec2 fragTexCoord;\n"
    "uniform mat4 projection;\n"
    "uniform mat4 model;\n"
    "void main() {\n"
    "    gl_Position = projection * model * vec4(position, 0.0, 1.0);\n"
    "    fragTexCoord = texCoord;\n"
    "}\n";

// Fragment Shader Source for Forest Rendering
const char* forestFragmentShaderSource =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 fragTexCoord;\n"
    "uniform sampler2D ourTexture;\n"
    "void main() {\n"
    "    vec4 texColor = texture2D(ourTexture, fragTexCoord);\n"
    "    if (texColor.a <= 0.1) discard;\n"
    "    gl_FragColor = texColor;\n"
    "}\n";

std::string getResourcePath()
{
	const char* dataPath = getenv("SARTRE_DATA_PATH");
	if (dataPath != nullptr) {
		printf("Reading data from path: %s\n", dataPath);
		return std::string(dataPath) + "/";
	}
#ifdef __APPLE__
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	if (mainBundle) {
		CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
		char path[PATH_MAX];
		if (CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX)) {
			CFRelease(resourcesURL);
			return std::string(path) + "/data/";
		}
		CFRelease(resourcesURL);
	}
	return "./data/";
#else
	return "./data/";
#endif
}

struct Sartre {
	GLfloat x;
	GLfloat y;
	GLfloat vy;
	int hahmo;
	bool hyppy;
};

enum GameMode { MENU, FOREST, RESULTS, EXIT };

struct GameStateForest {
	Sartre sartre;
};

struct GameStateMenu {

};

struct GameStateResults {

};

struct InputResult {
	bool transition;
	GameMode transitionTo;
};

struct Textures {
	GLuint forestSartre[2];
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
    int argc;
    char **argv;
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

const int KARTTA_LEVEYS = 2048;
const int KARTTA_KORKEUS = 2048;
const int HAHMO_LEVEYS = 256;
const int HAHMO_KORKEUS = 256;
const int MAA_KORKEUS = 50;

const float HAHMO_VX = 800.0f;
const float HAHMO_G = 4000.0f;
const float HAHMO_HYPPYNOPEUS = 2100.0f;

SDL_Surface* format_sdl_surface(SDL_Surface *surface)
{
	if (!surface) {
		printf("Error: SDL surface null.\n");
		return nullptr;
	}
	if (!surface->w || !surface->h || (surface->w & 1) || (surface->h & 1)) {
		printf("Error: Invalid SDL surface.\n");
		return nullptr;
	}

	SDL_Surface* formattedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
	if (!formattedSurface) {
		printf("Error: Could not format a surface: %s\n", SDL_GetError());
		return nullptr;
	}
	return formattedSurface;
}

void load_images(Textures &textures, Surfaces &surfaces, std::string dataPath)
{
	// Load textures
	SDL_Surface *forestSartreImage[2];
	SDL_Surface *forestTaustaImage;

	forestSartreImage[0] = IMG_Load((dataPath + "images/sartre.png").c_str());
	if (!forestSartreImage[0]) {
	    printf("Error loading image: %s\n", SDL_GetError());
	    exit(1);
	}

	forestSartreImage[1] = IMG_Load((dataPath + "images/sartre2.png").c_str());
	if (!forestSartreImage[1]) {
	    printf("Error loading image: %s\n", SDL_GetError());
	    exit(1);
	}

	forestTaustaImage = IMG_Load((dataPath + "images/lehto.png").c_str());
	if (!forestTaustaImage) {
	    printf("Error loading image: %s\n", SDL_GetError());
	    exit(1);
	}

	// Sartret
	glGenTextures(2, textures.forestSartre);
	for (int i = 0; i < 2; i++) {
		SDL_Surface* formattedSurface = format_sdl_surface(forestSartreImage[i]);
		if (!formattedSurface) {
			exit(1);
		}
		glBindTexture(GL_TEXTURE_2D, textures.forestSartre[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formattedSurface->w, formattedSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, formattedSurface->pixels);
		SDL_FreeSurface(formattedSurface);
		SDL_FreeSurface(forestSartreImage[i]);
	}

	SDL_Surface* formattedSurface = format_sdl_surface(forestTaustaImage);
	if (!formattedSurface) {
		exit(1);
	}
	glGenTextures(1, textures.forestTausta);
	glBindTexture(GL_TEXTURE_2D, textures.forestTausta[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formattedSurface->w, formattedSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, formattedSurface->pixels);
	SDL_FreeSurface(formattedSurface);
	SDL_FreeSurface(forestTaustaImage);

	// Load collision map
	surfaces.forestCollisionMap = format_sdl_surface(IMG_Load((dataPath + "images/lehto_platforms.png").c_str()));
	if (!surfaces.forestCollisionMap) {
	    printf("Error loading image: %s\n", SDL_GetError());
	    exit(1);
	}
}

void free_images(Textures &textures, Surfaces &surfaces)
{
	for (int a = 0; a < 2; a++) {
		glDeleteTextures(1, &textures.forestSartre[a]);
	}
	glDeleteTextures(1, &textures.forestTausta[0]);

	SDL_FreeSurface(surfaces.forestCollisionMap);
}

bool isPixelBlack(SDL_Surface* surface, int x, int y, Uint8 threshold = 50) {
	if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) {
		return false; // Out of bounds, consider it non-colliding (white)
	}

	// Calculate the position of the pixel's first byte in the pixel array
	Uint32 pixelIndex = y * surface->pitch + x * 4; // 4 bytes per pixel (RGBA32)
	Uint8* pixel = (Uint8*)surface->pixels + pixelIndex;

	// Extract the RGB components
	Uint8 red = pixel[0];
	Uint8 green = pixel[1];
	Uint8 blue = pixel[2];

	// Determine if the pixel is black by checking if all RGB values are below the threshold
	return red < threshold && green < threshold && blue < threshold;
}

void renderText(TTF_Font* font, const std::string& text, SDL_Color color, GLuint shader, GLuint VAO, GLuint VBO, float x, float y) {
	// Create an SDL surface with the text
	SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
	if (!surface) {
		printf("Failed to render text surface: %s\n", TTF_GetError());
		return;
	}

	// Ensure the surface has the expected format
	if (surface->format->BytesPerPixel != 4) {
		printf("Unexpected surface format: %d bytes per pixel\n", surface->format->BytesPerPixel);
		SDL_FreeSurface(surface);
		return;
	}

	// Create OpenGL texture and upload data
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Use glPixelStorei to set unpack alignment
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Manually buffer pixel data to handle pitch (row alignment) issues
	int mode = GL_RGBA;
	const int pitch = surface->pitch; // The bytes per row in the surface
	const int width = surface->w;
	const int height = surface->h;

	// Allocate buffer for tightly packed pixel data
	std::vector<unsigned char> pixels(width * height * 4); // 4 bytes per pixel for RGBA

	// Copy each row from surface->pixels to the new buffer
	for (int y = 0; y < height; ++y) {
		std::memcpy(
			&pixels[y * width * 4], // Target
			static_cast<unsigned char*>(surface->pixels) + y * pitch, // Source
			width * 4 // Number of bytes to copy
		);
	}

	// Upload to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, mode, width, height, 0, mode, GL_UNSIGNED_BYTE, pixels.data());

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Set the texture uniform and text color uniform
	GLint textColorLoc = glGetUniformLocation(shader, "textColor");
	glUniform4f(textColorLoc, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

	// Define the vertices and texture coordinates for a quad
	float w = static_cast<float>(width);
	float h = static_cast<float>(height);
	float vertices[] = {
		x,     y,     0.0f, 0.0f,
		x + w, y,     1.0f, 0.0f,
		x + w, y - h, 1.0f, 1.0f,
		x,     y - h, 0.0f, 1.0f 
	};

	// Bind the text VAO and update buffer data
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

	// Use the uploaded texture in your shader
	glBindTexture(GL_TEXTURE_2D, texture);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // Drawing the quad

	// Unbind the VAO and texture
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Cleanup
	glDeleteTextures(1, &texture);
	SDL_FreeSurface(surface);
}


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

WindowParams compute_window_params(bool fullscreen)
{
	WindowParams windowParams;
	SDL_DisplayMode DM;
	SDL_GetCurrentDisplayMode(0, &DM);

	int screenWidth = DM.w;
	int screenHeight = DM.h;

	// Pick smaller of the screen dimensions for viewport size.
	int viewportSize;
	if (fullscreen) {
		viewportSize = std::min(screenWidth, screenHeight);
	} else {
		// Include some extra space in windowed mode
		viewportSize = (std::min(screenWidth, screenHeight) / 4) * 3;
	}
	int windowWidth;
	int windowHeight;
	if(fullscreen) {
		windowWidth = screenWidth;
		windowHeight = screenHeight;
	} else {
		windowWidth = viewportSize;
		windowHeight = viewportSize;
	}
	windowParams.windowWidth = windowWidth;
	windowParams.windowHeight = windowHeight;
	windowParams.viewportSize = viewportSize;
	return windowParams;
}

void cleanup_render_context(RenderContext& context)
{
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

bool initialize_render_context(RenderContext& context, const std::string& dataPath, bool fullscreen)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("Error: SDL_Init: %s\n", SDL_GetError());
		return false;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	WindowParams windowParams = compute_window_params(fullscreen);
	int windowWidth = windowParams.windowWidth;
	int windowHeight = windowParams.windowHeight;

	Uint32 windowFlags = SDL_WINDOW_OPENGL;
	if (fullscreen) {
		windowFlags |= SDL_WINDOW_FULLSCREEN;
	}

	SDL_Window *window = SDL_CreateWindow(
	                         "Sartre lehdossa inhottavien asioiden ympäroimänä",
	                         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                         windowWidth, windowHeight,
	                         windowFlags
	                     );

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

	// Fonts
	if (TTF_Init() == -1) {
		printf("SDL could not initialize! SDL_Error: %s\n", TTF_GetError());
		return -1;
	}

	TTF_Font* font = TTF_OpenFont((dataPath + "fonts/Roboto-Regular.ttf").c_str(), 50);
	if (font == nullptr) {
		printf("Fonts could not be initialized. TTF_OpenFont Error: %s\n", TTF_GetError());
		return -1;
	}
	context.font = font;

	// Music
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return -1;
	}

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
		return -1;
	}

	Mix_Music *backgroundMusic = Mix_LoadMUS((dataPath + "music/music.ogg").c_str());
	if (backgroundMusic == NULL) {
		printf("Failed to load background music! SDL_mixer Error: %s\n", Mix_GetError());
		return -1;
	}
	context.backgroundMusic = backgroundMusic;

	return true;
}

GameLoopData gameLoopData;

bool initialize_game_data(int argc, char **argv) {
	std::string dataPath = getResourcePath();

	if (argc > 1 && std::strcmp(argv[1], "--smoke") == 0) {
		std::cout << "Smoketest ran fine!" << std::endl;
		return false;
	}

	gameLoopData.fullscreen = (argc > 1 && std::strcmp(argv[1], "--fullscreen") == 0);

	if (!initialize_render_context(gameLoopData.context, dataPath, gameLoopData.fullscreen)) {
		cleanup_render_context(gameLoopData.context);
		return false;
	}

	WindowParams windowParams = compute_window_params(gameLoopData.fullscreen);

	// Compile shader program and create VAO and VBO for text rendering
	gameLoopData.context.textShaderProgram = createProgram(textVertexShaderSource, textFragmentShaderSource);
	glGenVertexArrays(1, &gameLoopData.context.textVAO);
	glGenBuffers(1, &gameLoopData.context.textVBO);
	glBindVertexArray(gameLoopData.context.textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, gameLoopData.context.textVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Compile shader program and create VAO and VBO for forest rendering
	gameLoopData.context.forestShaderProgram = createProgram(forestVertexShaderSource, forestFragmentShaderSource);
	glGenVertexArrays(1, &gameLoopData.context.forestVAO);
	glGenBuffers(1, &gameLoopData.context.forestVBO);
	glBindVertexArray(gameLoopData.context.forestVAO);
	glBindBuffer(GL_ARRAY_BUFFER, gameLoopData.context.forestVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glViewport((windowParams.windowWidth - windowParams.viewportSize) / 2,
	           (windowParams.windowHeight - windowParams.viewportSize) / 2,
	           windowParams.viewportSize,
	           windowParams.viewportSize);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	load_images(gameLoopData.imageData.textures, gameLoopData.imageData.surfaces, dataPath);

	gameLoopData.gameMode = MENU;
	gameLoopData.lastTick = SDL_GetTicks();
	gameLoopData.totalElapsed = 0;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	return true;
}

void main_loop_iteration() {
	/* Initialization and cleanup are kept within the loop function for the sake of webgl context which would not be found here
	   if initializated was outside.
	*/

	if (gameLoopData.shouldExit) {
		free_images(gameLoopData.imageData.textures, gameLoopData.imageData.surfaces);
		cleanup_render_context(gameLoopData.context);
		exit(1);
	}

	if (!gameLoopData.initialized) {
		if (!initialize_game_data(gameLoopData.argc, gameLoopData.argv)) {
			gameLoopData.shouldExit = true;
			return;
		}
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

int main(int argc, char **argv) {

	gameLoopData.argc = argc;
	gameLoopData.argv = argv;
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

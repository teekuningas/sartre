#define SDL_MAIN_HANDLED

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <cstring>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <GL/glew.h>

#include "shader_utils.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

// Triangle Vertex Shader Source
const char* triangleVertexShaderSource = R"glsl(
#version 300 es
precision mediump float;
layout(location = 0) in vec2 position;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(position, 0.0, 1.0);
}
)glsl";

// Triangle Fragment Shader Source
const char* triangleFragmentShaderSource = R"glsl(
#version 300 es
precision mediump float;
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0, 0.5, 0.2, 1.0); // Orange color
}
)glsl";

// Vertex Shader Source for Text Rendering
const char* textVertexShaderSource = R"glsl(
#version 300 es
precision mediump float;
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;
out vec2 fragTexCoord;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(position, 0.0, 1.0);
    fragTexCoord = texCoord;
}
)glsl";

// Fragment Shader Source for Text Rendering
const char* textFragmentShaderSource = R"glsl(
#version 300 es
precision mediump float;
in vec2 fragTexCoord;
out vec4 fragColor;
uniform sampler2D textTexture;
uniform vec4 textColor;
void main() {
    vec4 sampled = texture(textTexture, fragTexCoord);
    fragColor = textColor * sampled;
}
)glsl";

// Function to create an orthographic projection matrix
void createOrthographicMatrix(float left, float right, float bottom, float top, float near, float far, float* matrix) {
	std::fill(matrix, matrix + 16, 0.0f);
	matrix[0] = 2.0f / (right - left);
	matrix[5] = 2.0f / (top - bottom);
	matrix[10] = -2.0f / (far - near);
	matrix[12] = -(right + left) / (right - left);
	matrix[13] = -(top + bottom) / (top - bottom);
	matrix[14] = -(far + near) / (far - near);
	matrix[15] = 1.0f;
}

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

struct SDLContext {
	SDL_Window* window = nullptr;
	SDL_GLContext glContext = nullptr;
	TTF_Font* font = nullptr;
	Mix_Music* backgroundMusic = nullptr;
};

struct WindowParams {
	int windowHeight;
	int windowWidth;
	int viewportSize;
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
	if (!surface || !surface->w || !surface->h || (surface->w & 1) || (surface->h & 1)) {
		std::cerr << "Error: Invalid SDL surface." << std::endl;
		return nullptr; // Indicate failure with nullptr
	}

	return SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
}


void load_images(Textures &textures, Surfaces &surfaces, std::string dataPath)
{

}

void free_images(Textures &textures, Surfaces &surfaces)
{

}

bool isPixelBlack(SDL_Surface* surface, int x, int y, Uint8 threshold = 50)
{
	if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) {
		return false; // Out of bounds, consider it non-colliding (white)
	}

	Uint8 pixel = *((Uint8*)surface->pixels + y * surface->pitch + x); // Direct pixel access for 8-bit grayscale

	return pixel < threshold; // Black if below the threshold
}

void renderText(TTF_Font* font, const std::string& text, SDL_Color color, GLuint shader, GLuint VAO, GLuint VBO, float x, float y) {
	// Create an SDL surface with the text
	SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
	if (!surface) {
		std::cerr << "Failed to render text surface: " << TTF_GetError() << std::endl;
		return;
	}

	// Create OpenGL texture and upload data
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Texture upload
	int mode = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, mode, surface->w, surface->h, 0, mode, GL_UNSIGNED_BYTE, surface->pixels);

	// Specify the texture uniform and text color uniform
	GLint textTextureLoc = glGetUniformLocation(shader, "textTexture");
	glUniform1i(textTextureLoc, 0); // Texture unit 0
	GLint textColorLoc = glGetUniformLocation(shader, "textColor");
	glUniform4f(textColorLoc, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

	// Define the vertices and texture coordinates for a quad
	float w = static_cast<float>(surface->w);
	float h = static_cast<float>(surface->h);
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

	// Draw the quad using the uploaded vertex data
	glBindTexture(GL_TEXTURE_2D, texture);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

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

}

void forest_draw(GameStateForest &gameStateForest, Textures &textures)
{

}

InputResult forest_update(GameStateForest &gameStateForest, Uint32 totalElapsed, float deltaTime, Surfaces &surfaces)
{
	InputResult inputResult;
	return inputResult;
}

void results_init(GameStateResults &gameStateResults)
{

}

void results_draw(TTF_Font* font)
{

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

    // Render text using the modern renderText implementation
    renderText(font, "Jean-Paul Sartre istui metsän keskellä, ", white, textShaderProgram, VAO, VBO, 300.0f, 1300.0f);
    renderText(font, "lehtien kahistessa ympärillään, ja kirjoitti uutta kirjaansa,", white, textShaderProgram, VAO, VBO, 300.0f, 1200.0f);
    renderText(font, "kun äkkiä metsän syvyyksistä alkoi hiipiä häiritseviä varjoja, ", white, textShaderProgram, VAO, VBO, 300.0f, 1100.0f);
    renderText(font, "jotka uhkasivat keskeyttää hänen luomisprosessinsa.", white, textShaderProgram, VAO, VBO, 300.0f, 1000.0f);
    renderText(font, "Jatka näpsäyttämällä entteriä", white, textShaderProgram, VAO, VBO, 600.0f, 500.0f);
}

void menu_draw_triangle(TTF_Font* font, GLuint shaderProgram, GLuint VAO, GLuint VBO)
{

	glUseProgram(shaderProgram);

	// Set up the orthographic projection
	float orthoMatrix[16];
	createOrthographicMatrix(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, orthoMatrix);

	// Pass the projection matrix to the shader
	GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, orthoMatrix);

	glBindVertexArray(VAO);

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw the triangle
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// Cleanup
	glBindVertexArray(0);

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
	printf("Screen width: %d\n", screenWidth);
	printf("Screen height: %d\n", screenHeight);

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

bool initialize_sdl(SDLContext& context, const std::string& dataPath, bool fullscreen)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("Error: SDL_Init: %s\n", SDL_GetError());
		return false;
	}

	// Set the OpenGL version here
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	// Core profile for OpenGL ES
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	// Request at least 32-bit depth buffer (or as necessary)
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

	// Initialize GLEW to setup OpenGL Function pointers
	// Make sure GLEW is initialized after the GL context
	glewExperimental = GL_TRUE; // Needed for OpenGL core profile
	GLenum glewStatus = glewInit();
	if (glewStatus != GLEW_OK) {
		printf("Error: glewInit failed: %s\n", glewGetErrorString(glewStatus));
		return false;
	}

	// Check if OpenGL and GLEW versions are correct.
	printf("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
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


void cleanup_sdl(SDLContext& context)
{

	SDL_GL_DeleteContext(context.glContext);
	SDL_DestroyWindow(context.window);

	SDL_Quit();

	if (context.backgroundMusic) {
		Mix_FreeMusic(context.backgroundMusic);
	}
	Mix_CloseAudio();
	Mix_Quit();

	if (context.font) {
		TTF_CloseFont(context.font);
	}
	TTF_Quit();

	if (context.glContext) {
		SDL_GL_DeleteContext(context.glContext);
	}
	if (context.window) {
		SDL_DestroyWindow(context.window);
	}

	SDL_Quit();
}



int main(int argc, char **argv)
{

	std::string dataPath = getResourcePath();

	// Tee savutesti
	if (argc > 1 && std::strcmp(argv[1], "--smoke") == 0) {
		std::cout << "Smoketest ran fine!" << std::endl;
		return 0;
	}

	bool fullscreen = false;
	if (argc > 1 && std::strcmp(argv[1], "--fullscreen") == 0) {
		fullscreen = true;
	}

	SDLContext context;
	if (!initialize_sdl(context, dataPath, fullscreen)) {
		cleanup_sdl(context);
		return -1;
	}

	// Initialize GLEW for modern OpenGL functionality
	if (glewInit() != GLEW_OK) {
		printf("Error: glewInit failed.\n");
		return -1;
	}

	WindowParams windowParams = compute_window_params(fullscreen);
	int windowWidth = windowParams.windowWidth;
	int windowHeight = windowParams.windowHeight;
	int viewportSize = windowParams.viewportSize;

	// Compile shader program and create VAO and VBO for the test triangle
	GLuint triangleVAO, triangleVBO;
	GLuint triangleShaderProgram = createProgram(triangleVertexShaderSource, triangleFragmentShaderSource);
	if (!triangleShaderProgram) {
		std::cerr << "Failed to create shader program" << std::endl;
		return -1;
	}
	float vertices[] = {
		0.0f,  0.5f,  // Vertex 1
		-0.5f, -0.5f, // Vertex 2
		0.5f, -0.5f  // Vertex 3
	};
	glGenVertexArrays(1, &triangleVAO);
	glGenBuffers(1, &triangleVBO);
	glBindVertexArray(triangleVAO);
	glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Compile shader program and create VAO and VBO for text rendering
	GLuint textShaderProgram = createProgram(textVertexShaderSource, textFragmentShaderSource);
	GLuint textVAO, textVBO;
	glGenVertexArrays(1, &textVAO);
	glGenBuffers(1, &textVBO);
	glBindVertexArray(textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glViewport((windowWidth - viewportSize) / 2, (windowHeight - viewportSize) / 2, viewportSize, viewportSize);

	GameStateMenu gameStateMenu;
	GameStateForest gameStateForest;
	GameStateResults gameStateResults;

	// Lattaa kaikki tekstuurit heti alkuun
	ImageData imageData;
	Textures &textures = imageData.textures;
	Surfaces &surfaces = imageData.surfaces;
	load_images(textures, surfaces, dataPath);

	GameMode gameMode = MENU;

	Uint32 lastTick = SDL_GetTicks();
	Uint32 currentTick = 0;
	Uint32 totalElapsed = 0;
	float deltaTime = 0.0f;

	while (true) {
		InputResult inputResult;

		inputResult = handle_events(gameMode, fullscreen);
		if (inputResult.transition == true) {
			if (inputResult.transitionTo == EXIT) {
				break;
			}
			if (inputResult.transitionTo == FOREST) {
				// Start the music
				if (Mix_PlayMusic(context.backgroundMusic, -1) == -1) {
					printf("Failed to play background music! SDL_mixer Error: %s\n", Mix_GetError());
					cleanup_sdl(context);
					return -1;
				}
				forest_init(gameStateForest);
			} else {
				Mix_HaltMusic();
			}
			if (inputResult.transitionTo == MENU) {
				menu_init(gameStateMenu);
			}
			if (inputResult.transitionTo == RESULTS) {
				results_init(gameStateResults);
			}
			gameMode = inputResult.transitionTo;
			continue;
		}

		currentTick = SDL_GetTicks();
		deltaTime = (currentTick - lastTick) / 1000.0f;
		totalElapsed += currentTick - lastTick;
		lastTick = currentTick;

		switch (gameMode) {
		case MENU:
			inputResult = menu_update(gameStateMenu, totalElapsed, deltaTime, surfaces);
			break;
		case FOREST:
			inputResult = forest_update(gameStateForest, totalElapsed, deltaTime, surfaces);
			break;
		case RESULTS:
			inputResult = results_update(gameStateResults, totalElapsed, deltaTime, surfaces);
			break;
		}

		switch (gameMode) {
		case MENU:
			menu_draw_triangle(context.font, triangleShaderProgram, triangleVAO, triangleVBO);
			break;
		case FOREST:
			forest_draw(gameStateForest, textures);
			break;
		case RESULTS:
			results_draw(context.font);
			break;
		}

		SDL_GL_SwapWindow(context.window);

		SDL_Delay(1);
	}

	glDeleteVertexArrays(1, &triangleVAO);
	glDeleteBuffers(1, &triangleVBO);
	glDeleteVertexArrays(1, &textVAO);
	glDeleteBuffers(1, &textVBO);
	glDeleteProgram(triangleShaderProgram);
	glDeleteProgram(textShaderProgram);

	free_images(textures, surfaces);

	cleanup_sdl(context);
	return 0;



}

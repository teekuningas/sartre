#define SDL_MAIN_HANDLED

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <cstring>

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

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
	SDL_RWops *rwop;

	// Load textures
	SDL_Surface *forestSartreImage[2];
	SDL_Surface *forestTaustaImage;

	rwop = SDL_RWFromFile((dataPath + "images/sartre.png").c_str(), "rb");
	forestSartreImage[0] = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	rwop = SDL_RWFromFile((dataPath + "images/sartre2.png").c_str(), "rb");
	forestSartreImage[1] = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	rwop = SDL_RWFromFile((dataPath + "images/lehto.png").c_str(), "rb");
	forestTaustaImage = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	// Sartret
	glGenTextures(2, textures.forestSartre);
	for (int i = 0; i < 2; i++) {
		SDL_Surface* formattedSurface = format_sdl_surface(forestSartreImage[i]);
		if (!formattedSurface) {
			printf("Virhe: Could not format a surface: %s\n", SDL_GetError());
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
		printf("Virhe: Could not format a surface: %s\n", SDL_GetError());
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
	rwop = SDL_RWFromFile((dataPath + "images/lehto_platforms.png").c_str(), "rb");
	surfaces.forestCollisionMap = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

}
void free_images(Textures &textures, Surfaces &surfaces)
{
	// Free textures
	for (int a = 0; a < 2; a++) {
		glDeleteTextures(1, &textures.forestSartre[a]);
	}
	glDeleteTextures(1, &textures.forestTausta[0]);

	// Free surfaces
	SDL_FreeSurface(surfaces.forestCollisionMap);
}

bool isPixelBlack(SDL_Surface* surface, int x, int y, Uint8 threshold = 50)
{
	if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) {
		return false; // Out of bounds, consider it non-colliding (white)
	}

	Uint8 pixel = *((Uint8*)surface->pixels + y * surface->pitch + x); // Direct pixel access for 8-bit grayscale

	return pixel < threshold; // Black if below the threshold
}

void renderText(TTF_Font* font, const std::string& text, SDL_Color color, float x, float y)
{
	// Create SDL surface with text
	SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);

	// Create OpenGL texture from the surface
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Upload texture to OpenGL
	int mode = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
	glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->pitch / surface->format->BytesPerPixel);
	glTexImage2D(GL_TEXTURE_2D, 0, mode, surface->w, surface->h, 0, mode, GL_UNSIGNED_BYTE, surface->pixels);

	// Texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Enable transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);

	// Render the texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Set up the quad vertices
	float w = static_cast<float>(surface->w);
	float h = static_cast<float>(surface->h);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(x, y);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(x + w, y);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(x + w, y - h);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(x, y - h);
	glEnd();

	// Disable texturing
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	// Clean up
	glDeleteTextures(1, &textureID);
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
void forest_draw(GameStateForest &gameStateForest, Textures &textures)
{
	Sartre &sartre = gameStateForest.sartre;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-KARTTA_LEVEYS / 2, KARTTA_LEVEYS / 2, 0.0f, KARTTA_KORKEUS, -100.0f, 100.0f);
	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
	glTranslatef(sartre.x, sartre.y, 0.1f); // foreground

	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);

	if (sartre.hahmo == 0) glBindTexture(GL_TEXTURE_2D, textures.forestSartre[0]);
	else glBindTexture(GL_TEXTURE_2D, textures.forestSartre[1]);

	glBegin(GL_QUADS);
	glTexCoord2f(0.01f, -0.99f);
	glVertex3f(-HAHMO_LEVEYS/2, HAHMO_KORKEUS/2, 0.0f); // Top Left
	glTexCoord2f(0.99f, -0.99f);
	glVertex3f(HAHMO_LEVEYS/2, HAHMO_KORKEUS/2, 0.0f); // Top Right
	glTexCoord2f(0.99f, 0.01f);
	glVertex3f(HAHMO_LEVEYS/2, -HAHMO_KORKEUS/2, 0.0f); // Bottom Right
	glTexCoord2f(0.01f, 0.01f);
	glVertex3f(-HAHMO_LEVEYS/2, -HAHMO_KORKEUS/2, 0.0f); // Bottom Left
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	glLoadIdentity();

	glTranslatef(0.0f, 0.0f, 0.0f);
	glBindTexture(GL_TEXTURE_2D, textures.forestTausta[0]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, -1.0f);
	glVertex3f(-KARTTA_LEVEYS / 2, KARTTA_KORKEUS, 0.0f); // Top Left
	glTexCoord2f(1.0f, -1.0f);
	glVertex3f(KARTTA_LEVEYS / 2, KARTTA_KORKEUS, 0.0f); // Top Right
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(KARTTA_LEVEYS / 2, 0.0f, 0.0f); // Bottom Right
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-KARTTA_LEVEYS / 2, 0.0f, 0.0f); // Bottom Left
	glEnd();

	glDisable(GL_TEXTURE_2D);
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

void results_draw(TTF_Font* font)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, KARTTA_LEVEYS, 0, KARTTA_KORKEUS, -1, 1);

	glMatrixMode(GL_MODELVIEW);

	SDL_Color white = {255, 255, 255, 255};
	renderText(font, "Ei ole kirjailijan työ aina helppoa!", white, 300.0f, 1000.0f);
	renderText(font, "Jatka näpsäyttämällä entteriä", white, 600.0f, 500.0f);


}

InputResult results_update(GameStateResults &gameStateResults, Uint32 totalElapsed, float deltaTime, Surfaces &surfaces)
{
	InputResult inputResult;
	return inputResult;
}

void menu_init(GameStateMenu &gameStateMenu)
{

}

void menu_draw(TTF_Font* font)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, KARTTA_LEVEYS, 0, KARTTA_KORKEUS, -1, 1);

	glMatrixMode(GL_MODELVIEW);

	SDL_Color white = {255, 255, 255, 255};
	renderText(font, "Jean-Paul Sartre istui metsän keskellä, ", white, 300.0f, 1300.0f);
	renderText(font, "lehtien kahistessa ympärillään, ja kirjoitti uutta kirjaansa,", white, 300.0f, 1200.0f);
	renderText(font, "kun äkkiä metsän syvyyksistä alkoi hiipiä häiritseviä varjoja, ", white, 300.0f, 1100.0f);
	renderText(font, "jotka uhkasivat keskeyttää hänen luomisprosessinsa.", white, 300.0f, 1000.0f);
	renderText(font, "Jatka näpsäyttämällä entteriä", white, 600.0f, 500.0f);

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

bool initialize_sdl(SDLContext& context, const std::string& dataPath, bool fullscreen)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("Virhe: SDL_Init: %s\n", SDL_GetError());
		return false;
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	WindowParams windowParams = compute_window_params(fullscreen);
	int windowWidth = windowParams.windowWidth;
	int windowHeight = windowParams.windowHeight;
	int viewportSize = windowParams.viewportSize;

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
		printf("Virhe: SDL_CreateWindow: %s\n", SDL_GetError());
		return false;
	}
	context.window = window;

	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	if (!glContext) {
		printf("SDL could not initialize! SDL_GL_CreateContext: %s\n", SDL_GetError());
		return false;
	}
	context.glContext = glContext;

	// Fontit
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

	// Musiikki
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return -1;
	}

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
		return -1;
	}

	if (!(Mix_Init(MIX_INIT_MP3) & MIX_INIT_MP3)) {
		printf("Error initializing SDL_mixer: %s\n", Mix_GetError());
		return -1;
	}
	Mix_Music *backgroundMusic = Mix_LoadMUS((dataPath + "music/Myytti.mp3").c_str());
	if (backgroundMusic == NULL) {
		printf("Failed to load background music! SDL_mixer Error: %s\n", Mix_GetError());
		return -1;
	}
	context.backgroundMusic = backgroundMusic;

	return true;
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

	WindowParams windowParams = compute_window_params(fullscreen);
	int windowWidth = windowParams.windowWidth;
	int windowHeight = windowParams.windowHeight;
	int viewportSize = windowParams.viewportSize;

	glViewport((windowWidth - viewportSize) / 2, (windowHeight - viewportSize) / 2, viewportSize, viewportSize);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// glOrtho(-KARTTA_LEVEYS / 2, KARTTA_LEVEYS / 2, 0.0f, KARTTA_KORKEUS, -100.0f, 100.0f);
	// glMatrixMode(GL_MODELVIEW);
	// glLoadIdentity();

	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER, 0.1f);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glClearColor(0.0, 0.0, 0.0, 0.0);

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
			menu_draw(context.font);
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

	free_images(textures, surfaces);

	cleanup_sdl(context);
	return 0;
}

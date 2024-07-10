#define SDL_MAIN_HANDLED

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <cstring>

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

std::string getResourcePath()
{
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
	int selectedButton = 0;
};

struct GameStateResults {

};

struct InputResult {
	bool transition;
	GameMode transitionTo;
};

struct Textures {
	GLuint forestSartre[3];
	GLuint forestTausta[1];
};

struct Surfaces {
	SDL_Surface* forestCollisionMap;
};

struct ImageData {
	Textures textures;
	Surfaces surfaces;
};

const int KARTTA_LEVEYS = 2048;
const int KARTTA_KORKEUS = 2048;
const int HAHMO_LEVEYS = 250;
const int HAHMO_KORKEUS = 250;
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
	SDL_Surface *forestSartreImage[3];
	SDL_Surface *forestTaustaImage;

	rwop = SDL_RWFromFile((dataPath + "images/Sartre1a.png").c_str(), "rb");
	forestSartreImage[0] = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	rwop = SDL_RWFromFile((dataPath + "images/Sartre2a.png").c_str(), "rb");
	forestSartreImage[1] = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	rwop = SDL_RWFromFile((dataPath + "images/Sartre3a.png").c_str(), "rb");
	forestSartreImage[2] = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	rwop = SDL_RWFromFile((dataPath + "images/lehto.png").c_str(), "rb");
	forestTaustaImage = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	// Sartret
	glGenTextures(3, textures.forestSartre);
	for (int i = 0; i < 3; i++) {
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

	// Metsän tausta
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
	for (int a = 0; a < 4; a++) {
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
		}
	}
	return inputResult;
}

void forest_draw(GameStateForest &gameStateForest, Textures &textures)
{
	Sartre &sartre = gameStateForest.sartre;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glTranslatef(sartre.x, sartre.y, 0.1f); // foreground

	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);

	if (sartre.hahmo == 0) glBindTexture(GL_TEXTURE_2D, textures.forestSartre[0]);
	else if (sartre.hahmo == 1) glBindTexture(GL_TEXTURE_2D, textures.forestSartre[1]);
	else glBindTexture(GL_TEXTURE_2D, textures.forestSartre[2]);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, -1.0f);
	glVertex3f(-HAHMO_LEVEYS/2, HAHMO_KORKEUS/2, 0.0f); // Top Left
	glTexCoord2f(1.0f, -1.0f);
	glVertex3f(HAHMO_LEVEYS/2, HAHMO_KORKEUS/2, 0.0f); // Top Right
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(HAHMO_LEVEYS/2, -HAHMO_KORKEUS/2, 0.0f); // Bottom Right
	glTexCoord2f(0.0f, 0.0f);
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
}

InputResult forest_update(GameStateForest &gameStateForest, Uint32 totalElapsed, float deltaTime, Surfaces &surfaces)
{
	InputResult inputResult;
	inputResult.transition = false;

	Sartre &sartre = gameStateForest.sartre;

	const Uint8 *keystate = SDL_GetKeyboardState(NULL);

	// Vaihda hahmoa liikkeessä
	if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_LEFT]) {
		if (totalElapsed % 1000 <= 333) {
			sartre.hahmo = 0;
		} else if (totalElapsed % 1000 <= 666) {
			sartre.hahmo = 1;
		} else {
			sartre.hahmo = 2;
		}
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

	if (keystate[SDL_SCANCODE_ESCAPE]) {
		inputResult.transitionTo = EXIT;
		inputResult.transition = true;
	}

	return inputResult;
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

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("Virhe: SDL_Init: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

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

	Uint32 windowFlags = SDL_WINDOW_OPENGL;
	if (fullscreen) {
		windowFlags |= SDL_WINDOW_FULLSCREEN;
	}
	SDL_Window *window = SDL_CreateWindow(
	                         "Sartre lehdossa inhottavien asioiden ympäröimänä",
	                         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                         windowWidth, windowHeight,
	                         windowFlags
	                     );

	if (!window) {
		std::cerr << "Virhe: SDL_CreateWindow: " << SDL_GetError() << std::endl;
		exit(2);
	}

	SDL_GLContext context = SDL_GL_CreateContext(window);
	if (!context) {
		std::cerr << "Virhe: SDL_GL_CreateContext: " << SDL_GetError() << std::endl;
		SDL_DestroyWindow(window);
		exit(2);
	}
	glViewport((windowWidth - viewportSize) / 2, (windowHeight - viewportSize) / 2, viewportSize, viewportSize);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(-KARTTA_LEVEYS / 2, KARTTA_LEVEYS / 2, 0.0f, KARTTA_KORKEUS, -100.0f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
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

	// Alusta tila
	GameStateForest gameStateForest;
	Sartre &sartre = gameStateForest.sartre;

	// Alusta hahmo
	sartre.x = 0.0;
	sartre.y = HAHMO_KORKEUS / 2 + MAA_KORKEUS;
	sartre.hahmo = 0;
	sartre.hyppy = 0;

	// Lattaa kaikki tekstuurit heti alkuun
	ImageData imageData;
	Textures &textures = imageData.textures;
	Surfaces &surfaces = imageData.surfaces;
	load_images(textures, surfaces, dataPath);

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

	if (Mix_PlayMusic(backgroundMusic, -1) == -1) {
		printf("Failed to play background music! SDL_mixer Error: %s\n", Mix_GetError());
		return -1;
	}

	GameMode gameMode = FOREST;

	Uint32 lastTick = SDL_GetTicks();
	Uint32 currentTick = 0;
	Uint32 totalElapsed = 0;
	float deltaTime = 0.0f;

	while (true) {
		InputResult inputResult;
		inputResult = handle_events(gameMode, fullscreen);
		if (inputResult.transition == true && inputResult.transitionTo == EXIT) {
			break;
		}

		currentTick = SDL_GetTicks();
		deltaTime = (currentTick - lastTick) / 1000.0f;
		totalElapsed += currentTick - lastTick;
		lastTick = currentTick;

		// Päivitä
		if (gameMode == FOREST) {
			inputResult = forest_update(gameStateForest, totalElapsed, deltaTime, surfaces);
			if (inputResult.transition == true && inputResult.transitionTo == EXIT) {
				break;
			}
		}

		// Piirrä
		if (gameMode == FOREST) {
			forest_draw(gameStateForest, textures);
		}

		SDL_GL_SwapWindow(window);

		SDL_Delay(1);
	}

	free_images(textures, surfaces);

	// Tuhoa loput
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}

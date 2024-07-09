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
	GLfloat vx;
	GLfloat alkupy;
	int hahmo;
	bool hyppy;
};

struct Tausta {
	GLfloat x;
	GLfloat y;
};

enum GameMode { MENU, FOREST, RESULTS, EXIT };

struct GameStateForest {
	Sartre sartre;
	Tausta tausta;
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

const int IKKUNA_LEVEYS = 1024;
const int IKKUNA_KORKEUS = 1024;
const int KARTTA_LEVEYS = 2048;
const int KARTTA_KORKEUS = 2048;
const int HAHMO_LEVEYS = 500;
const int HAHMO_KORKEUS = 500;

SDL_Surface* format_sdl_surface(SDL_Surface *surface)
{
	if (!surface || !surface->w || !surface->h || (surface->w & 1) || (surface->h & 1)) {
		std::cerr << "Error: Invalid SDL surface." << std::endl;
		return nullptr; // Indicate failure with nullptr
	}

	return SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
}

void load_textures(Textures &textures, std::string dataPath)
{
	SDL_Surface *forestSartreImage[3];
	SDL_Surface *forestTaustaImage;

	// Lataa kuvadata heti alkuun
	SDL_RWops *rwop;
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

}
void free_textures(Textures &textures)
{
	for (int a = 0; a < 4; a++) {
		glDeleteTextures(1, &textures.forestSartre[a]);
	}
	glDeleteTextures(1, &textures.forestTausta[0]);
}

InputResult handle_input(GameMode &gameMode)
{
	SDL_Event event;

	InputResult inputResult;
	inputResult.transition = false;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			inputResult.transitionTo = EXIT;
			inputResult.transition = true;
		}
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
			inputResult.transitionTo = EXIT;
			inputResult.transition = true;
		}
	}
	return inputResult;
}

void forest_draw(GameStateForest &gameStateForest, Textures &textures)
{
	Sartre &sartre = gameStateForest.sartre;
	Tausta &tausta = gameStateForest.tausta;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glTranslatef(sartre.x, sartre.y, 0.0f);

	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);

	if (sartre.hahmo == 0) glBindTexture(GL_TEXTURE_2D, textures.forestSartre[0]);
	else if (sartre.hahmo == 1) glBindTexture(GL_TEXTURE_2D, textures.forestSartre[1]);
	else glBindTexture(GL_TEXTURE_2D, textures.forestSartre[2]);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, -1.0f);
	glVertex3f(-HAHMO_LEVEYS/2, HAHMO_KORKEUS/2, 1.0f); // Top Left
	glTexCoord2f(1.0f, -1.0f);
	glVertex3f(HAHMO_LEVEYS/2, HAHMO_KORKEUS/2, 1.0f); // Top Right
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(HAHMO_LEVEYS/2, -HAHMO_KORKEUS/2, 1.0f); // Bottom Right
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-HAHMO_LEVEYS/2, -HAHMO_KORKEUS/2, 1.0f); // Bottom Left
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	glLoadIdentity();

	glTranslatef(tausta.x, tausta.y, 0.0f);
	glBindTexture(GL_TEXTURE_2D, textures.forestTausta[0]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, -1.0f);
	glVertex3f(-KARTTA_LEVEYS, KARTTA_KORKEUS*2, 0.0f); // Top Left
	glTexCoord2f(1.0f, -1.0f);
	glVertex3f(KARTTA_LEVEYS, KARTTA_KORKEUS*2, 0.0f); // Top Right
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(KARTTA_LEVEYS, 0.0f, 0.0f); // Bottom Right
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-KARTTA_LEVEYS, 0.0f, 0.0f); // Bottom Left
	glEnd();
}

void forest_update(GameStateForest &gameStateForest, Uint32 totalElapsed, float deltaTime)
{
	Sartre &sartre = gameStateForest.sartre;
	Tausta &tausta = gameStateForest.tausta;

	// Vaihda hahmoa
	if (totalElapsed % 1000 <= 333) {
		sartre.hahmo = 0;
	} else if (totalElapsed % 1000 <= 666) {
		sartre.hahmo = 1;
	} else {
		sartre.hahmo = 2;
	}

	const Uint8 *keystate = SDL_GetKeyboardState(NULL);

	if (keystate[SDL_SCANCODE_RIGHT]) {
		sartre.x = sartre.x + deltaTime*sartre.vx;
	}

	if (keystate[SDL_SCANCODE_LEFT]) {
		sartre.x = sartre.x - deltaTime*sartre.vx;
	}

	if (sartre.hyppy == 0 && keystate[SDL_SCANCODE_UP]) {
		sartre.hyppy = 1;
		sartre.vy = 3000;
		sartre.alkupy = sartre.y;
	}

	if (sartre.hyppy == 1) {
		sartre.y = sartre.y + deltaTime*sartre.vy;
		sartre.vy = sartre.vy - deltaTime*5000;
		if (sartre.y < sartre.alkupy) {
			sartre.hyppy = 0;
			sartre.y = sartre.alkupy;
		}
	}
}


int main(int argc, char **argv)
{

	std::string dataPath = getResourcePath();

	// Tee savutesti
	if (argc > 1 && std::strcmp(argv[1], "--smoke") == 0) {
		std::cout << "Smoketest ran fine!" << std::endl;
		return 0;
	}

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("Virhe: SDL_Init: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	SDL_Window *window = SDL_CreateWindow(
	                         "Sartre temmeltää lehdossa nälkaisenä",
	                         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                         IKKUNA_LEVEYS, IKKUNA_KORKEUS,
	                         SDL_WINDOW_OPENGL | 0
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

	glViewport(0, 0, IKKUNA_LEVEYS, IKKUNA_KORKEUS);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(-KARTTA_LEVEYS, KARTTA_LEVEYS, 0.0f, KARTTA_KORKEUS*2, -100.0f, 100.0f);
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
	Tausta &tausta = gameStateForest.tausta;

	// Alusta hahmo
	sartre.x = 0.0;
	sartre.y = HAHMO_KORKEUS + 50.0f; // ground
	sartre.hahmo = 0;
	sartre.hyppy = 0;
	sartre.vx = 1000;

	// Alusta tausta
	tausta.x = 0.0;
	tausta.y = 0.0;

	// Lattaa kaikki tekstuurit heti alkuun
	Textures textures;
	load_textures(textures, dataPath);

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

		InputResult inputResult = handle_input(gameMode);
		if (inputResult.transition == true && inputResult.transitionTo == EXIT) {
			break;
		}

		currentTick = SDL_GetTicks();
		deltaTime = (currentTick - lastTick) / 1000.0f;
		totalElapsed += currentTick - lastTick;
		lastTick = currentTick;

		// Päivitä
		if (gameMode == FOREST) {
			forest_update(gameStateForest, totalElapsed, deltaTime);
		}

		// Piirrä
		if (gameMode == FOREST) {
			forest_draw(gameStateForest, textures);
		}

		SDL_GL_SwapWindow(window);

		SDL_Delay(1);
	}

	free_textures(textures);

	// Tuhoa loput
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}

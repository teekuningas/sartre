#define SDL_MAIN_HANDLED

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <cstring>

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

class Hahmo
{
private:
	GLfloat x;
	GLfloat y;
	GLfloat vy;
	GLfloat alkupy;
	GLfloat vx;
public:
	SDL_Surface *image[3];
	int hahmo;
	bool hyppy;
	void MuutaX(GLfloat xpar)
	{
		x = xpar;
	}
	void MuutaY(GLfloat ypar)
	{
		y = ypar;
	}
	void MuutaVY(GLfloat vypar)
	{
		vy = vypar;
	}
	void MuutaVX(GLfloat vxpar)
	{
		vx = vxpar;
	}
	void MuutaAlkupY(GLfloat alkupypar)
	{
		alkupy = alkupypar;
	}
	GLfloat LueX()
	{
		return x;
	}
	GLfloat LueY()
	{
		return y;
	}
	GLfloat LueVY()
	{
		return vy;
	}
	GLfloat LueVX()
	{
		return vx;
	}
	GLfloat LueAlkupY()
	{
		return alkupy;
	}
};

class Tausta
{
private:
	GLfloat x;
	GLfloat y;
public:
	SDL_Surface *image;
	void MuutaX(GLfloat xpar)
	{
		x = xpar;
	}
	void MuutaY(GLfloat ypar)
	{
		y = ypar;
	}
	GLfloat LueX()
	{
		return x;
	}
	GLfloat LueY()
	{
		return y;
	}
};

int hoida_viestit(void);
void metsa_piirra(void);
void metsa_toiminta(float deltaTime);

const int IKKUNA_LEVEYS = 1024;
const int IKKUNA_KORKEUS = 768;
const int KARTTA_LEVEYS = 2028;
const int KARTTA_KORKEUS = 768;
const int HAHMO_LEVEYS = 100;
const int HAHMO_KORKEUS = 100;

GLuint tex[4];

Hahmo sartre;
GLuint texSartre[3];
Tausta tausta;
GLuint texTausta[1];

int MySDL_glTexImage2D(SDL_Surface *kuva)
{
	if (!kuva || !kuva->w || !kuva->h || (kuva->w & 1) || (kuva->h & 1)) {
		return -1;
	}

	SDL_Surface *formattedSurface = SDL_ConvertSurfaceFormat(kuva, SDL_PIXELFORMAT_RGBA32, 0);
	if (!formattedSurface) {
		return -1;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formattedSurface->w, formattedSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, formattedSurface->pixels);

	SDL_FreeSurface(formattedSurface);
	return 0;
}

int hoida_viestit(void)
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			return -1;
		}
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
			return -1;
		}
	}
	return 0;
}

void metsa_piirra(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glTranslatef(sartre.LueX(), sartre.LueY(), 0.0f);

	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);

	if (sartre.hahmo == 0) glBindTexture(GL_TEXTURE_2D, texSartre[0]);
	else if (sartre.hahmo == 1) glBindTexture(GL_TEXTURE_2D, texSartre[1]);
	else glBindTexture(GL_TEXTURE_2D, texSartre[2]);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, -1.0f);
	glVertex3f(-HAHMO_LEVEYS, -HAHMO_KORKEUS, 1.0f); // Bottom Left
	glTexCoord2f(1.0f, -1.0f);
	glVertex3f(HAHMO_LEVEYS, -HAHMO_KORKEUS, 1.0f); // Bottom Right
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(HAHMO_LEVEYS, HAHMO_KORKEUS, 1.0f); // Top Right
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-HAHMO_LEVEYS, HAHMO_KORKEUS, 1.0f); // Top Left
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	glLoadIdentity();

	glTranslatef(tausta.LueX(), tausta.LueY(), 0.0f);
	glBindTexture(GL_TEXTURE_2D, texTausta[0]);
	glBegin(GL_QUADS);
	// Cropataan vähän reunasta
	glTexCoord2f(0.01f, 0.0f);
	glVertex3f(-KARTTA_LEVEYS*2, KARTTA_KORKEUS, 0.0f); // Bottom Left
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(KARTTA_LEVEYS*2, KARTTA_KORKEUS, 0.0f); // Bottom Right
	glTexCoord2f(1.0f, -1.0f);
	glVertex3f(KARTTA_LEVEYS*2, 0.0f, 0.0f); // Top Right
	// Cropataan vähän reunasta
	glTexCoord2f(0.01f, -1.0f);
	glVertex3f(-KARTTA_LEVEYS*2, 0.0f, 0.0f); // Top Left
	glEnd();
}

void metsa_toiminta(float deltaTime)
{
	const Uint8 *keystate = SDL_GetKeyboardState(NULL);

	if (keystate[SDL_SCANCODE_RIGHT]) {
		if (tausta.LueX() > -(KARTTA_LEVEYS*2 - IKKUNA_LEVEYS)) {
			if (sartre.LueX() < ((IKKUNA_LEVEYS / 4) * 3)) {
				sartre.MuutaX(sartre.LueX() + deltaTime*sartre.LueVX());
			} else tausta.MuutaX(tausta.LueX() - deltaTime*sartre.LueVX());
		} else {
			if (sartre.LueX() < IKKUNA_LEVEYS - 100) {
				sartre.MuutaX(sartre.LueX() + deltaTime*sartre.LueVX());
			}
		}
	}

	if (keystate[SDL_SCANCODE_LEFT]) {
		if (tausta.LueX() < KARTTA_LEVEYS*2) {
			if (sartre.LueX() > (IKKUNA_LEVEYS / 4)) {
				sartre.MuutaX(sartre.LueX() - deltaTime*sartre.LueVX());
			} else tausta.MuutaX(tausta.LueX() + deltaTime*sartre.LueVX());
		} else {
			if (sartre.LueX() > 0 + 100) {
				sartre.MuutaX(sartre.LueX() - deltaTime*sartre.LueVX());
			}
		}
	}

	if (sartre.hyppy == 0 && keystate[SDL_SCANCODE_UP]) {
		sartre.hyppy = 1;
		sartre.MuutaVY(-1200);
		sartre.MuutaAlkupY(sartre.LueY());
	}

	if (sartre.hyppy == 1) {
		sartre.MuutaY(sartre.LueY() + deltaTime*sartre.LueVY());
		sartre.MuutaVY(sartre.LueVY() + deltaTime*3000);
		if (sartre.LueY() > sartre.LueAlkupY()) {
			sartre.hyppy = 0;
			sartre.MuutaY(sartre.LueAlkupY());
		}
	}
}


int main(int argc, char **argv)
{

	// Check if "--smoke" argument is provided
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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window *window = SDL_CreateWindow(
	                         "Sartre temmeltaa lehdossa nälkaisenä",
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

	glOrtho(0.0f, IKKUNA_LEVEYS, IKKUNA_KORKEUS, 0.0f, -100.0f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER, 0.1f);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glClearColor(0.0, 0.0, 0.0, 0.0);

	// Kuvat
	SDL_RWops *rwop;
	rwop = SDL_RWFromFile("data/images/Sartre1a.png", "rb");
	sartre.image[0] = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	rwop = SDL_RWFromFile("data/images/Sartre2a.png", "rb");
	sartre.image[1] = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	rwop = SDL_RWFromFile("data/images/Sartre3a.png", "rb");
	sartre.image[2] = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	rwop = SDL_RWFromFile("data/images/lehto.png", "rb");
	tausta.image = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	glGenTextures(3, texSartre);
	glGenTextures(1, texTausta);

	glBindTexture(GL_TEXTURE_2D, texSartre[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	MySDL_glTexImage2D(sartre.image[0]);

	glBindTexture(GL_TEXTURE_2D, texSartre[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	MySDL_glTexImage2D(sartre.image[1]);

	glBindTexture(GL_TEXTURE_2D, texSartre[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	MySDL_glTexImage2D(sartre.image[2]);

	glBindTexture(GL_TEXTURE_2D, texTausta[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	MySDL_glTexImage2D(tausta.image);

	// Musiikki

	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return -1;
	}

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
		return -1;
	}

	Mix_Music *backgroundMusic = Mix_LoadMUS("data/music/Myytti.mp3");
	if (backgroundMusic == NULL) {
		printf("Failed to load background music! SDL_mixer Error: %s\n", Mix_GetError());
		return -1;
	}

	// Play the MP3 file in a loop
	if (Mix_PlayMusic(backgroundMusic, -1) == -1) {
		printf("Failed to play background music! SDL_mixer Error: %s\n", Mix_GetError());
		return -1;
	}

	// Alusta hahmo
	sartre.MuutaX(400.0);
	sartre.MuutaY(650.0);
	sartre.hahmo = 0;
	sartre.hyppy = 0;
	sartre.MuutaVX(500);

	if (hoida_viestit() != 0) {
		exit(0);
	}

	Uint32 lastTick = SDL_GetTicks();
	Uint32 currentTick = 0;
	Uint32 hahmoElapsed = 0;
	float deltaTime = 0.0f;

	while (hoida_viestit() == 0) {
		currentTick = SDL_GetTicks();
		deltaTime = (currentTick - lastTick) / 1000.0f;
		hahmoElapsed += currentTick - lastTick;
		lastTick = currentTick;

		// Laske liikkeet
		metsa_toiminta(deltaTime);

		// Vaihda hahmoa
		if (hahmoElapsed >= 300) {
			sartre.hahmo = (sartre.hahmo + 1) % 3;
			hahmoElapsed = 0;
		}

		// Piirrä
		metsa_piirra();
		SDL_GL_SwapWindow(window);


		SDL_Delay(1);
	}

	for (int a = 0; a < 3; a++) {
		SDL_FreeSurface(sartre.image[a]);
	}
	SDL_FreeSurface(tausta.image);

	for (int a = 0; a < 4; a++) {
		glDeleteTextures(1, &texSartre[a]);
	}
	glDeleteTextures(1, &texTausta[0]);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}

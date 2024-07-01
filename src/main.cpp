#include <stdlib.h>
#include <stdio.h>
#include <iostream>

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
		x=xpar;
	}
	void MuutaY(GLfloat ypar)
	{
		y=ypar;
	}
	void MuutaVY(GLfloat vypar)
	{
		vy=vypar;
	}
	void MuutaVX(GLfloat vxpar)
	{
		vx=vxpar;
	}
	void MuutaAlkupY(GLfloat alkupypar)
	{
		alkupy=alkupypar;
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
		x=xpar;
	}
	void MuutaY(GLfloat ypar)
	{
		y=ypar;
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

const int SYVYYS = 16;
const int LEVEYS = 1024;
const int KORKEUS = 768;
const int IKKUNALIPPU = 0;
const int METSA_LEVEYS = 8192;
const int METSA_KORKEUS = 768;
const GLfloat gravKiiht = 5;

int syvyys, kaksoispuskurointi;

GLuint tex[4];

int hoida_viestit(void);
void metsa_piirra(void);
void metsa_toiminta(void);

const int fps = 20;
int lastTick = 0;
int tick = 0;



Hahmo Sartre;
Tausta tausta[1];

int x, y;

int MySDL_glTexImage2D(SDL_Surface *kuva)
{
	SDL_Surface *apu;

	if (!kuva || !kuva->w || !kuva->h || (kuva->w & 1) || (kuva->h & 1)) {
		return -1;
	}

	apu = SDL_CreateRGBSurfaceWithFormat(0, kuva->w, kuva->h, 32, SDL_PIXELFORMAT_RGBA32);
	if (!apu) {
		return -1;
	}

	SDL_FillRect(apu, NULL, 0);
	SDL_SetSurfaceAlphaMod(kuva, 0);
	SDL_SetSurfaceBlendMode(kuva, SDL_BLENDMODE_NONE);

	SDL_Rect r1, r2;
	r1.x = r2.x = 0;
	r1.h = r2.h = 1;
	r1.w = r2.w = kuva->w;
	for (r1.y = 0, r2.y = kuva->h - 1; r2.y >= 0; ++r1.y, --r2.y) {
		SDL_BlitSurface(kuva, &r1, apu, &r2);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, apu->w, apu->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, apu->pixels);

	SDL_FreeSurface(apu);
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
	glTranslatef(Sartre.LueX(), Sartre.LueY(), 0.0f);
	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);

	if (Sartre.hahmo == 0) glBindTexture(GL_TEXTURE_2D, tex[0]);
	else if (Sartre.hahmo == 1) glBindTexture(GL_TEXTURE_2D, tex[1]);
	else glBindTexture(GL_TEXTURE_2D, tex[2]);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-100.0f, -100.0f, 1.0f); // Bottom Left
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(100.0f, -100.0f, 1.0f); // Bottom Right
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(100.0f, 100.0f, 1.0f); // Top Right
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-100.0f, 100.0f, 1.0f); // Top Left
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	glLoadIdentity();
	glTranslatef(tausta[0].LueX(), tausta[0].LueY(), 0.0f);
	glBindTexture(GL_TEXTURE_2D, tex[3]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(0.0f, KORKEUS, 0.0f); // Bottom Left
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(LEVEYS, KORKEUS, 0.0f); // Bottom Right
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(LEVEYS, 0.0f, 0.0f); // Top Right
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f); // Top Left
	glEnd();
}

void metsa_toiminta(void)
{
	const Uint8 *keystate = SDL_GetKeyboardState(NULL);

	if (keystate[SDL_SCANCODE_RIGHT]) {
		if (tausta[0].LueX() > -3072) {
			if (Sartre.LueX() < ((LEVEYS / 4) * 3)) {
				Sartre.MuutaX(Sartre.LueX() + Sartre.LueVX());
			} else tausta[0].MuutaX(tausta[0].LueX() - Sartre.LueVX());
		} else {
			if (Sartre.LueX() < LEVEYS - 100) {
				Sartre.MuutaX(Sartre.LueX() + Sartre.LueVX());
			}
		}
	}

	if (keystate[SDL_SCANCODE_LEFT]) {
		if (tausta[0].LueX() < 4096) {
			if (Sartre.LueX() > (LEVEYS / 4)) {
				Sartre.MuutaX(Sartre.LueX() - Sartre.LueVX());
			} else tausta[0].MuutaX(tausta[0].LueX() + Sartre.LueVX());
		} else {
			if (Sartre.LueX() > 0 + 100) {
				Sartre.MuutaX(Sartre.LueX() - Sartre.LueVX());
			}
		}
	}

	if (Sartre.hyppy == 0 && keystate[SDL_SCANCODE_UP]) {
		Sartre.hyppy = 1;
		Sartre.MuutaVY(-50);
		Sartre.MuutaAlkupY(Sartre.LueY());
	}

	if (Sartre.hyppy == 1) {
		Sartre.MuutaY(Sartre.LueY() + Sartre.LueVY());
		Sartre.MuutaVY(Sartre.LueVY() + gravKiiht);
		if (Sartre.LueY() > Sartre.LueAlkupY()) {
			Sartre.hyppy = 0;
			Sartre.MuutaY(Sartre.LueAlkupY());
		}
	}
}


int main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("Virhe: SDL_Init: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window *window = SDL_CreateWindow(
	                         "Sartre temmeltaa lehdossa nalkaisena",
	                         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                         LEVEYS, KORKEUS,
	                         SDL_WINDOW_OPENGL | IKKUNALIPPU
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

	SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &syvyys);
	SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &kaksoispuskurointi);

	if (syvyys > SYVYYS) {
		printf("Syvyys on toivottua suurempi \\o/. Pyydettiin %d, saatiinkin %d...\n", SYVYYS, syvyys);
	} else if (syvyys < SYVYYS) {
		printf("Syvyys on toivottua pienempi :(. Haluttiin %d, saatiinkin %d...\n", SYVYYS, syvyys);
	}

	if (!kaksoispuskurointi) {
		printf("Ei ole kaksoispuskuria! Qu'est-ce que c'est que cela? O_o\n");
		printf("Nyt voi sattua hassuja...\n");
	}

	glViewport(0, 0, LEVEYS, KORKEUS);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0.0f, LEVEYS, KORKEUS, 0.0f, -100.0f, 100.0f);
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

	Sartre.MuutaX(400.0);
	Sartre.MuutaY(650.0);
	Sartre.hahmo = 0;
	Sartre.hyppy = 0;
	Sartre.MuutaVX(16);

	lastTick = SDL_GetTicks();

	SDL_RWops *rwop;
	rwop = SDL_RWFromFile("data/images/Sartre1a.png", "rb");
	Sartre.image[0] = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	rwop = SDL_RWFromFile("data/images/Sartre2a.png", "rb");
	Sartre.image[1] = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	rwop = SDL_RWFromFile("data/images/Sartre3a.png", "rb");
	Sartre.image[2] = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	rwop = SDL_RWFromFile("data/images/Lehto.png", "rb");
	tausta[0].image = IMG_LoadPNG_RW(rwop);
	SDL_RWclose(rwop);

	glGenTextures(4, tex);

	glBindTexture(GL_TEXTURE_2D, tex[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	MySDL_glTexImage2D(Sartre.image[0]);

	glBindTexture(GL_TEXTURE_2D, tex[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	MySDL_glTexImage2D(Sartre.image[1]);

	glBindTexture(GL_TEXTURE_2D, tex[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	MySDL_glTexImage2D(Sartre.image[2]);

	glBindTexture(GL_TEXTURE_2D, tex[3]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	MySDL_glTexImage2D(tausta[0].image);

	if (hoida_viestit() != 0) {
		exit(0);
	}

	while (hoida_viestit() == 0) {
		tick = SDL_GetTicks();

		if (tick <= lastTick) {
			SDL_Delay(5);
			continue;
		}

		if (lastTick >= tick)
			continue;

		while (lastTick < tick) {
			metsa_toiminta();
			lastTick += (1000 / fps);
		}

		metsa_piirra();
		if (kaksoispuskurointi) {
			SDL_GL_SwapWindow(window);
		}
	}

	for (int a = 0; a < 3; a++) {
		SDL_FreeSurface(Sartre.image[a]);
	}
	SDL_FreeSurface(tausta[0].image);

	for (int a = 0; a < 4; a++) {
		glDeleteTextures(1, &tex[a]);
	}

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}



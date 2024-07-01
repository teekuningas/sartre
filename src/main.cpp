#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#include "Hahmo.h"

const int VARIN_BITIT = 8;
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
void alusta(void);
void metsa_piirra(void);
void metsa_toiminta(void);
void lopetusfunktio(void);

const int fps = 20;
int lastTick = 0;
int tick = 0;

const Uint8 *keystate = SDL_GetKeyboardState(NULL);

Hahmo Sartre;
Tausta tausta[1];

int x, y;

int virhe = 0;

int MySDL_glTexImage2D(SDL_Surface *kuva)
{
	SDL_Surface *apu;
	/* Helpottaa, jos tavut ovat järjestyksessä RGBA.
	 * Säädetään siis konetyypin mukaan värien bittimaskit
	 * niin, että tavujen järjestys muistissa osuu oikein. */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	const Uint32 rshift = 24, gshift = 16, bshift = 8, ashift = 0;
#else
	const Uint32 rshift = 0, gshift = 8, bshift = 16, ashift = 24;
#endif
	const Uint32
	rmask = 0xff << rshift,
	gmask = 0xff << gshift,
	bmask = 0xff << bshift,
	amask = 0xff << ashift;
	Uint32 kuva_flags;
	Uint32 kuva_colorkey;
	Uint8 kuva_alpha;
	SDL_Rect r1, r2;

	/* Tarkistetaan kuva. OpenGL:ää varten sivun pitää olla kahdella jaollinen. */
	if (!kuva || !kuva->w || !kuva->h || (kuva->w & 1) || (kuva->h & 1)) {
		return -1;
	}

	/* Otetaan talteen arvot, jotka muuttuvat funktion aikana */
	kuva_flags = kuva->flags;

	SDL_GetSurfaceAlphaMod(kuva, &kuva_alpha);
	SDL_GetColorKey(kuva, &kuva_colorkey);

	/* Luodaan apupinta halutussa formaatissa (RGBA). */
	apu = SDL_CreateRGBSurface(SDL_SWSURFACE, kuva->w, kuva->h, 32, rmask, gmask, bmask, amask);
	if (!apu) {
		return -1;
	}
	SDL_FillRect(apu, 0, 0);
	SDL_SetSurfaceAlphaMod(kuva, 0);
	SDL_SetSurfaceBlendMode(kuva, SDL_BLENDMODE_NONE);

	Uint32 colorkey;
	if ((kuva_flags & SDL_TRUE) && SDL_GetColorKey(kuva, &colorkey) == 0 && kuva->format->Amask) {
		SDL_SetColorKey(kuva, SDL_FALSE, colorkey);
	}

	/* OpenGL:n ja SDL:n y-akselit osoittavat eri suuntiin.
	 * Kopioidaan siis kuva pikselirivi kerrallaan ylösalaisin. */
	r1.x = r2.x = 0;
	r1.h = r2.h = 1;
	r1.w = r2.w = kuva->w;
	for (r1.y = 0, r2.y = kuva->h - 1; r2.y >= 0; ++r1.y, --r2.y) {
		SDL_BlitSurface(kuva, &r1, apu, &r2);
	}

	/* Koko pinnan alfa-arvo pitää palauttaa erikseen, jos sellainen on. */
	if ((kuva_flags & SDL_BLENDMODE_BLEND) && !kuva->format->Amask && kuva_alpha != 0xff) {
		for (r1.y = 0; r1.y < apu->h; ++r1.y) {
			Uint32* ptr = (Uint32*)((Uint8*) apu->pixels + r1.y * apu->pitch);
			for (r1.x = 0; r1.x < apu->w; ++r1.x) {
				if ((ptr[r1.x] & amask) != 0) {
					ptr[r1.x] &= (kuva_alpha << ashift) | ~amask;
				}
			}
		}
	}

	/* Lähetetään kuva OpenGL:lle, tuhotaan apupinta ja palautetaan asetukset. */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, apu->w, apu->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, apu->pixels);

	SDL_FreeSurface(apu);
	SDL_SetSurfaceAlphaMod(kuva, kuva_alpha);

	SDL_BlendMode blendMode;
	if (kuva_flags & SDL_BLENDMODE_BLEND) {
		blendMode = SDL_BLENDMODE_BLEND;
	} else if (kuva_flags & SDL_BLENDMODE_ADD) {
		blendMode = SDL_BLENDMODE_ADD;
	} else if (kuva_flags & SDL_BLENDMODE_MOD) {
		blendMode = SDL_BLENDMODE_MOD;
	} else {
		blendMode = SDL_BLENDMODE_NONE;
	}
	SDL_SetSurfaceBlendMode(kuva, blendMode);

	SDL_SetColorKey(kuva, SDL_TRUE, kuva_colorkey);

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

void alusta(void)
{
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

	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER,0.1f);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glClearColor(0.0, 0.0, 0.0, 0.0);

	Sartre.MuutaX(400.0);
	Sartre.MuutaY(650.0);
	Sartre.hahmo = 0;
	Sartre.hyppy = 0;
	Sartre.MuutaVX(16);

	lastTick = SDL_GetTicks();

	SDL_RWops *rwop;
	rwop=SDL_RWFromFile("data/images/Sartre1a.png", "rb");
	Sartre.image[0]=IMG_LoadPNG_RW(rwop);

	rwop=SDL_RWFromFile("data/images/Sartre2a.png", "rb");
	Sartre.image[1]=IMG_LoadPNG_RW(rwop);

	rwop=SDL_RWFromFile("data/images/Sartre3a.png", "rb");
	Sartre.image[2]=IMG_LoadPNG_RW(rwop);

	rwop=SDL_RWFromFile("data/images/lehto2.jpg", "rb");
	tausta[0].image=IMG_LoadJPG_RW(rwop);
	glGenTextures(4, &tex[0]);

	glBindTexture(GL_TEXTURE_2D, tex[0]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	MySDL_glTexImage2D(Sartre.image[0]);

	glBindTexture(GL_TEXTURE_2D, tex[1]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	MySDL_glTexImage2D(Sartre.image[1]);

	glBindTexture(GL_TEXTURE_2D, tex[2]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	MySDL_glTexImage2D(Sartre.image[2]);

	glBindTexture(GL_TEXTURE_2D, tex[3]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	MySDL_glTexImage2D(tausta[0].image);

	if (hoida_viestit() != 0) {
		exit(virhe = 0);
	}

}


void metsa_piirra(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glTranslatef(Sartre.LueX(),Sartre.LueY(),0.0f);
	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);

	if(Sartre.hahmo == 0) glBindTexture(GL_TEXTURE_2D, tex[0]);
	else if(Sartre.hahmo == 1) glBindTexture(GL_TEXTURE_2D, tex[1]);
	else glBindTexture(GL_TEXTURE_2D, tex[2]);

	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-100.0f, -100.0f,  1.0f); // Bottom Left
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f( 100.0f, -100.0f,  1.0f); // Bottom Right
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f( 100.0f,  100.0f,  1.0f); // Top Right
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-100.0f,  100.0f,  1.0f); // Top Left
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	glLoadIdentity();
	glTranslatef(tausta[0].LueX(),tausta[0].LueY(),0.0f);
	glBindTexture(GL_TEXTURE_2D, tex[3]);
	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(0.0f, KORKEUS,  0.0f); // Bottom Left
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f( LEVEYS, KORKEUS,  0.0f); // Bottom Right
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f( LEVEYS, 0.0f,  0.0f); // Top Right
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f,  0.0f); // Top Left
	glEnd();
}

void metsa_toiminta(void)
{

	if ( keystate[SDLK_RIGHT] ) {
		// Jos ei olla vielä vasemmassa reunassa
		if(tausta[0].LueX() > -3072) {
			// Ruudusta neljäsosa näkötilaa skrollatessa
			if(Sartre.LueX() < ((LEVEYS/4)*3)) {
				Sartre.MuutaX(Sartre.LueX()+Sartre.LueVX());
			} else tausta[0].MuutaX(tausta[0].LueX()-Sartre.LueVX());
		} else {
			// Reunassa puolet hahmon leveydestä pois liikkumatilasta
			if ( Sartre.LueX() < LEVEYS - 100 ) {
				Sartre.MuutaX(Sartre.LueX()+Sartre.LueVX());
			}
		}
	}

	if ( keystate[SDLK_LEFT] ) {
		// Jos ei olla vielä oikeassa reunassa
		if (tausta[0].LueX() < 4096 ) {
			// Ruudusta neljäsosa näkötilaa skrollatessa
			if(Sartre.LueX() > (LEVEYS/4)) {
				Sartre.MuutaX(Sartre.LueX()-Sartre.LueVX());
			} else tausta[0].MuutaX(tausta[0].LueX()+Sartre.LueVX());
		} else {
			// Reunassa puolet hahmon leveydestä pois liikkumatilasta vuoksi
			if ( Sartre.LueX() > 0 + 100  ) {
				Sartre.MuutaX(Sartre.LueX()-Sartre.LueVX());
			}
		}
	}

	// Jos ylöspäin-nappi painettu, aloita hyppy. Säilytetään alkuperäinen y-koordinaatti hypyn ajan, jotta osataan pysäyttää oikessa kohdassa.
	if ( Sartre.hyppy == 0 && keystate[SDLK_UP] ) {
		Sartre.hyppy = 1;
		Sartre.MuutaVY(-50);
		Sartre.MuutaAlkupY(Sartre.LueY());
	}

	// Hypyn ollessa käynnissä
	if(Sartre.hyppy == 1) {
		Sartre.MuutaY(Sartre.LueY() + Sartre.LueVY());
		Sartre.MuutaVY(Sartre.LueVY() + gravKiiht);
		if(Sartre.LueY() > Sartre.LueAlkupY()) {
			Sartre.hyppy = 0;
			Sartre.MuutaY(Sartre.LueAlkupY());
		}
	}
}


void lopetusfunktio(void)
{
	for(int a = 0; a < 3; a++) {
		SDL_FreeSurface(Sartre.image[a]);
	}
	for(int a = 0; a < 3; a++) {
		glDeleteTextures(1, &tex[a]);
	}
	SDL_FreeSurface(tausta[0].image);

	SDL_Quit();
}

int main(int argc, char **argv)
{
	atexit(lopetusfunktio);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("Virhe: SDL_Init: %s\n", SDL_GetError());
		exit(virhe = 1);
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window* window = SDL_CreateWindow(
	                         "Sartre temmeltaa lehdossa nalkaisena",
	                         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                         LEVEYS, KORKEUS,
	                         SDL_WINDOW_OPENGL | IKKUNALIPPU
	                     );

	if (!window) {
		std::cerr << "Virhe: SDL_CreateWindow: " << SDL_GetError() << std::endl;
		exit(virhe = 2);
	}

	SDL_GLContext context = SDL_GL_CreateContext(window);
	if (!context) {
		std::cerr << "Virhe: SDL_GL_CreateContext: " << SDL_GetError() << std::endl;
		SDL_DestroyWindow(window);
		exit(virhe = 2);
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

	alusta();

	while (hoida_viestit() == 0) {
		tick = SDL_GetTicks();

		if (tick <= lastTick) {
			SDL_Delay(5);
			continue;
		}

		// ala piirra taikka laske fysiikoita jos ei ole tickkiakaan kulunut
		// this is just for if you comment out the SDL_Delay part..
		if( lastTick >= tick )
			continue;

		// Toiminta metsässä tahdistimella
		while(lastTick < tick) {
			metsa_toiminta();
			lastTick += ( 1000 / fps );
		}

		metsa_piirra();
		if (kaksoispuskurointi) {
			SDL_GL_SwapWindow(window);
		}

	}

	return virhe = 0;
}

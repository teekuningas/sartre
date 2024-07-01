#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

// N‰ihin on ehk‰ lis‰tt‰v‰ SDL/-etuliite, jos include-polkuja ei ole s‰‰detty.
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#include "Hahmo.h"

// Asetukset
const int VARIN_BITIT = 8; // 8 bitti‰ jokaiselle v‰rin komponentille
const int SYVYYS = 16;     // 16-bittinen syvyyspuskuri (turha t‰ll‰ kertaa)
const int LEVEYS = 1024;    // Ikkunan mittoja voi huoletta muuttaa
const int KORKEUS = 768;   // T‰ll‰ kertaa kuitenkin pieni ikkuna riitt‰‰
const int IKKUNALIPPU = 0; // tai SDL_FULLSCREEN
const int METSA_LEVEYS = 8192; // Mets‰tekstuurin leveys
const int METSA_KORKEUS = 768; // Mets‰tekstuurin korkeus
const GLfloat gravKiiht = 5; // gravitaatiokiihtyvyys

/* Mix_Music actually holds the music information.  */
//Mix_Music *music = NULL;

// Tarkistusta varten, jotta saadaan selville, mit‰ asetuksista tulikaan.
int syvyys, kaksoispuskurointi;

// glbindtexture-funktiolle tekstuurimuuttuja

GLuint tex[4];

// Funktiot
int hoida_viestit(void);
void alusta(void);
void metsa_piirra(void);
void metsa_toiminta(void);
void lopetusfunktio(void);
void musicDone(void);
void debuggausfunktio(void);


// Ohjelman kierrokseen kulunut aika (millisekunteina)

const int fps = 20;
int lastTick = 0;
int tick = 0;



// debug

Uint32 deb_alku = 0;
Uint32 deb_loppu = 0;
bool deb_mittaus = 0;
std::ofstream myfile;



Uint8 *keystate = SDL_GetKeyState(NULL);

Hahmo Sartre;
Tausta tausta[1];

int x, y;


// Virhemuuttuja, jotta lopetusfunktio hoitaa oikeat asiat
int virhe = 0;

// Itse suuri tekstuurinasetusfunktio

int MySDL_glTexImage2D(SDL_Surface *kuva)
{
	SDL_Surface *apu;
	/* Helpottaa, jos tavut ovat j‰rjestyksess‰ RGBA.
	 * S‰‰det‰‰n siis konetyypin mukaan v‰rien bittimaskit
	 * niin, ett‰ tavujen j‰rjestys muistissa osuu oikein. */
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
	Uint32 *ptr;
	Uint32 kuva_flags;
	Uint32 kuva_colorkey;
	Uint8 kuva_alpha;
	SDL_Rect r1, r2;

	/* Tarkistetaan kuva. OpenGL:‰‰ varten sivun pit‰‰ olla kahdella jaollinen. */
	if (!kuva || !kuva->w || !kuva->h || (kuva->w & 1) || (kuva->h & 1)) {
		return -1;
	}

	/* Otetaan talteen arvot, jotka muuttuvat funktion aikana */
	kuva_flags = kuva->flags;
	kuva_alpha = kuva->format->alpha;
	kuva_colorkey = kuva->format->colorkey;

	/* Luodaan apupinta halutussa formaatissa (RGBA). */
	apu = SDL_CreateRGBSurface(SDL_SWSURFACE, kuva->w, kuva->h, 32, rmask, gmask, bmask, amask);
	if (!apu) {
		return -1;
	}
	SDL_FillRect(apu, 0, 0);

	/* Poistetaan erityiset l‰pin‰kyvyysasetukset. */
	SDL_SetAlpha(kuva, 0, 0);
	if ((kuva_flags & SDL_SRCALPHA) != 0 && kuva->format->Amask) {
		SDL_SetColorKey(kuva, 0, 0);
	}

	/* OpenGL:n ja SDL:n y-akselit osoittavat eri suuntiin.
	 * Kopioidaan siis kuva pikselirivi kerrallaan ylˆsalaisin. */
	r1.x = r2.x = 0;
	r1.h = r2.h = 1;
	r1.w = r2.w = kuva->w;
	for (r1.y = 0, r2.y = kuva->h - 1; r2.y >= 0; ++r1.y, --r2.y) {
		SDL_BlitSurface(kuva, &r1, apu, &r2);
	}

	/* Koko pinnan alfa-arvo pit‰‰ palauttaa erikseen, jos sellainen on. */
	if ((kuva_flags & SDL_SRCALPHA) && !kuva->format->Amask && kuva_alpha != 0xff) {
		for (r1.y = 0; r1.y < apu->h; ++r1.y) {
			ptr = (Uint32*)((Uint8*) apu->pixels + r1.y * apu->pitch);
			for (r1.x = 0; r1.x < apu->w; ++r1.x) {
				if ((ptr[r1.x] & amask) != 0) {
					ptr[r1.x] &= (kuva_alpha << ashift) | ~amask;
				}
			}
		}
	}

	/* L‰hetet‰‰n kuva OpenGL:lle, tuhotaan apupinta ja palautetaan asetukset. */
	glTexImage2D(GL_TEXTURE_2D, 0, 4, apu->w, apu->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, apu->pixels);
	SDL_FreeSurface(apu);
	SDL_SetAlpha(kuva, kuva_flags, kuva_alpha);
	SDL_SetColorKey(kuva, kuva_flags, kuva_colorkey);
	return 0;
}



int hoida_viestit(void)
{
	// SDL hoitaa viestit - aivan kuten yleens‰kin
	SDL_Event event;

	// Tutkitaan kaikki tapahtumat jonosta
	while (SDL_PollEvent(&event)) {
		// Rastin painamisesta aiheutuu yleens‰ t‰llainen
		if (event.type == SDL_QUIT) {
			return -1;
		}
		// Esc sulkee myˆs
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
			return -1;
		}
	}

	// Nolla on onnistuminen
	return 0;
}

void alusta(void)
{
	// Ensin pit‰‰ hoitaa SDL aluilleen
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("Virhe: SDL_Init: %s\n", SDL_GetError());
		exit(virhe = 1);
	}

	// ‰‰net

	/* Initialization */
	if(SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
		printf("SDL Audio subsystem could not be started");
	}

	//if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1){
	//printf("SDL_Mixer could not be started");
	//}


	// Sitten asetetaan OpenGL:lle parametrit
	// V‰reille tietty bittim‰‰r‰
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, VARIN_BITIT);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, VARIN_BITIT);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, VARIN_BITIT);
	// Syvyyspuskurille jotain...
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, SYVYYS);
	// Kaksoispuskurointi k‰yttˆˆn
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// K‰ynnistet‰‰n video. Pintaa ei tarvitse ottaa muistiin.
	if (SDL_SetVideoMode(LEVEYS, KORKEUS, 0, SDL_OPENGL | IKKUNALIPPU) == NULL) {
		printf("Virhe: SDL_SetVideoMode: %s\n", SDL_GetError());
		exit(virhe = 2);
	}
	// Moi

	SDL_WM_SetCaption("Sartre temmeltaa lehdossa nalkaisena", 0);

	// Katsellaan, millaiset asetukset saatiin aikaan
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

	// T‰ss‰ voisi s‰‰t‰‰ OpenGL:n asetuksia kuntoon aivan normaalisti,
	// siis glEnablella ja muilla OpenGL:n s‰‰tˆfunktioilla.

	// OPENGL tulee t‰ss‰

	glViewport(0, 0, LEVEYS, KORKEUS);					// Reset The Current Viewport
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();							// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	// gluPerspective(45.0f,(GLfloat)LEVEYS/(GLfloat)KORKEUS,0.1f,100.0f);
	glOrtho(0.0f, LEVEYS, KORKEUS, 0.0f, -100.0f, 100.0f);

	glMatrixMode(GL_MODELVIEW);						// Select The Modelview Matrix
	glLoadIdentity();							// Reset The Modelview Matrix

	glShadeModel(GL_SMOOTH);						// Enables Smooth Shading
	glClearDepth(1.0f);							// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);						// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);		            // The Type Of Depth Test To Do

	// Alustukset alfa-kanavoinnille

	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER,0.1f);


	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);			// Really Nice Perspective Calculations
	// Asetetaan taustav‰riksi tyls‰ musta
	glClearColor(0.0, 0.0, 0.0, 0.0);

	// ƒ‰ni

	//music = Mix_LoadMUS("Musiikki/Myytti.mp3");
	//if(!music){
	//printf("Could not load the music file from disk");
	//}
	//Mix_PlayMusic(music, -1);


	// Sartre-hahmon ja taustan alustukset

	Sartre.MuutaX(400.0);
	Sartre.MuutaY(650.0);
	Sartre.hahmo = 0;
	Sartre.hyppy = 0;
	Sartre.MuutaVX(16);

	// aika

	lastTick = SDL_GetTicks();

	// Valmistellaan tekstuurit.

	SDL_RWops *rwop;
	rwop=SDL_RWFromFile("data/images/Sartre1a.png", "rb");
	Sartre.image[0]=IMG_LoadPNG_RW(rwop);
	if(!Sartre.image[0]) {
		printf("IMG_LoadPNG_RW: %s\n", IMG_GetError());
		// handle error
	}

	rwop=SDL_RWFromFile("data/images/Sartre2a.png", "rb");
	Sartre.image[1]=IMG_LoadPNG_RW(rwop);
	if(!Sartre.image[1]) {
		printf("IMG_LoadPNG_RW: %s\n", IMG_GetError());
		// handle error
	}

	rwop=SDL_RWFromFile("data/images/Sartre3a.png", "rb");
	Sartre.image[2]=IMG_LoadPNG_RW(rwop);
	if(!Sartre.image[2]) {
		printf("IMG_LoadPNG_RW: %s\n", IMG_GetError());
		// handle error
	}

	rwop=SDL_RWFromFile("data/images/lehto2.jpg", "rb");
	tausta[0].image=IMG_LoadJPG_RW(rwop);
	if(!tausta[0].image) {
		printf("IMG_LoadJPG_RW: %s\n", IMG_GetError());
		// handle error
	}
	glGenTextures(4, &tex[0]);
	// Sartre

	glBindTexture(GL_TEXTURE_2D, tex[0]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	MySDL_glTexImage2D(Sartre.image[0]);

	glBindTexture(GL_TEXTURE_2D, tex[1]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	MySDL_glTexImage2D(Sartre.image[1]);

	glBindTexture(GL_TEXTURE_2D, tex[2]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	MySDL_glTexImage2D(Sartre.image[2]);

	// tausta

	glBindTexture(GL_TEXTURE_2D, tex[3]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	MySDL_glTexImage2D(tausta[0].image);




	// Rullataan viel‰ viestit l‰pi ja nollataan aikalaskuri
	if (hoida_viestit() != 0) {
		exit(virhe = 0);
	}

}


void metsa_piirra(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
	glLoadIdentity();					// Reset The Current Modelview Matrix

	// Sartre

	glTranslatef(Sartre.LueX(),Sartre.LueY(),0.0f);

	// Alfa-kanavalle blendaus

	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);

	// Oikean tekstuurin valinta, hahmo-muuttujan arvo vaihtelee ajan mukaan

	if(Sartre.hahmo == 0) glBindTexture(GL_TEXTURE_2D, tex[0]);
	else if(Sartre.hahmo == 1) glBindTexture(GL_TEXTURE_2D, tex[1]);
	else glBindTexture(GL_TEXTURE_2D, tex[2]);

	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-100.0f, -100.0f,  1.0f);	// Bottom Left Of The Texture and Quad
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f( 100.0f, -100.0f,  1.0f);	// Bottom Right Of The Texture and Quad
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f( 100.0f,  100.0f,  1.0f);	// Top Right Of The Texture and Quad
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-100.0f,  100.0f,  1.0f);	// Top Left Of The Texture and Quad
	glEnd();

	// Alfa-blendaus pois

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	// Tausta


	glLoadIdentity();
	glTranslatef(tausta[0].LueX(),tausta[0].LueY(),0.0f);				// Left 1.5 Then Into Screen Six Units
	glBindTexture(GL_TEXTURE_2D, tex[3]);
	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(0.0f, KORKEUS,  0.0f);	// Bottom Left Of The Texture and Quad
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f( LEVEYS, KORKEUS,  0.0f);	// Bottom Right Of The Texture and Quad
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f( LEVEYS, 0.0f,  0.0f);	// Top Right Of The Texture and Quad
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f,  0.0f);	// Top Left Of The Texture and Quad
	glEnd();



	// Piirrokset esille
	if (kaksoispuskurointi) {
		SDL_GL_SwapBuffers();
	}
}

void metsa_toiminta(void)
{

	if ( keystate[SDLK_RIGHT] ) {
		// Jos ei olla viel‰ vasemmassa reunassa
		if(tausta[0].LueX() > -3072) {
			// Ruudusta nelj‰sosa n‰kˆtilaa skrollatessa
			if(Sartre.LueX() < ((LEVEYS/4)*3)) {
				Sartre.MuutaX(Sartre.LueX()+Sartre.LueVX());
			} else tausta[0].MuutaX(tausta[0].LueX()-Sartre.LueVX());
		} else {
			// Reunassa puolet hahmon leveydest‰ pois liikkumatilasta estetiikan vuoksi
			if ( Sartre.LueX() < LEVEYS - 100 ) {
				Sartre.MuutaX(Sartre.LueX()+Sartre.LueVX());
			}
		}
	}

	if ( keystate[SDLK_LEFT] ) {
		// Jos ei olla viel‰ oikeassa reunassa
		if (tausta[0].LueX() < 4096 ) {
			// Ruudusta nelj‰sosa n‰kˆtilaa skrollatessa
			if(Sartre.LueX() > (LEVEYS/4)) {
				Sartre.MuutaX(Sartre.LueX()-Sartre.LueVX());
			} else tausta[0].MuutaX(tausta[0].LueX()+Sartre.LueVX());
		} else {
			// Reunassa puolet hahmon leveydest‰ pois liikkumatilasta estetiikan vuoksi
			if ( Sartre.LueX() > 0 + 100  ) {
				Sartre.MuutaX(Sartre.LueX()-Sartre.LueVX());
			}
		}
	}

	// Jos ylˆsp‰in-nappi painettu, aloita hyppy. S‰ilytet‰‰n alkuper‰inen y-koordinaatti hypyn ajan, jotta osataan pys‰ytt‰‰ oikessa kohdassa.
	if ( Sartre.hyppy == 0 && keystate[SDLK_UP] ) {
		Sartre.hyppy = 1;
		Sartre.MuutaVY(-50);
		Sartre.MuutaAlkupY(Sartre.LueY());
		deb_alku = SDL_GetTicks();
		deb_mittaus = true;
	}

	// Hypyn ollessa k‰ynniss‰
	if(Sartre.hyppy == 1) {
		Sartre.MuutaY(Sartre.LueY() + Sartre.LueVY());
		Sartre.MuutaVY(Sartre.LueVY() + gravKiiht);
		if(Sartre.LueY() > Sartre.LueAlkupY()) {
			Sartre.hyppy = 0;
			Sartre.MuutaY(Sartre.LueAlkupY());
			deb_loppu = SDL_GetTicks();
		}
	}

	// Tekstuurien vaihdot ajan mukaan
	//if ( timer2 >= 0 && timer2 < 200) Sartre.hahmo = 0;
	//else if ( timer2 >= 200 && timer2 < 200*2) Sartre.hahmo = 1;
	//else if ( timer2 >= 200*2 && timer2 < 200*3) Sartre.hahmo = 2;
	//else timer2 = 0;

}


void lopetusfunktio(void)
{
	// Nerokasta k‰yttˆ‰ t‰llekin rakenteelle...
	switch (virhe) {
	// case 0 - normaali lopetus. Kaikki sulkemistoimenpiteet on teht‰v‰.
	case 0:

	// case 2 - ohjelma loppui kesken ja virhekoodi on 2.
	case 2:
		SDL_Quit();

	// case 1 - SDL:‰‰ ei saatu alustettua. Alustamattomia osia ei tietenk‰‰n tarvitse sulkeakaan.
	case 1:
		break;

	// Muu virhekoodi - t‰m‰h‰n voi olla vaikka bugi, on unohtunut lis‰t‰ t‰nne listaan jokin virhekoodi.
	default:
		printf("Ja virhekoodikin pieleen... %d xD\n", virhe);
	}

	// Muistia vapaaksi
	for(int a = 0; a++; a < 3) {
		SDL_FreeSurface(Sartre.image[a]);
	}
	for(int a = 0; a++; a < 4) {
		glDeleteTextures(1, &tex[a]);
	}
	SDL_FreeSurface(tausta[0].image);
	//Mix_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void musicDone()
{
	//Mix_HaltMusic();
	//Mix_FreeMusic(music);
	//music = NULL;
}

void debuggausfunktio()
{
	if(deb_mittaus == true && Sartre.hyppy == 0) {
		deb_mittaus = false;

		myfile.open ("debug.txt");
		myfile << "Hyppyyn kuluu: " << deb_loppu - deb_alku << std::endl;
		myfile.close();
	}
}


// SDL kaipaa toisinaan mainille parametreja, vaikkei niit‰ k‰ytett‰isi
int main(int argc, char **argv)
{
	// Lopetusfunktio suoritetaan automaattisesti lopuksi.
	atexit(lopetusfunktio);
	// Alustusfunktio
	alusta();




	while (hoida_viestit() == 0) {
		// (graphics loop)
		tick = SDL_GetTicks();

		if (tick <= lastTick) {
			// 5 milliseconds should be enough for anyone, go back at the loop beginning to retest the clause
			SDL_Delay(5);
			continue;
		}

		// ala piirra taikka laske fysiikoita jos ei ole tickkiakaan kulunut
		// this is just for if you comment out the SDL_Delay part..
		if( lastTick >= tick )
			continue;

		// Toiminta mets‰ss‰ tahdistimella
		while(lastTick < tick) {
			// (physics loop)
			metsa_toiminta();
			//debuggausfunktio();
			lastTick += ( 1000 / fps );
		}
		// after the physics, cause we want the latest update to be shown on screen.
		metsa_piirra();
	}
	// Mix_HookMusicFinished(musicDone);


	return virhe = 0;
}


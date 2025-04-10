#include "utils.h"

#include <vector>
#include <cstring>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <GL/glew.h>

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

void create_textures(Textures &textures, std::string dataPath)
{
	// Sartre
	SDL_Surface *forestSartreImage[2];
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

	// Pages
	SDL_Surface *forestPageImage[1];
	forestPageImage[0] = IMG_Load((dataPath + "images/objects/page.png").c_str());
	if (!forestPageImage[0]) {
		printf("Error loading image: %s\n", SDL_GetError());
		exit(1);
	}
	glGenTextures(1, textures.forestPage);
	for (int i = 0; i < 1; i++) {
		SDL_Surface* formattedSurface = format_sdl_surface(forestPageImage[i]);
		if (!formattedSurface) {
			exit(1);
		}
		glBindTexture(GL_TEXTURE_2D, textures.forestPage[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formattedSurface->w, formattedSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, formattedSurface->pixels);
		SDL_FreeSurface(formattedSurface);
		SDL_FreeSurface(forestPageImage[i]);
	}

	// Background
	SDL_Surface *forestTaustaImage;
	forestTaustaImage = IMG_Load((dataPath + "images/lehto.png").c_str());
	if (!forestTaustaImage) {
		printf("Error loading image: %s\n", SDL_GetError());
		exit(1);
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
}

void create_surfaces(Surfaces &surfaces, std::string dataPath)
{
	// Load collision map
	surfaces.forestCollisionMap = format_sdl_surface(IMG_Load((dataPath + "images/lehto_platforms.png").c_str()));
	if (!surfaces.forestCollisionMap) {
		printf("Error loading collision map: %s\n", SDL_GetError());
		exit(1);
	}
}

void free_textures(Textures &textures)
{
	for (int a = 0; a < 2; a++) {
		glDeleteTextures(1, &textures.forestSartre[a]);
	}
	for (int a = 0; a < 2; a++) {
		glDeleteTextures(1, &textures.forestPage[a]);
	}
	glDeleteTextures(1, &textures.forestTausta[0]);
}

void free_surfaces(Surfaces &surfaces)
{
	SDL_FreeSurface(surfaces.forestCollisionMap);
}

bool isPixelBlack(SDL_Surface* surface, int x, int y)
{
	// Determine if a pixel is considered “black” by checking that all RGB components are below this threshold.
	Uint8 threshold = 50;
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

void renderText(TTF_Font* font, const std::string& text, SDL_Color color, GLuint shader, GLuint VAO, GLuint VBO, float x, float y)
{
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

	// Blended font needs this
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

	glDisable(GL_BLEND);
}





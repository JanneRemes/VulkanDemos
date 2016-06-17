#ifndef VKDEMOS_LOADIMAGEFROMFILE_H
#define VKDEMOS_LOADIMAGEFROMFILE_H

#include <SDL2/SDL_image.h>
#include <string>
#include <cstring>

/**
 * Load an image from a file.
 * @param path the path of the image file
 * @returns a SDL_Surface containing the loaded image, or nullptr if an error occurred.
 */
SDL_Surface* loadImageFromFile(const std::string & path)
{
	SDL_PixelFormat pixformat;
	std::memset(&pixformat, 0, sizeof(SDL_PixelFormat));

	pixformat.BitsPerPixel = 32;
	pixformat.BytesPerPixel = 4;
	pixformat.palette = nullptr;
	pixformat.format = SDL_PIXELFORMAT_RGBA8888;
	pixformat.Rmask = 0x000000FF;
	pixformat.Gmask = 0x0000FF00;
	pixformat.Bmask = 0x00FF0000;
	pixformat.Amask = 0xFF000000;

	// Load image
	SDL_Surface* loadedimage = IMG_Load(path.c_str());

	if(loadedimage != nullptr)
	{
		// Puts the file in a SDL acceptable format
		return SDL_ConvertSurface(loadedimage, &pixformat, 0);
	}

	return nullptr;
}

#endif

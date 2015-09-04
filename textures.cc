// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "textures.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

struct TexturesImpl {
};


Textures::Textures(): self(new TexturesImpl) {}

Textures::~Textures() {}

/** If the surface hasn't been built, or if the set of sprites has
 * changed, build the surface, and return it. 
 */
SDL_Surface* Textures::GetSurface() {
#if 0
  return IMG_Load("assets/red-blob.png");
#else
  unsigned char ttf_buffer[1<<20];
  static unsigned char temp_bitmap[256*256];

  stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs

  fread(ttf_buffer, 1, 1<<20, fopen("assets/share-tech-mono.ttf", "rb"));
  stbtt_BakeFontBitmap(ttf_buffer,0, 36.0, temp_bitmap,256,256, 32,96, cdata);

  // HACK: figure out the right interface
  auto surface = SDL_CreateRGBSurfaceFrom(temp_bitmap,
                                          256, 256,
                                          8,
                                          256,
                                          0, 0, 0, 0xff);
  return surface;
#endif
}

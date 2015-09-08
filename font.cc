// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "font.h"
#include "glwrappers.h"
#include "common.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <fstream>
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_rect_pack.h"
#include "stb/stb_truetype.h"
#pragma clang diagnostic pop

// This is the ascii font range (half-open interval) I support:
const int LOW_CHAR = 32; // space
const int HIGH_CHAR = 126+1; // tilde

// How many pixels to leave around each sprite
const int PADDING = 1;

/* For each character in the font, I maintain the region of the
   surface that stores it, as well as the position of the baseline and
   the x-advance width. */
struct FontCharacter {
  // TODO: reuse SDL_Rect here
  int x, y, w, h; // source region of the surface
  int xadvance;   // how much x increases after drawing
  int ybaseline;  // how far down into the region the baseline is
};
  
struct FontImpl {
  SDL_Surface* surface;
  std::vector<unsigned char> rendered_font;
  std::vector<FontCharacter> mapping;
};


// TODO: we don't need rect packing anymore because I can make a very
// wide SDL_Surface
const int SIZE = 512;

Font::Font(const char* filename, float ptsize): self(new FontImpl) {
  // Load the font into memory
  std::vector<char> font_buffer;
  std::ifstream in(filename, std::ifstream::binary);
  in.seekg(0, std::ios_base::end);
  font_buffer.resize(in.tellg());
  in.seekg(0, std::ios_base::beg);
  in.read(font_buffer.data(), font_buffer.size());

  // Render the font into a bitmap
  std::vector<unsigned char> rendered_font_grayscale;
  rendered_font_grayscale.resize(SIZE * SIZE);
  
  int N = HIGH_CHAR - LOW_CHAR;
  stbtt_packedchar chardata[N];
  stbtt_pack_context packer;
  stbtt_PackBegin(&packer, rendered_font_grayscale.data(),
                  SIZE, SIZE, 0, PADDING, nullptr);
  stbtt_PackSetOversampling(&packer, 1, 1);
  int r = stbtt_PackFontRange(&packer,
                              reinterpret_cast<unsigned char*>(font_buffer.data()),
                              0, ptsize, LOW_CHAR, N, chardata);
  if (r == 0) { FAIL("Font unable to fit"); }
  stbtt_PackEnd(&packer);

  // Store the character data as separate sprites
  self->mapping.resize(HIGH_CHAR);
  for (int c = LOW_CHAR; c < HIGH_CHAR; c++) {
    auto& M = self->mapping[c];
    float x = 0.0, y = 0.0;
    stbtt_aligned_quad q;
    stbtt_GetPackedQuad(chardata, SIZE, SIZE, c-LOW_CHAR,
                        &x, &y, &q, 0);
    M.x = q.s0 * SIZE;
    M.y = q.t0 * SIZE;
    M.w = (q.s1 - q.s0) * SIZE;
    M.h = (q.t1 - q.t0) * SIZE;
    M.xadvance = x;
    M.ybaseline = -q.y0;
  }

  // Copy the grayscale bitmap into RGBA
  self->rendered_font.resize(SIZE * SIZE * 4);
  for (int i = 0; i < rendered_font_grayscale.size(); i++) {
    self->rendered_font[i*4    ] = 255;
    self->rendered_font[i*4 + 1] = 255;
    self->rendered_font[i*4 + 2] = 255;
    self->rendered_font[i*4 + 3] = rendered_font_grayscale[i];
  }
  
  self->surface = SDL_CreateRGBSurfaceFrom(self->rendered_font.data(),
                                         SIZE, SIZE,
                                         32, SIZE*4,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                                         0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#else
                                         0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#endif
                                         );
}


Font::~Font() {}


void Font::Draw(SDL_Surface* surface, int x, int y, const char* text) const {
  for (const char* s = text; *s != '\0'; s++) {
    FontCharacter loc = self->mapping[*s];
    SDL_Rect src, dst;
    src.x = loc.x;
    src.y = loc.y;
    src.w = loc.w;
    src.h = loc.h;
    dst.x = x;
    dst.y = y - loc.ybaseline;
    x += loc.xadvance;
    if (SDL_BlitSurface(self->surface, &src, surface, &dst) < 0) {
      FAIL("Blit character");
    }
  }

}

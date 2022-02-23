// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "font.h"
#include "common.h"

#include <SDL.h>

#include <fstream>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

// This is the ascii font range (half-open interval) I support:
const int LOW_CHAR = 32; // space
const int HIGH_CHAR = 126+1; // tilde

/* For each character in the font, I maintain the region of the
   surface that stores it, as well as the position of the baseline and
   the x-advance width. */
struct FontCharacter {
  SDL_Rect region;
  int xadvance;   // how much x increases after drawing
  int ybaseline;  // how far down into the region the baseline is
};
  
struct FontImpl {
  SDL_Surface* surface;
  int baseline;
  int height;
  std::vector<unsigned char> rendered_font;
  std::vector<FontCharacter> mapping;
};


Font::Font(const char* filename, float ptsize, float xadvance_adjust): self(new FontImpl) {
  // Load the font into memory
  std::vector<char> font_buffer;
  std::ifstream in(filename, std::ifstream::binary);
  in.seekg(0, std::ios_base::end);
  font_buffer.resize(in.tellg());
  in.seekg(0, std::ios_base::beg);
  in.read(font_buffer.data(), font_buffer.size());

  // Use font metrics to determine how big I need to make the bitmap
  int width = 0, height = int(ceil(ptsize));
  self->height = int(ceil(height));
  stbtt_fontinfo font;
  stbtt_InitFont(&font, reinterpret_cast<unsigned char*>(font_buffer.data()), 0);
  for (char c = LOW_CHAR; c < HIGH_CHAR; c++) {
    int ix0, iy0, ix1, iy1;
    stbtt_GetCodepointBitmapBox(&font, c, 1, 1, &ix0, &iy0, &ix1, &iy1);
    // NOTE(amitp): I'm not 100% convinced this is always enough space
    width += 1 + int(ceil(ix1 * ptsize / 1000.0) - floor(ix0 * ptsize / 1000.0));
  }

  // HACK(amitp): Some fonts and some sizes seem to need a little more
  // space here, for reasons I don't understand.
  height += 5;
  
  // Render the font into the bitmap
  std::vector<unsigned char> rendered_font_grayscale;
  rendered_font_grayscale.resize(width * height);
  
  constexpr int N = HIGH_CHAR - LOW_CHAR;
  stbtt_bakedchar chardata[N];
  int r = stbtt_BakeFontBitmap(reinterpret_cast<unsigned char*>(font_buffer.data()),
                               0, ptsize, rendered_font_grayscale.data(), width, height,
                               LOW_CHAR, N, chardata);
  if (r < 0) { FAIL("BakeFontBitmap not enough space"); }
  self->mapping.resize(HIGH_CHAR);
  self->baseline = 0;
  for (int c = LOW_CHAR; c < HIGH_CHAR; c++) {
    auto& M = self->mapping[c];
    float x = 0.0, y = 0.0;
    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(chardata, width, height, c-LOW_CHAR, &x, &y, &q, true);
    M.region.x = int(floor(q.s0 * width));
    M.region.y = int(floor(q.t0 * height));
    M.region.w = int(ceil((q.s1 - q.s0) * width));
    M.region.h = int(ceil((q.t1 - q.t0) * height));
    M.xadvance = int(round(x + xadvance_adjust));
    M.ybaseline = int(-q.y0);
    self->baseline = std::max(self->baseline, M.ybaseline);
  }
  
  // Copy the grayscale bitmap into RGBA
  self->rendered_font.resize(width * height * 4);
  for (unsigned i = 0; i < rendered_font_grayscale.size(); i++) {
    self->rendered_font[i*4    ] = 255;
    self->rendered_font[i*4 + 1] = 255;
    self->rendered_font[i*4 + 2] = 255;
    self->rendered_font[i*4 + 3] = rendered_font_grayscale[i];
  }
  
  self->surface = SDL_CreateRGBSurfaceFrom(self->rendered_font.data(),
                                           width, height,
                                           32, width*4,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                                           0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#else
                                           0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#endif
                                           );
}


Font::~Font() {}


void Font::Draw(SDL_Surface* surface, int x, int y, const char* text) const {
  SDL_Rect dest;
  dest.x = x;
  for (const char* s = text; *s != '\0'; s++) {
    FontCharacter loc = self->mapping[*s];
    dest.y = y - loc.ybaseline;
    SDL_BlitSurface(self->surface, &loc.region, surface, &dest);
    dest.x += loc.xadvance;
    // TODO: kerning with stbtt_GetCodepointKernAdvance(&font, s[0], s[1])
  }
}


int Font::Height() const {
  return self->height;
}

int Font::Baseline() const {
  return self->baseline;
}

int Font::Width(const char* text) const {
  int width = 0;
  for (const char* s = text; *s != '\0'; s++) {
    width += self->mapping[*s].xadvance;
  }
  return width;
}

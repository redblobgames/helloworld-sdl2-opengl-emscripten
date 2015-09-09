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
  SDL_Rect region;
  int xadvance;   // how much x increases after drawing
  int ybaseline;  // how far down into the region the baseline is
};
  
struct FontImpl {
  SDL_Surface* surface;
  std::vector<unsigned char> rendered_font;
  std::vector<FontCharacter> mapping;
};


Font::Font(const char* filename, float ptsize): self(new FontImpl) {
  // Load the font into memory
  std::vector<char> font_buffer;
  std::ifstream in(filename, std::ifstream::binary);
  in.seekg(0, std::ios_base::end);
  font_buffer.resize(in.tellg());
  in.seekg(0, std::ios_base::beg);
  in.read(font_buffer.data(), font_buffer.size());

  // Use font metrics to determine how big I need to make the bitmap
  int width = 0, height = ceil(ptsize);
  stbtt_fontinfo font;
  stbtt_InitFont(&font, reinterpret_cast<unsigned char*>(font_buffer.data()), 0);
  for (char c = LOW_CHAR; c < HIGH_CHAR; c++) {
    int ix0, iy0, ix1, iy1;
    stbtt_GetCodepointBitmapBox(&font, c, 1, 1, &ix0, &iy0, &ix1, &iy1);
    width += ceil((ix1 - ix0) * ptsize / 1000.0);
  }
  
  // Render the font into the bitmap
  // TODO: stbtt_GetCodepointBitmap might be a simpler way to get
  // the rendered glyph. I don't need the rectangle packer, and I
  // already have the font metrics, and I'm converting from one-byte
  // to four-byte format afterwards.
  std::vector<unsigned char> rendered_font_grayscale;
  rendered_font_grayscale.resize(width * height);
  
  int N = HIGH_CHAR - LOW_CHAR;
  stbtt_packedchar chardata[N];
  stbtt_pack_context packer;
  stbtt_PackBegin(&packer, rendered_font_grayscale.data(),
                  width, height, 0, PADDING, nullptr);
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
    stbtt_GetPackedQuad(chardata, width, height, c-LOW_CHAR,
                        &x, &y, &q, 0);
    M.region.x = q.s0 * width;
    M.region.y = q.t0 * height;
    M.region.w = (q.s1 - q.s0) * width;
    M.region.h = (q.t1 - q.t0) * height;
    M.xadvance = x;
    M.ybaseline = -q.y0;
  }

  // Copy the grayscale bitmap into RGBA
  self->rendered_font.resize(width * height * 4);
  for (int i = 0; i < rendered_font_grayscale.size(); i++) {
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
  for (const char* s = text; *s != '\0'; s++) {
    FontCharacter loc = self->mapping[*s];
    SDL_Rect dest;
    dest.x = x;
    dest.y = y - loc.ybaseline;
    x += loc.xadvance;
      
    if (SDL_BlitSurface(self->surface, &loc.region, surface, &dest) < 0) {
      FAIL("Blit character");
    }
  }
}

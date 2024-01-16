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

// How many pixels to leave around each sprite
const int PADDING = 4;

struct FontImpl {
  stbtt_fontinfo font;
  SDL_Surface *surface;
  int height;
  int atlas_width;
  int atlas_height;
  std::vector<unsigned char> rendered_font;
  std::vector<stbtt_packedchar> chardata; // metrics
};


Font::Font(const char* filename, float pixelsize_, float xadvance_adjust)
    : pixelsize(pixelsize_), self(new FontImpl) {
  // Load the font into memory
  std::vector<char> font_buffer;
  std::ifstream in(filename, std::ifstream::binary);
  in.seekg(0, std::ios_base::end);
  font_buffer.resize(in.tellg());
  in.seekg(0, std::ios_base::beg);
  in.read(font_buffer.data(), font_buffer.size());

  // Use font metrics to determine how big I need to make the bitmap

  self->atlas_width = 0;
  self->height = int(ceil(pixelsize));
  self->atlas_height = self->height + 2 * PADDING;
  stbtt_InitFont(&self->font, reinterpret_cast<unsigned char*>(font_buffer.data()), 0);
  const float scale = stbtt_ScaleForPixelHeight(&self->font, pixelsize);
  for (char c = LOW_CHAR; c < HIGH_CHAR; c++) {
    int ix0, iy0, ix1, iy1;
    stbtt_GetCodepointBitmapBox(&self->font, c, scale, scale,
                                &ix0, &iy0, &ix1, &iy1);
    self->atlas_width += ix1 - ix0 + 2 * PADDING;
  }

  // Render the font into the bitmap
  std::vector<unsigned char> rendered_font_grayscale;
  rendered_font_grayscale.resize(self->atlas_width * self->atlas_height);

  const int N = HIGH_CHAR - LOW_CHAR;
  self->chardata.resize(HIGH_CHAR - LOW_CHAR);
  stbtt_pack_context context;
  stbtt_PackBegin(&context, rendered_font_grayscale.data(),
                  self->atlas_width, self->atlas_height, 0, PADDING, nullptr);
  int r = stbtt_PackFontRange(&context,
                              reinterpret_cast<unsigned char*>(font_buffer.data()),
                              0, pixelsize, LOW_CHAR, N, self->chardata.data());
  if (r == 0) { FAIL("Font unable to fit on texture"); }
  stbtt_PackEnd(&context);

  // Copy the grayscale bitmap into RGBA, for SDL
  self->rendered_font.resize(self->atlas_width * self->atlas_height * 4);
  for (unsigned i = 0; i < rendered_font_grayscale.size(); i++) {
    self->rendered_font[i*4    ] = 255;
    self->rendered_font[i*4 + 1] = 255;
    self->rendered_font[i*4 + 2] = 255;
    self->rendered_font[i*4 + 3] = rendered_font_grayscale[i];
  }

  self->surface = SDL_CreateRGBSurfaceFrom(self->rendered_font.data(),
                                           self->atlas_width, self->atlas_height,
                                           32, self->atlas_width*4,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                                           0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#else
                                           0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#endif
  );
}


Font::~Font() {}


void Font::Draw(SDL_Surface *surface, int x, int y, const char* text) const {
  const float scale = stbtt_ScaleForPixelHeight(&self->font, pixelsize);
  float offset_x = x, offset_y = y;
  stbtt_aligned_quad quad;
  for (const char* s = text; *s != '\0'; s++) {
    int char_index = *s - LOW_CHAR;
    if (char_index < 0 || char_index >= int(self->chardata.size())) {
      FAIL("Tried to render character out of range");
    }
    stbtt_GetPackedQuad(self->chardata.data(), self->atlas_width, self->atlas_height,
                        char_index, &offset_x, &offset_y, &quad,
                        0);

    SDL_Rect src;
    src.x = int(round(quad.s0 * self->atlas_width));
    src.y = int(round(quad.t0 * self->atlas_height));
    src.w = int(round((quad.s1 - quad.s0) * self->atlas_width));
    src.h = int(round((quad.t1 - quad.t0) * self->atlas_height));

    SDL_Rect dest;
    dest.x = int(floor(quad.x0));
    dest.y = int(floor(quad.y0));
    dest.w = int(ceil(quad.x1 - quad.x0));
    dest.h = int(ceil(quad.y1 - quad.y0));

    if (SDL_BlitSurface(self->surface, &src, surface, &dest) < 0) {
      FAIL("Blit character");
    }

    if (s[1]) {
      offset_x += scale * stbtt_GetCodepointKernAdvance(&self->font, s[0], s[1]);
    }
  }
}

int Font::Width(const char* text) const {
  const float scale = stbtt_ScaleForPixelHeight(&self->font, pixelsize);
  float offset_x = 0.0, offset_y = 0.0;
  stbtt_aligned_quad quad;
  for (const char* s = text; *s != '\0'; s++) {
    int char_index = *s - LOW_CHAR;
    if (char_index < 0 || char_index >= int(self->chardata.size())) {
      FAIL("Tried to render character out of range");
    }
    stbtt_GetPackedQuad(self->chardata.data(), self->atlas_width, self->atlas_height,
                        char_index, &offset_x, &offset_y, &quad,
                        0);
    if (s[1]) {
      offset_x += scale * stbtt_GetCodepointKernAdvance(&self->font, s[0], s[1]);
    }
  }
  return int(ceil(offset_x));
}

int Font::Height() const {
  return self->height;
}

int Font::Baseline() const {
  const float scale = stbtt_ScaleForPixelHeight(&self->font, pixelsize);
  int x0, y0, x1, y1;
  stbtt_GetFontBoundingBox(&self->font, &x0, &y0, &x1, &y1);
  return int(pixelsize + ceil(scale * y0));
}

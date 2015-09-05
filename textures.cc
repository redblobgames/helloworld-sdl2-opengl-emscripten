// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "textures.h"
#include "common.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <fstream>
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"
#include "stb/stb_truetype.h"
#pragma clang diagnostic pop

// This is the ascii font range (half-open interval) I support:
const int LOW_CHAR = 32; // space
const int HIGH_CHAR = 126+1; // tilde

// How many pixels to leave around each sprite
const int PADDING = 1;

/* For each sprite id, I want to keep its original surface
   and its current assignment in the texture atlas: x,y,w,h
   For now, fonts always go into their own texture and aren't
   shared with other images -- I don't yet know how to mix them,
   and they're 8-bit instead of 32-bit anyway. Their x,y,w,h
   table will be populated differently. */

struct TexturesImpl {
  int size;
  SDL_Surface* atlas;
  std::vector<unsigned char> rendered_font;
  std::vector<SDL_Surface*> sources;
  std::vector<SpriteLocation> mapping;
};


Textures::Textures(): self(new TexturesImpl) {
  // TODO: figure out sizing later
  self->size = 256;
  self->atlas = nullptr;
}

Textures::~Textures() {}


const SpriteLocation& Textures::GetLocation(int id) const {
  return self->mapping.at(id);
}


int Textures::LoadImage(const char* filename) {
  // The previous atlas is no longer valid
  if (self->atlas) {
    if (!self->rendered_font.empty()) { FAIL("LoadImage after LoadFont"); }
    SDL_FreeSurface(self->atlas);
    self->atlas = nullptr;
  }
  
  int id = self->mapping.size();
  self->sources.push_back(IMG_Load(filename));
  if (self->sources.back() == nullptr) { FAIL("Unable to load image"); }
  
  self->mapping.emplace_back();
  SpriteLocation& loc = self->mapping.back();
  loc.x0 = -0.5;
  loc.x1 = +0.5;
  loc.y0 = -0.5;
  loc.y1 = +0.5;
  // s0,t0,s1,t1 will be filled in during the packing phase
  
  return id;
}


void Textures::LoadFont(const char* filename, float ptsize) {
  // Load the font into memory
  std::vector<char> font_buffer;
  std::ifstream in(filename, std::ifstream::binary);
  in.seekg(0, std::ios_base::end);
  font_buffer.resize(in.tellg());
  in.seekg(0, std::ios_base::beg);
  in.read(font_buffer.data(), font_buffer.size());

  // Render the font into a bitmap
  self->rendered_font.resize(self->size * self->size);
  int N = HIGH_CHAR - LOW_CHAR;
  stbtt_packedchar chardata[N];
  stbtt_pack_context packer;
  stbtt_PackBegin(&packer, self->rendered_font.data(),
                  self->size, self->size, 0, PADDING, nullptr);
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
    stbtt_GetPackedQuad(chardata, self->size, self->size, c-LOW_CHAR,
                        &x, &y, &q, 0);
    M.x0 = q.x0 / ptsize;
    M.y0 = q.y0 / ptsize;
    M.x1 = q.x1 / ptsize;
    M.y1 = q.y1 / ptsize;
    M.s0 = q.s0;
    M.t0 = q.t0;
    M.s1 = q.s1;
    M.t1 = q.t1;
    M.xadvance = x / ptsize;
  }
  
  self->atlas = SDL_CreateRGBSurfaceFrom(self->rendered_font.data(),
                                         self->size, self->size,
                                         8,
                                         self->size,
                                         0, 0, 0, 0xff);
}


#include <iostream>

/** If the surface hasn't been built, or if the set of sprites has
 * changed, build the surface, and return it. 
 */
SDL_Surface* Textures::GetSurface() {
  if (self->atlas == nullptr) {
    self->atlas = SDL_CreateRGBSurface
      (0, self->size, self->size, 32,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
       0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#else
       0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#endif
       );
    if (self->atlas == nullptr) { FAIL("SDL_CreateRGBSurface"); }

    std::vector<stbrp_rect> rects;
    std::vector<stbrp_node> working_space;
    stbrp_context context;

    rects.resize(self->sources.size());
    for (int i = 0; i < self->sources.size(); i++) {
      rects[i].id = i;
      rects[i].w = 2*PADDING + self->sources[i]->w;
      rects[i].h = 2*PADDING + self->sources[i]->h;
    }
      
    working_space.resize(self->size);
    stbrp_init_target(&context, self->size, self->size,
                      working_space.data(), working_space.size());
    stbrp_pack_rects(&context, rects.data(), rects.size());

    for (int i = 0; i < self->sources.size(); i++) {
      if (!rects[i].was_packed) { FAIL("Could not fit all images"); }
      
      SDL_Rect rect;
      rect.x = PADDING + rects[i].x; rect.y = PADDING + rects[i].y;
      rect.w = self->sources[i]->w; rect.h = self->sources[i]->h;
      if (SDL_BlitSurface(self->sources[i], nullptr, self->atlas, &rect) < 0) {
        FAIL("SDL_BlitSurface");
      }
      
      self->mapping[i].s0 = float(rect.x) / self->size;
      self->mapping[i].s1 = float(rect.x + rect.w) / self->size;
      self->mapping[i].t0 = float(rect.y) / self->size;
      self->mapping[i].t1 = float(rect.y + rect.h) / self->size;
    }
  }

  return self->atlas;
}

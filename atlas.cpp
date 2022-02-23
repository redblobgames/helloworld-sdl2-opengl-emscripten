// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "atlas.h"
#include "glwrappers.h"
#include "common.h"

#include <SDL.h>
#include <SDL_image.h>

#include <fstream>
#include <vector>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>

// How many pixels to leave around each sprite
const int PADDING = 1;

/* For each sprite id, I want to keep its original surface
   and its current location in the texture atlas */

struct AtlasImpl {
  int size;
  SDL_Surface* atlas;
  std::vector<SDL_Surface*> sources;
  std::vector<SpriteLocation> mapping;
};


Atlas::Atlas(): self(new AtlasImpl) {
  // TODO: figure out sizing later
  self->size = 1024;
  self->atlas = nullptr;
}

Atlas::~Atlas() {}


const SpriteLocation& Atlas::GetLocation(int id) const {
  return self->mapping.at(id);
}


int Atlas::AddSurface(SDL_Surface* surface) {
  // The previous atlas is no longer valid
  if (self->atlas) {
    SDL_FreeSurface(self->atlas);
    self->atlas = nullptr;
  }
  
  int id = self->mapping.size();
  self->sources.push_back(surface);
  self->mapping.emplace_back();
  SpriteLocation& loc = self->mapping.back();
  loc.x0 = -0.5;
  loc.x1 = +0.5;
  loc.y0 = -0.5;
  loc.y1 = +0.5;
  // s0,t0,s1,t1 will be filled in during the packing phase

  return id;
}

int Atlas::LoadImage(const char* filename) {
  SDL_Surface* surface = IMG_Load(filename);
  if (surface == nullptr) { FAIL("Unable to load image"); }
  return AddSurface(surface);
}


/** If the surface hasn't been built, or if the set of sprites has
 * changed, build the surface, and return it. 
 */
SDL_Surface* Atlas::GetSurface() {
  if (self->atlas == nullptr) {
    self->atlas = CreateRGBASurface(self->size, self->size);
    std::vector<stbrp_rect> rects;
    std::vector<stbrp_node> working_space;
    stbrp_context context;

    rects.resize(self->sources.size());
    for (unsigned i = 0; i < self->sources.size(); i++) {
      rects[i].id = i;
      rects[i].w = 2*PADDING + self->sources[i]->w;
      rects[i].h = 2*PADDING + self->sources[i]->h;
    }
      
    working_space.resize(self->size);
    stbrp_init_target(&context, self->size, self->size,
                      working_space.data(), working_space.size());
    stbrp_pack_rects(&context, rects.data(), rects.size());

    for (unsigned i = 0; i < self->sources.size(); i++) {
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

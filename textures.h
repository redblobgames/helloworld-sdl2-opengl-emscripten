// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef TEXTURES_H
#define TEXTURES_H

#include <memory>

struct SDL_Surface;
struct TexturesImpl;

class Textures {
public:
  Textures();
  ~Textures();

  SDL_Surface* GetSurface();
private:
  std::unique_ptr<TexturesImpl> self;
};

#endif

// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef RENDER_SPRITES_H
#define RENDER_SPRITES_H

#include "render-layer.h"
#include <memory>

struct SDL_Window;
struct RenderSpritesImpl;


class RenderSprites: public IRenderLayer {
public:
  RenderSprites();
  ~RenderSprites();
  virtual void Render(SDL_Window* window, bool reset);

protected:
  std::unique_ptr<RenderSpritesImpl> self;
};


#endif
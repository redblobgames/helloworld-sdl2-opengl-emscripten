// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef RENDER_LAYER_H
#define RENDER_LAYER_H

#include "common.h"

struct SDL_Window;
union SDL_Event;

struct IRenderLayer: nocopy {
  virtual void Render(SDL_Window* window, bool reset) {}
  virtual void ProcessEvent(SDL_Event* event) {}
  virtual ~IRenderLayer();
};


#endif

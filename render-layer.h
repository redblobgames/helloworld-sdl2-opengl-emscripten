// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef RENDER_LAYER_H
#define RENDER_LAYER_H

struct SDL_Window;

struct IRenderLayer {
  virtual void Render(SDL_Window* window, bool reset) = 0;
};


#endif

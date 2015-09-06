// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef RENDER_SURFACE_H
#define RENDER_SURFACE_H

#include <memory>

struct SDL_Window;
struct SDL_Surface;
struct RenderSurfaceImpl;


class RenderSurface {
public:
  RenderSurface(SDL_Surface* surface);
  ~RenderSurface();
  void Render(SDL_Window* window, bool reset);
  void HandleResize();
  
protected:
  std::unique_ptr<RenderSurfaceImpl> self;
};


#endif

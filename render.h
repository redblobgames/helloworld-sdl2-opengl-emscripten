// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef RENDER_H
#define RENDER_H

#include <memory>

struct SDL_Window;
struct RendererImpl;
struct RenderLayerImpl;

class Renderer {
public:
  Renderer(SDL_Window* window);
  ~Renderer();
  void Render();
  void HandleResize();

  static int FRAME;
private:
  std::unique_ptr<RendererImpl> self;
};

class RenderLayer {
public:
  RenderLayer();
  ~RenderLayer();
  void Render(SDL_Window* window, bool reset);
  void HandleResize();

protected:
  std::unique_ptr<RenderLayerImpl> self;
};


#endif

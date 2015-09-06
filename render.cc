// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "render.h"
#include "textures.h"

#include <SDL2/SDL.h>
#include "glwrappers.h"

#include <vector>


struct RendererImpl {
  SDL_Window* window;
  bool context_initialized;
  GlContext context;
  std::unique_ptr<RenderLayer> layer;
  
  RendererImpl(SDL_Window* window);
  ~RendererImpl();
};

int Renderer::FRAME = 0;
Renderer::Renderer(SDL_Window* window):self(new RendererImpl(window)) {}
Renderer::~Renderer() {}


void Renderer::HandleResize() {
  // This is the easiest way to handle resize but it'd be better not
  // to regenerate the texture.
  SDL_Window* window = self->window;
  self = nullptr;
  self = std::unique_ptr<RendererImpl>(new RendererImpl(window));
}


void Renderer::Render() {
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  self->layer->Render(self->window, !self->context_initialized);
  self->context_initialized = true;
  SDL_GL_SwapWindow(self->window);
  FRAME++;
}


RendererImpl::RendererImpl(SDL_Window* window_)
  :window(window_), context_initialized(false), context(window), layer(new RenderLayer)
{
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

RendererImpl::~RendererImpl() {}

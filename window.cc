// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "window.h"
#include "render-sprites.h"
#include "render-surface.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "glwrappers.h"

#include <vector>


struct WindowImpl {
  SDL_Window* window;
  bool context_initialized;
  GlContext context;
  std::unique_ptr<RenderSprites> layer;
  std::unique_ptr<RenderSurface> layer2;
  
  WindowImpl(SDL_Window* window_);
  ~WindowImpl();
};

int Window::FRAME = 0;
Window::Window(int width, int height)
  :self(new WindowImpl(SDL_CreateWindow("Skeleton",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            width,
                            height,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
                           ))) {}
Window::~Window() {
  // TODO(amitp): can't call this because 'self' has been destroyed
  // SDL_DestroyWindow(self->window);
}


void Window::HandleResize() {
  // This is the easiest way to handle resize but it'd be better not
  // to regenerate the texture.
  SDL_Window* window = self->window;
  self = nullptr;
  self = std::unique_ptr<WindowImpl>(new WindowImpl(window));
}


void Window::Render() {
  glClear(GL_COLOR_BUFFER_BIT);
  self->layer->Render(self->window, !self->context_initialized);
  self->layer2->Render(self->window, !self->context_initialized);
  self->context_initialized = true;
  SDL_GL_SwapWindow(self->window);
  FRAME++;
}


WindowImpl::WindowImpl(SDL_Window* window_)
  :window(window_), context_initialized(false),
   context(window),
   layer(new RenderSprites),
   layer2(new RenderSurface(IMG_Load("assets/red-blob.png")))
{
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

WindowImpl::~WindowImpl() {}

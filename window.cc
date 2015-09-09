// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "window.h"

#include <SDL2/SDL.h>
#include "glwrappers.h"

#include <vector>


struct WindowImpl {
  SDL_Window* window;
  bool context_initialized;
  GlContext context;
  std::vector<IRenderLayer*> layers;
  
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
                           )))
{
  HandleResize();
}

Window::~Window() {}


void Window::AddLayer(IRenderLayer* layer) {
  self->layers.push_back(layer);
}

void Window::HandleResize() {
  self->context_initialized = false;
  int w, h;
  SDL_GL_GetDrawableSize(self->window, &w, &h);
  glViewport(0, 0, w, h);
}


void Window::Render() {
  glClear(GL_COLOR_BUFFER_BIT);
  for (auto layer : self->layers) {
    layer->Render(self->window, !self->context_initialized);
  }
  self->context_initialized = true;
  SDL_GL_SwapWindow(self->window);
  FRAME++;
}


WindowImpl::WindowImpl(SDL_Window* window_)
  :window(window_), context_initialized(false), context(window)
{
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  glClearColor(1.0, 1.0, 1.0, 1.0);
}

WindowImpl::~WindowImpl() {
  SDL_DestroyWindow(window);
}

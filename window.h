// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef WINDOW_H
#define WINDOW_H

#include "render-layer.h"
#include <memory>

struct SDL_Window;
union SDL_Event;
struct WindowImpl;

class Window {
public:
  Window(int width, int height);
  ~Window();
  void Render();
  void HandleResize();
  void AddLayer(IRenderLayer* layer);
  void ProcessEvent(SDL_Event* event);

  static int FRAME;
  bool visible;
  int width, height;
private:
  std::unique_ptr<WindowImpl> self;
};


#endif

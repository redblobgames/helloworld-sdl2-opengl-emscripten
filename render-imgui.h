// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef RENDER_IMGUI_H
#define RENDER_IMGUI_H

#include "render-layer.h"
#include <memory>

struct SDL_Window;
struct RenderImGuiImpl;


class RenderImGui: public IRenderLayer {
public:
  RenderImGui();
  ~RenderImGui();
  virtual void Render(SDL_Window* window, bool reset);
  virtual void ProcessEvent(SDL_Event* event);
  
protected:
  std::unique_ptr<RenderImGuiImpl> self;
};


#endif

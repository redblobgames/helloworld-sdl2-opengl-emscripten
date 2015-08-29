// From http://www.redblobgames.com/x/1535-opengl-emscripten/
// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include <iostream>
#include <SDL2/SDL.h>

#include "render.h"

static void SDLFAIL(const char* name) {
  std::cerr << name << " failed : " << SDL_GetError() << std::endl;
  exit(EXIT_FAILURE);
}

int main() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) { SDLFAIL("SDL_Init"); }
  
  Renderer renderer;
  renderer.Render();
    
  // TODO: main loop
  SDL_Delay(3000);

  SDL_Quit();
}

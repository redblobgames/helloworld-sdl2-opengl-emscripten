// From http://www.redblobgames.com/x/1535-opengl-emscripten/
// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include <iostream>
#include <SDL2/SDL.h>

#include "render.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

static void SDLFAIL(const char* name) {
  std::cerr << name << " failed : " << SDL_GetError() << std::endl;
  exit(EXIT_FAILURE);
}

static bool main_loop_running = true;

void main_loop() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      main_loop_running = false;
      break;
    }
  }
}


int main() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) { SDLFAIL("SDL_Init"); }
  SDL_GL_SetSwapInterval(1);
  
  Renderer renderer;
  renderer.Render();
    
#ifdef EMSCRIPTEN
  // 0 fps means to use requestAnimationFrame; non-0 means to use setTimeout.
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (main_loop_running) {
    main_loop();
    SDL_Delay(16);
  }
#endif
  
  SDL_Quit();
}

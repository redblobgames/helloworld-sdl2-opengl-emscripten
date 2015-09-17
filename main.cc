// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "common.h"
#include "glwrappers.h"
#include "window.h"
#include "render-sprites.h"
#include "render-surface.h"
#include "font.h"

#include <SDL2/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif


std::unique_ptr<Window> window;
std::unique_ptr<RenderSprites> sprite_layer;
static bool main_loop_running = true;
static bool main_loop_rendering = true;

float camera[2] = {0.0, 0.0};
float rotation = 0;

void main_loop() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT: {
      main_loop_running = false;
      break;
    }
    case SDL_KEYUP: {
      int sym = event.key.keysym.sym;
      if (sym == SDLK_ESCAPE) { main_loop_running = false; }
      break;
    }
    case SDL_MOUSEWHEEL: {
      // Panning is useful only if we have two axes from a trackpad; a
      // traditional scrollwheel should map to zoom. However we don't
      // know which is which.
      camera[0] += event.wheel.x * 0.1;
      camera[1] += event.wheel.y * 0.1;
      break;
    }
    case SDL_WINDOWEVENT: {
      switch (event.window.event) {
      case SDL_WINDOWEVENT_SHOWN: {
        main_loop_rendering = true;
        break;
      }
      case SDL_WINDOWEVENT_HIDDEN: {
        main_loop_rendering = false;
        break;
      }
      case SDL_WINDOWEVENT_SIZE_CHANGED: {
        window->HandleResize();
        break;
      }
      }
      break;
    }
    }
  }

  const Uint8* pressed = SDL_GetKeyboardState(nullptr);
  if (pressed[SDL_SCANCODE_Q]) { camera[0] -= 0.01; }
  if (pressed[SDL_SCANCODE_E]) { camera[0] += 0.01; }
  if (pressed[SDL_SCANCODE_W]) { camera[1] -= 0.01; }
  if (pressed[SDL_SCANCODE_S]) { camera[1] += 0.01; }
  if (pressed[SDL_SCANCODE_A]) { rotation -= 0.05; }
  if (pressed[SDL_SCANCODE_D]) { rotation += 0.05; }
  
  if (main_loop_rendering) {
    static float t = 0.0;
    t += 0.01;

    std::vector<Sprite> s;
    int SIDE = 40;
    int NUM = SIDE * SIDE;
    for (int j = 0; j < NUM; j++) {
      s.emplace_back();
      s[j].image_id = 0;
      s[j].x = (0.5 + j % SIDE - 0.5*SIDE + ((j/SIDE)%2) * 0.5 - 0.25) * 2.0 / SIDE;
      s[j].y = (0.5 + j / SIDE - 0.5*SIDE) * 2.0 / SIDE;
      s[j].scale = 2.0 / SIDE;
      s[j].rotation_degrees = 180/M_PI * (rotation + (j * 0.03 * t));
    }
    
    sprite_layer->SetSprites(s);
    
    window->Render();
  }
}


int main(int, char**) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) { FAIL("SDL_Init"); }
  SDL_GL_SetSwapInterval(1);

  window = std::unique_ptr<Window>(new Window(800, 600));

  Font font("assets/share-tech-mono.ttf", 32);

  SDL_Surface* overlay_surface = CreateRGBASurface(window->width, window->height);
  SDL_Rect fillarea;
  fillarea.x = 0;
  fillarea.y = 0;
  fillarea.w = overlay_surface->w;
  fillarea.h = 24;
  SDL_FillRect(overlay_surface, &fillarea, SDL_MapRGBA(overlay_surface->format, 64, 32, 0, 192));

  font.Draw(overlay_surface, 1, 21, "Hello");

  sprite_layer = std::unique_ptr<RenderSprites>(new RenderSprites);
  std::unique_ptr<RenderSurface> overlay_layer(new RenderSurface(overlay_surface));
  window->AddLayer(sprite_layer.get());
  // window->AddLayer(overlay_layer.get());

#ifdef __EMSCRIPTEN__
  // 0 fps means to use requestAnimationFrame; non-0 means to use setTimeout.
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (main_loop_running) {
    main_loop();
    SDL_Delay(16);
  }
#endif

  overlay_layer = nullptr;
  sprite_layer = nullptr;
  window = nullptr;
  SDL_Quit();
}

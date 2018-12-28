// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "common.h"
#include "glwrappers.h"
#include "window.h"
#include "render-sprites.h"
#include "render-shapes.h"
#include "render-surface.h"
#include "render-imgui.h"
#include "font.h"

#include <SDL2/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define SHOW_SPRITES 1
#define SHOW_SHAPES 0
#define SHOW_IMGUI 1
#define SHOW_OVERLAY 1

std::unique_ptr<Window> window;
std::unique_ptr<RenderSprites> sprite_layer;
std::unique_ptr<RenderShapes> shape_layer;
static bool main_loop_running = true;

Triangle tri(float x1, float y1, float x2, float y2, float x3, float y3) {
  return Triangle{{x1, x2, x3}, {y1, y2, y3}};
}

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
    }

    window->ProcessEvent(&event);
  }

  if (window->visible) {
    static float t = 0.0;
    t += 0.01;

#if SHOW_SPRITES
    {
      std::vector<Sprite> sprites;
      int SIDE = 4; // Try changing to 100; can't have more than 128 though because I use GLushort somewhere
      int NUM = SIDE * SIDE;
      for (int j = 0; j < NUM; j++) {
        sprites.emplace_back();
        auto& s = sprites.back();
        s.image_id = 0;
        s.x = (0.5 + j % SIDE - 0.5*SIDE + ((j/SIDE)%2) * 0.5 - 0.25) * 2.0 / SIDE;
        s.y = (0.5 + j / SIDE - 0.5*SIDE) * 2.0 / SIDE;
        s.scale = 2.0 / SIDE;
        s.rotation_degrees = 180/M_PI * (j * 0.03 * t);
      }

      sprite_layer->SetSprites(sprites);
    }
#endif

#if SHOW_SHAPES
    {
      std::vector<Shape> shapes;
      Shape s;
      s.r = 1.0; s.g = 0.5; s.b = 0.5; s.a = 1.0;
    
      s.triangles = {
        tri(0, -3, 1, -1, 0.5, 0),
        tri(1, -1, 3, 0, 0.5, 0),
        tri(3, 0, 1, 1, 0.5, 0),
        tri(1, 1, 0, 3, 0.5, 0),
        tri(0, 3, 0, 1, 0.5, 0),
        tri(0, 1, -1, 1, 0.5, 0),
        tri(-1, 1, -1, -1, 0.5, 0),
        tri(-1, -1, 0, -1, 0.5, 0),
        tri(0, -1, 0, -3, 0.5, 0)
      };
      shapes.push_back(s);
    
      shape_layer->SetShapes(shapes);
    }
#endif
    
    window->Render();
  }
}


int main(int, char**) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) { FAIL("SDL_Init"); }
  SDL_GL_SetSwapInterval(1);

  window = std::unique_ptr<Window>(new Window(800, 600));

  Font font("imgui/misc/fonts/DroidSans.ttf", 32);

  SDL_Surface* overlay_surface = CreateRGBASurface(window->width, window->height);
  SDL_Rect fillarea;
  fillarea.x = 0;
  fillarea.y = 0;
  fillarea.w = 2 + font.Width("Hello world");
  fillarea.h = font.Height();
  SDL_FillRect(overlay_surface, &fillarea, SDL_MapRGBA(overlay_surface->format, 64, 32, 0, 192));

  font.Draw(overlay_surface, 1, font.Baseline(), "Hello world");

#if SHOW_SPRITES
  sprite_layer = std::unique_ptr<RenderSprites>(new RenderSprites);
  window->AddLayer(sprite_layer.get());
#endif

#if SHOW_SHAPES
  shape_layer = std::unique_ptr<RenderShapes>(new RenderShapes);
  window->AddLayer(shape_layer.get());
#endif

#if SHOW_OVERLAY
  std::unique_ptr<RenderSurface> overlay_layer(new RenderSurface(overlay_surface));
  window->AddLayer(overlay_layer.get());
#endif

#if SHOW_IMGUI
  std::unique_ptr<RenderImGui> ui_layer(new RenderImGui());
  window->AddLayer(ui_layer.get());
#endif
  
#ifdef __EMSCRIPTEN__
  // 0 fps means to use requestAnimationFrame; non-0 means to use setTimeout.
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (main_loop_running) {
    main_loop();
    SDL_Delay(16);
  }
#endif

  sprite_layer = nullptr;
  shape_layer = nullptr;
  window = nullptr;
  SDL_Quit();
}

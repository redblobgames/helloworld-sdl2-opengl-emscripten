// From http://www.redblobgames.com/x/1535-opengl-emscripten/
// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef _RENDER_H_
#define _RENDER_H_

#include <memory>

struct RendererImpl;

class Renderer {
public:
  Renderer();
  ~Renderer();
  void Render();
private:
  std::unique_ptr<RendererImpl> self;
};

#endif

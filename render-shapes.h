// Copyright 2016 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

// Render shapes made up of polygons, with border/glow effects around the edges

#ifndef RENDER_SHAPES_H
#define RENDER_SHAPES_H

#include "render-layer.h"
#include <memory>
#include <vector>

struct SDL_Window;
struct RenderShapesImpl;


struct Triangle {
  // Vertices 1 and 2 should be on the exterior of the polygon, and
  // will have distance 0. Vertex 3 is on the interior of the polygon
  // and its distance will be calculated automatically.
  float x[3], y[3];
};

struct Shape {
  std::vector<Triangle> triangles;
  float r, g, b, a;
};


class RenderShapes: public IRenderLayer {
public:
  RenderShapes();
  ~RenderShapes();
  virtual void Render(SDL_Window* window, bool reset);

  void SetShapes(const std::vector<Shape>& shapes);
  
protected:
  std::unique_ptr<RenderShapesImpl> self;
};


#endif

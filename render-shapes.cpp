// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "render-shapes.h"
#include "window.h"

#include <SDL.h>
#include "glwrappers.h"

#include <vector>


namespace {
  
  struct Attributes {
    GLfloat position[2];
    GLfloat distance[3];
    GLfloat color[4];
  };
  
}

struct RenderShapesImpl {
  std::vector<Shape> sprites;
  std::vector<Attributes> vertices;
  
  ShaderProgram shader;
  
  VertexBuffer vbo;
  
  // Uniforms
  GLint loc_u_camera_position;
  GLint loc_u_camera_scale;
  
  // Attributes
  GLint loc_a_position;
  GLint loc_a_distance;
  GLint loc_a_color;

  RenderShapesImpl();
};


RenderShapes::RenderShapes(): self(new RenderShapesImpl) {}
RenderShapes::~RenderShapes() {}

// Shader program for drawing triangles
namespace {
  GLchar vertex_shader[] = R"(
  uniform vec2 u_camera_position;
  uniform vec2 u_camera_scale;
  attribute vec2 a_position;
  attribute vec3 a_distance;
  attribute vec4 a_color;
  varying vec3 v_distance;
  varying vec4 v_color;
  
  void main() {
    vec2 screen_coords = (a_position - u_camera_position) * u_camera_scale;
    gl_Position = vec4(screen_coords, 0.0, 1.0);
    v_distance = a_distance;
    v_color = a_color;
  }
)";

  GLchar fragment_shader[] = R"(
  varying vec3 v_distance;
  varying vec4 v_color;
  void main() {
/* NOTE: this doesn't handle corners correctly; that's what the other two edge fields are for, but I haven't figured out how to use them */
    gl_FragColor = 0.0 <= v_distance.x && v_distance.x <= 0.03 ? vec4(0.0,0.0,0.0,1.0) : v_color;
/* Debug: show colors for each of the three distance fields
    gl_FragColor = vec4(0.0 <= v_distance.y && v_distance.y <= 0.2 ? 1.0 : v_distance.y < 0.0? 0.5 : 0.0,
                        0.0 <= v_distance.x && v_distance.x <= 0.2 ? 0.5 : v_distance.x < 0.0? 0.3 : 0.0,
                        0.0 <= v_distance.z && v_distance.z <= 0.2 ? 1.0 : v_distance.z < 0.0? 0.5 : 0.0,
                        1.0);
*/
  }
)";
}


RenderShapesImpl::RenderShapesImpl()
  :shader(vertex_shader, fragment_shader)
{
  loc_u_camera_position = glGetUniformLocation(shader.id, "u_camera_position");
  loc_u_camera_scale = glGetUniformLocation(shader.id, "u_camera_scale");
  loc_a_position = glGetAttribLocation(shader.id, "a_position");
  loc_a_distance = glGetAttribLocation(shader.id, "a_distance");
  loc_a_color = glGetAttribLocation(shader.id, "a_color");
}


float distance_from_edge_to_point(float x1, float y1, float x2, float y2, float x, float y) {
  // This vertex gets distance set to the distance between the
  // vertex and the opposing edge. The distance can be
  // calculated in many different ways, such as calculating
  // the distance from the point to the line
  // <http://paulbourke.net/geometry/pointlineplane/>, or by
  // using the area of the triangle
  // <http://www.mathopenref.com/coordtrianglearea.html> and
  // setting that to 1/2 * base * height. I chose the area
  // approach; see
  // <https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line#Line_defined_by_two_points>
  float dx = x2 - x1, dy = y2 - y1;
  float length = hypot(dx, dy);
  return -(dy * x - dx * y + x2*y1 - y2*x1) / length;
}

void RenderShapes::SetShapes(const std::vector<Shape>& shapes) {
  auto& vertices = self->vertices;
  
  int N = 0;
  for (const auto& shape : shapes) { N += shape.triangles.size(); }
  vertices.resize(N * 3);

  int i = 0;
  for (const auto& shape : shapes) {
    for (size_t j = 0; j < shape.triangles.size(); ++j) {
      const auto& tri = shape.triangles[j];
      for (int k = 0; k < 3; ++k) {
        auto &V = vertices[i];
        V.position[0] = tri.x[k];
        V.position[1] = tri.y[k];
        V.distance[0] = 0.0;
        V.color[0] = shape.r;
        V.color[1] = shape.g;
        V.color[2] = shape.b;
        V.color[3] = shape.a;
        if (k == 2) {
          V.distance[0] = distance_from_edge_to_point(tri.x[0], tri.y[0], tri.x[1], tri.y[1], tri.x[2], tri.y[2]);
        }
        // Calculate distance to exterior edges of adjacent triangles
        // NOTE: I'm assuming adjacent triangles in the vector are
        // adjacent geometrically! I'll generalize later if I can make
        // this work properly.
        const auto& prev_tri = shape.triangles[(j - 1 + shape.triangles.size()) % shape.triangles.size()];
        const auto& next_tri = shape.triangles[(j + 1) % shape.triangles.size()];
        V.distance[1] = distance_from_edge_to_point(prev_tri.x[0], prev_tri.y[0], prev_tri.x[1], prev_tri.y[1], tri.x[k], tri.y[k]);
        V.distance[2] = distance_from_edge_to_point(next_tri.x[0], next_tri.y[0], next_tri.x[1], next_tri.y[1], tri.x[k], tri.y[k]);
        i++;
      }
    }
  }
}

  
void RenderShapes::Render(SDL_Window* window, bool reset) {
  glUseProgram(self->shader.id);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GLERRORS("useProgram");

  // The uniforms are data that will be the same for all records. The
  // attributes are data in each record. TODO: share camera with
  // render-sprites
  GLfloat u_camera_position[2] = { 0, 0 };
  glUniform2fv(self->loc_u_camera_position, 1, u_camera_position);

  // Rescale from world coordinates to OpenGL coordinates. We can
  // either choose for the world coordinates to have Y increasing
  // downwards (by setting u_camera_scale[1] to be negative) or have Y
  // increasing upwards and flipping the textures (by telling the
  // texture shader to flip the Y coordinate).
  int sdl_window_width, sdl_window_height;
  SDL_GL_GetDrawableSize(window, &sdl_window_width, &sdl_window_height);
  float sdl_window_size = std::min(sdl_window_height, sdl_window_width);
  GLfloat u_camera_scale[2] = { 0.3f * sdl_window_size / sdl_window_width,
                                0.3f * -sdl_window_size / sdl_window_height };
  glUniform2fv(self->loc_u_camera_scale, 1, u_camera_scale);
  
  GLERRORS("glUniform2fv");

  glBindBuffer(GL_ARRAY_BUFFER, self->vbo.id);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(Attributes) * self->vertices.size(),
               self->vertices.data(),
               GL_DYNAMIC_DRAW);
  
  // Tell the shader program where to find each of the input variables
  // ("attributes") in its vertex shader input.
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo.id);
  glVertexAttribPointer(self->loc_a_position,
                        2, GL_FLOAT, GL_FALSE, sizeof(Attributes),
                        reinterpret_cast<GLvoid*>(offsetof(Attributes, position)));
  glVertexAttribPointer(self->loc_a_distance,
                        3, GL_FLOAT, GL_FALSE, sizeof(Attributes),
                        reinterpret_cast<GLvoid*>(offsetof(Attributes, distance)));
  glVertexAttribPointer(self->loc_a_color,
                        4, GL_FLOAT, GL_FALSE, sizeof(Attributes),
                        reinterpret_cast<GLvoid*>(offsetof(Attributes, color)));
  
  GLERRORS("glVertexAttribPointer");

  // Run the shader program. Enable the vertex attribs just while
  // running this program. Which ones are enabled is global state, and
  // we don't want to interfere with any other shader programs we want
  // to run elsewhere.
  glEnableVertexAttribArray(self->loc_a_position);
  glEnableVertexAttribArray(self->loc_a_distance);
  glEnableVertexAttribArray(self->loc_a_color);
  glDrawArrays(GL_TRIANGLES, 0, self->vertices.size());
  glDisableVertexAttribArray(self->loc_a_color);
  glDisableVertexAttribArray(self->loc_a_distance);
  glDisableVertexAttribArray(self->loc_a_position);
  GLERRORS("draw arrays");

  glDisable(GL_BLEND);
}

// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "render-surface.h"
#include "window.h"

#include <SDL.h>
#include <SDL_image.h>
#include "glwrappers.h"

#include <vector>


struct RenderSurfaceImpl {
  SDL_Surface* surface;
  ShaderProgram shader;
  VertexBuffer vbo_pos;
  VertexBuffer vbo_tex;
  Texture texture;
  
  GLint loc_u_texture;
  GLint loc_a_position;
  GLint loc_a_texcoord;

  RenderSurfaceImpl(SDL_Surface* surface);
};


RenderSurface::RenderSurface(SDL_Surface* surface): self(new RenderSurfaceImpl(surface)) {}
RenderSurface::~RenderSurface() {}

// Shader program for drawing a single quad
namespace {
  GLchar vertex_shader[] = R"(
  attribute vec2 a_position;
  attribute vec2 a_texcoord;
  varying vec2 v_texcoord;
  void main() {
    gl_Position = vec4(a_position * 2.0 - 1.0, 0.0, 1.0);
    v_texcoord = a_texcoord;
  }
)";

  GLchar fragment_shader[] = R"(
  uniform sampler2D u_texture;
  varying vec2 v_texcoord;
  void main() {
    gl_FragColor = texture2D(u_texture, v_texcoord);
  }
)";

  // The positions (0-1) is the region of the screen to draw to, and
  // the texcoords (0-1) is the region of the surface to draw. Note
  // that texcoords are Y-axis-down and positions are Y-axis-up.
  GLfloat position[] = { 0, 1, 1, 1, 0, 0, 1, 0 };
  GLfloat texcoord[] = { 0, 0, 1, 0, 0, 1, 1, 1 };
}


RenderSurfaceImpl::RenderSurfaceImpl(SDL_Surface* surface_)
  :surface(surface_), shader(vertex_shader, fragment_shader)
{
  loc_u_texture = glGetUniformLocation(shader.id, "u_texture");
  loc_a_position = glGetAttribLocation(shader.id, "a_position");
  loc_a_texcoord = glGetAttribLocation(shader.id, "a_texcoord");
}


void RenderSurface::Render(SDL_Window* window, bool reset) {
  glUseProgram(self->shader.id);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  self->texture.CopyFromSurface(self->surface);
  
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, self->texture.id);
  glUniform1i(self->loc_u_texture, 0);
  
  if (reset) {
    glBindBuffer(GL_ARRAY_BUFFER, self->vbo_pos.id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, self->vbo_tex.id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texcoord), texcoord, GL_STATIC_DRAW);
  }
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo_pos.id);
  glVertexAttribPointer(self->loc_a_position,
                        2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), 0);
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo_tex.id);
  glVertexAttribPointer(self->loc_a_texcoord,
                        2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), 0);
  
  glEnableVertexAttribArray(self->loc_a_position);
  glEnableVertexAttribArray(self->loc_a_texcoord);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(self->loc_a_texcoord);
  glDisableVertexAttribArray(self->loc_a_position);

  glDisable(GL_BLEND);
}

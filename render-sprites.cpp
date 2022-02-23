// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "render-sprites.h"
#include "window.h"
#include "atlas.h"

#include <SDL.h>
#include "glwrappers.h"

#include <vector>


struct Attributes {
  GLfloat corner[2];   // location of corner relative to center, in world coords
  GLfloat texcoord[2]; // texture s,t of this corner
  GLfloat position[2]; // location of center in world coordinates
  GLfloat rotation;    // rotation in radians
};

struct RenderSpritesImpl {
  Atlas atlas;
  
  std::vector<Sprite> sprites;
  std::vector<Attributes> vertices;
  std::vector<GLushort> indices;
  
  ShaderProgram shader;
  Texture texture;
  
  VertexBuffer vbo_attributes;
  VertexBuffer vbo_index;
  
  // Uniforms
  GLint loc_u_camera_position;
  GLint loc_u_camera_scale;
  GLint loc_u_texture;
  
  // Attributes per sprite
  GLint loc_a_corner;
  GLint loc_a_texcoord;
  GLint loc_a_position;
  GLint loc_a_rotation;

  RenderSpritesImpl();
};


RenderSprites::RenderSprites(): self(new RenderSpritesImpl) {}
RenderSprites::~RenderSprites() {}

// Shader program for drawing sprites
namespace {
  GLchar vertex_shader[] = R"(
  uniform vec2 u_camera_position;
  uniform vec2 u_camera_scale;
  attribute vec2 a_corner;
  attribute vec2 a_texcoord;
  attribute vec2 a_position;
  attribute float a_rotation;
  varying vec2 v_texcoord;
  
  void main() {
    mat2 rot = mat2(cos(a_rotation), -sin(a_rotation), sin(a_rotation), cos(a_rotation));
    vec2 local_coords = a_corner * rot;
    vec2 world_coords = local_coords + a_position;
    vec2 screen_coords = (world_coords - u_camera_position) * u_camera_scale;
    gl_Position = vec4(screen_coords, 0.0, 1.0);
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
}


RenderSpritesImpl::RenderSpritesImpl()
  :shader(vertex_shader, fragment_shader)
{
  loc_u_camera_position = glGetUniformLocation(shader.id, "u_camera_position");
  loc_u_camera_scale = glGetUniformLocation(shader.id, "u_camera_scale");
  loc_u_texture = glGetUniformLocation(shader.id, "u_texture");
  loc_a_corner = glGetAttribLocation(shader.id, "a_corner");
  loc_a_texcoord = glGetAttribLocation(shader.id, "a_texcoord");
  loc_a_position = glGetAttribLocation(shader.id, "a_position");
  loc_a_rotation = glGetAttribLocation(shader.id, "a_rotation");

  atlas.LoadImage("assets/red-blob.png");
  texture.CopyFromSurface(atlas.GetSurface());
}


namespace {
  static const GLushort corner_index[6] = { 0, 1, 2, 2, 1, 3 };
}


void RenderSprites::SetSprites(const std::vector<Sprite>& sprites) {
  auto& vertices = self->vertices;
  auto& indices = self->indices;
  
  static float t = 0.0;
  t += 0.01f;

  int N = sprites.size();
  vertices.resize(N * 4);
  for (int j = 0; j < N; j++) {
    const Sprite& S = sprites[j];
    SpriteLocation loc = self->atlas.GetLocation(S.image_id);
    int i = j * 4;
    vertices[i].corner[0] = loc.x0 * S.scale;
    vertices[i].corner[1] = loc.y0 * S.scale;
    vertices[i].texcoord[0] = loc.s0;
    vertices[i].texcoord[1] = loc.t0;
    i++;
    vertices[i].corner[0] = loc.x1 * S.scale;
    vertices[i].corner[1] = loc.y0 * S.scale;
    vertices[i].texcoord[0] = loc.s1;
    vertices[i].texcoord[1] = loc.t0;
    i++;
    vertices[i].corner[0] = loc.x0 * S.scale;
    vertices[i].corner[1] = loc.y1 * S.scale;
    vertices[i].texcoord[0] = loc.s0;
    vertices[i].texcoord[1] = loc.t1;
    i++;
    vertices[i].corner[0] = loc.x1 * S.scale;
    vertices[i].corner[1] = loc.y1 * S.scale;
    vertices[i].texcoord[0] = loc.s1;
    vertices[i].texcoord[1] = loc.t1;
  }

  for (int i = 0; i < N * 4; i++) {
    int j = i / 4;
    vertices[i].position[0] = sprites[j].x;
    vertices[i].position[1] = sprites[j].y;
    vertices[i].rotation = sprites[j].rotation_degrees / DEG_TO_RAD;
  }
  
  indices.resize(N * 6);
  for (int i = 0; i < N * 6; i++) {
    int j = i / 6;
    indices[i] = j * 4 + corner_index[i % 6];
  }
}

  
void RenderSprites::Render(SDL_Window* window, bool reset) {
  glUseProgram(self->shader.id);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GLERRORS("useProgram");

  // The uniforms are data that will be the same for all records. The
  // attributes are data in each record. TODO: export camera
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
  GLfloat u_camera_scale[2] = { sdl_window_size / sdl_window_width,
                                -sdl_window_size / sdl_window_height };
  glUniform2fv(self->loc_u_camera_scale, 1, u_camera_scale);
  
  GLERRORS("glUniform2fv");

  // Textures have an id and also a register (0 in this
  // case). We have to bind register 0 to the texture id:
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, self->texture.id);
  // and then we have to tell the shader which register (0) to use:
  glUniform1i(self->loc_u_texture, 0);
  // It might be ok to hard-code the register number inside the shader.
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->vbo_index.id);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               sizeof(GLushort) * self->indices.size(),
               self->indices.data(),
               GL_DYNAMIC_DRAW);

  // Tell the shader program where to find each of the input variables
  // ("attributes") in its vertex shader input.
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo_attributes.id);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(Attributes) * self->vertices.size(),
               self->vertices.data(),
               GL_STREAM_DRAW);
  
  glVertexAttribPointer(self->loc_a_corner,
                        2, GL_FLOAT, GL_FALSE, sizeof(Attributes),
                        reinterpret_cast<GLvoid*>(offsetof(Attributes, corner)));
  glVertexAttribPointer(self->loc_a_texcoord,
                        2, GL_FLOAT, GL_FALSE, sizeof(Attributes),
                        reinterpret_cast<GLvoid*>(offsetof(Attributes, texcoord)));
  glVertexAttribPointer(self->loc_a_position,
                        2, GL_FLOAT, GL_FALSE, sizeof(Attributes),
                        reinterpret_cast<GLvoid*>(offsetof(Attributes, position)));
  glVertexAttribPointer(self->loc_a_rotation,
                        1, GL_FLOAT, GL_FALSE, sizeof(Attributes),
                        reinterpret_cast<GLvoid*>(offsetof(Attributes, rotation)));
  
  GLERRORS("glVertexAttribPointer");

  // Run the shader program. Enable the vertex attribs just while
  // running this program. Which ones are enabled is global state, and
  // we don't want to interfere with any other shader programs we want
  // to run elsewhere.
  glEnableVertexAttribArray(self->loc_a_corner);
  glEnableVertexAttribArray(self->loc_a_texcoord);
  glEnableVertexAttribArray(self->loc_a_position);
  glEnableVertexAttribArray(self->loc_a_rotation);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->vbo_index.id);
  glDrawElements(GL_TRIANGLES, self->indices.size(), GL_UNSIGNED_SHORT, 0);
  glDisableVertexAttribArray(self->loc_a_rotation);
  glDisableVertexAttribArray(self->loc_a_position);
  glDisableVertexAttribArray(self->loc_a_texcoord);
  glDisableVertexAttribArray(self->loc_a_corner);
  GLERRORS("draw arrays");

  glDisable(GL_BLEND);
}

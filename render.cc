// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "render.h"
#include "textures.h"

#include <SDL2/SDL.h>
#include "glwrappers.h"

#include <iostream>
#include <vector>

void GLERRORS(const char* label) {
#ifndef __EMSCRIPTEN__
  while (true) {
    GLenum err = glGetError();
    if (err == GL_NO_ERROR) { break; }
    std::cerr << label << " glGetError returned " << err << std::endl;
  }
#endif
}

void FAIL(const char* label) {
  GLERRORS(label);
  std::cerr << label << " failed : " << SDL_GetError() << std::endl;
  exit(EXIT_FAILURE);
}


struct AttributesStatic {
  float corner[2]; // location of corner relative to center, in world coords
  float texcoord[2]; // texture s,t of this corner
};

struct AttributesDynamic {
  float position[2]; // location of center in world coordinates
  float rotation; // rotation in radians
};

struct RendererImpl {
  bool context_initialized;
  SDL_Window* window;
  SDL_GLContext context;
  Textures textures;
  std::unique_ptr<VertexBuffer> vbo_static;
  std::unique_ptr<VertexBuffer> vbo_dynamic;
  std::unique_ptr<VertexBuffer> vbo_index;
  std::unique_ptr<ShaderProgram> shader;
  std::unique_ptr<Texture> texture;
  
  // Uniforms
  GLint loc_u_camera_position;
  GLint loc_u_camera_scale;
  GLint loc_u_texture;
  
  // A sprite is described with "static" attributes:
  GLint loc_a_corner;
  GLint loc_a_texcoord;

  // and "dynamic" attributes that are expected to change every frame:
  GLint loc_a_position;
  GLint loc_a_rotation;

  RendererImpl(SDL_Window* window);
  ~RendererImpl();
};


Renderer::Renderer(SDL_Window* window): self(new RendererImpl(window)) {}
Renderer::~Renderer() {}


// Shader program for drawing sprites

GLchar vertex_shader[] =
  "uniform vec2 u_camera_position;\n"
  "uniform vec2 u_camera_scale;\n"
  "attribute vec2 a_corner;\n"
  "attribute vec2 a_texcoord;\n"
  "attribute vec2 a_position;\n"
  "attribute float a_rotation;\n"
  "varying vec2 v_texcoord;\n"
  "\n"
  "void main() {\n"
  "  mat2 rot = mat2(cos(a_rotation), -sin(a_rotation), sin(a_rotation), cos(a_rotation));\n"
  "  vec2 local_coords = a_corner * rot;\n"
  "  vec2 world_coords = local_coords + a_position;\n"
  "  vec2 screen_coords = (world_coords - u_camera_position) * u_camera_scale;\n"
  "  gl_Position = vec4(screen_coords, 0.0, 1.0);\n"
  "  v_texcoord = a_texcoord;\n"
  "}\n";

GLchar fragment_shader[] =
#ifdef __EMSCRIPTEN__
  // NOTE(amitp): WebGL requires precision hints but OpenGL 2.1 disallows them
  "precision mediump float;\n"
  "\n"
#endif
  "uniform sampler2D u_texture;\n"
  "varying vec2 v_texcoord;\n"
  "void main() {\n"
  "  gl_FragColor = texture2D(u_texture, v_texcoord);\n"
  "}\n";



RendererImpl::RendererImpl(SDL_Window* window_):
  context_initialized(false), window(window_)
{
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  // TODO: once the context is being created separately from shader,
  // etc., the shader etc don't have to be unique_ptr anymore
  
  context = SDL_GL_CreateContext(window);
  if (context == nullptr) { FAIL("SDL_GL_CreateContext"); }
  
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  shader = std::unique_ptr<ShaderProgram>(new ShaderProgram(vertex_shader, fragment_shader));
  vbo_static = std::unique_ptr<VertexBuffer>(new VertexBuffer);
  vbo_dynamic = std::unique_ptr<VertexBuffer>(new VertexBuffer);
  vbo_index = std::unique_ptr<VertexBuffer>(new VertexBuffer);

  loc_u_camera_position = glGetUniformLocation(shader->id, "u_camera_position");
  loc_u_camera_scale = glGetUniformLocation(shader->id, "u_camera_scale");
  loc_u_texture = glGetUniformLocation(shader->id, "u_texture");
  loc_a_corner = glGetAttribLocation(shader->id, "a_corner");
  loc_a_texcoord = glGetAttribLocation(shader->id, "a_texcoord");
  loc_a_position = glGetAttribLocation(shader->id, "a_position");
  loc_a_rotation = glGetAttribLocation(shader->id, "a_rotation");

  // textures.LoadFont("assets/share-tech-mono.ttf", 52.0);
  textures.LoadImage("assets/red-blob.png");
  texture = std::unique_ptr<Texture>(new Texture(textures.GetSurface()));
}

RendererImpl::~RendererImpl() {
  texture = nullptr;
  shader = nullptr;
  vbo_index = nullptr;
  vbo_dynamic = nullptr;
  vbo_static = nullptr;
  SDL_GL_DeleteContext(context);
  GLERRORS("~RendererImpl");
}


void Renderer::HandleResize() {
  // This is the easiest way to handle resize but it'd be better not
  // to regenerate the texture.
  SDL_Window* window = self->window;
  self = nullptr;
  self = std::unique_ptr<RendererImpl>(new RendererImpl(window));
}

extern float rotation;
namespace {
  static int FRAME = -1;
  
  const int SIDE = 40;
  const int NUM = SIDE * SIDE;

  std::vector<AttributesStatic> vertices_static;
  std::vector<AttributesDynamic> vertices_dynamic;
  std::vector<GLushort> indices;
  static const GLushort corner_index[6] = { 0, 1, 2, 2, 1, 3 };

  void FillVertexData(const Textures& textures) {
    static float t = 0.0;
    t += 0.01;

    float scale = 2.0 / SIDE;
    if (FRAME % 100 == 0) {
      vertices_static.resize(NUM * 4);
      for (int j = 0; j < NUM; j++) {
        SpriteLocation loc = textures.GetLocation(0);
        // loc.s0 = 0; loc.t0 = 0; loc.s1 = 1; loc.t1 = 1;
        int i = j * 4;
        vertices_static[i].corner[0] = loc.x0 * scale;
        vertices_static[i].corner[1] = loc.y0 * scale;
        vertices_static[i].texcoord[0] = loc.s0;
        vertices_static[i].texcoord[1] = loc.t0;
        i++;
        vertices_static[i].corner[0] = loc.x1 * scale;
        vertices_static[i].corner[1] = loc.y0 * scale;
        vertices_static[i].texcoord[0] = loc.s1;
        vertices_static[i].texcoord[1] = loc.t0;
        i++;
        vertices_static[i].corner[0] = loc.x0 * scale;
        vertices_static[i].corner[1] = loc.y1 * scale;
        vertices_static[i].texcoord[0] = loc.s0;
        vertices_static[i].texcoord[1] = loc.t1;
        i++;
        vertices_static[i].corner[0] = loc.x1 * scale;
        vertices_static[i].corner[1] = loc.y1 * scale;
        vertices_static[i].texcoord[0] = loc.s1;
        vertices_static[i].texcoord[1] = loc.t1;
      }

      indices.resize(NUM * 6);
      for (int i = 0; i < NUM * 6; i++) {
        int j = i / 6;
        indices[i] = j * 4 + corner_index[i % 6];
      }
    }
    
    vertices_dynamic.resize(NUM * 4);
    for (int i = 0; i < NUM * 4; i++) {
      AttributesDynamic& record = vertices_dynamic[i];
      int j = i / 4;
      record.position[0] = (0.5 + j % SIDE - 0.5*SIDE + ((j/SIDE)%2) * 0.5 - 0.25) * 2.0 / SIDE;
      record.position[1] = (0.5 + j / SIDE - 0.5*SIDE) * 2.0 / SIDE;
      record.rotation = rotation + (j * 0.03 * t);
    }
  }
}

void Renderer::Render() {
  FRAME++;
  glClear(GL_COLOR_BUFFER_BIT);
  
  glUseProgram(self->shader->id);
  GLERRORS("useProgram");

  // The uniforms are data that will be the same for all records. The
  // attributes are data in each record.
  extern float camera[2];
  GLfloat u_camera_position[2] = { camera[0], camera[1] };
  glUniform2fv(self->loc_u_camera_position, 1, u_camera_position);

  // Rescale from world coordinates to OpenGL coordinates. We can
  // either choose for the world coordinates to have Y increasing
  // downwards (by setting u_camera_scale[1] to be negative) or have Y
  // increasing upwards and flipping the textures (by telling the
  // texture shader to flip the Y coordinate).
  int sdl_window_width, sdl_window_height;
  SDL_GL_GetDrawableSize(self->window, &sdl_window_width, &sdl_window_height);
  float sdl_window_size = std::min(sdl_window_height, sdl_window_width);
  GLfloat u_camera_scale[2] = { sdl_window_size / sdl_window_width,
                                -sdl_window_size / sdl_window_height };
  glUniform2fv(self->loc_u_camera_scale, 1, u_camera_scale);
  
  GLERRORS("glUniform2fv");

  // Textures have an id and also a register (0 in this
  // case). We have to bind register 0 to the texture id:
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, self->texture->id);
  // and then we have to tell the shader which register (0) to use:
  glUniform1i(self->loc_u_texture, 0);
  // It might be ok to hard-code the register number inside the shader.
  
  // The data for the vertex shader will be in a vertex buffer
  // ("VBO"). I'm going to use a single vertex buffer for all the
  // parameters, and I'm going to fill it each frame.
  FillVertexData(self->textures);

  if (FRAME % 100 == 0 || !self->context_initialized) {
    glBindBuffer(GL_ARRAY_BUFFER, self->vbo_static->id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(AttributesStatic) * vertices_static.size(), vertices_static.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->vbo_index->id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices.size(), indices.data(), GL_DYNAMIC_DRAW);
  }
  
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo_dynamic->id);
  glBufferData(GL_ARRAY_BUFFER, sizeof(AttributesDynamic) * vertices_dynamic.size(), vertices_dynamic.data(), GL_STREAM_DRAW);
  
  // Tell the shader program where to find each of the input variables
  // ("attributes") in its vertex shader input. They're all coming
  // from the same vertex buffer, but they're different slices of that
  // data. The API is flexible enough to allow us to have a start
  // position and stride within the vertex buffer. This allows us to
  // bind the position and rotation inside the struct.
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo_static->id);
  glVertexAttribPointer(self->loc_a_corner,
                        2, GL_FLOAT, GL_FALSE, sizeof(AttributesStatic),
                        reinterpret_cast<GLvoid*>(offsetof(AttributesStatic, corner)));
  glVertexAttribPointer(self->loc_a_texcoord,
                        2, GL_FLOAT, GL_FALSE, sizeof(AttributesStatic),
                        reinterpret_cast<GLvoid*>(offsetof(AttributesStatic, texcoord)));
  
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo_dynamic->id);
  glVertexAttribPointer(self->loc_a_position,
                        2, GL_FLOAT, GL_FALSE, sizeof(AttributesDynamic),
                        reinterpret_cast<GLvoid*>(offsetof(AttributesDynamic, position)));
  glVertexAttribPointer(self->loc_a_rotation,
                        1, GL_FLOAT, GL_FALSE, sizeof(AttributesDynamic),
                        reinterpret_cast<GLvoid*>(offsetof(AttributesDynamic, rotation)));
  
  GLERRORS("glVertexAttribPointer");

  self->context_initialized = true;
  
  // Run the shader program. Enable the vertex attribs just while
  // running this program. Which ones are enabled is global state, and
  // we don't want to interfere with any other shader programs we want
  // to run elsewhere.
  glEnableVertexAttribArray(self->loc_a_corner);
  glEnableVertexAttribArray(self->loc_a_texcoord);
  glEnableVertexAttribArray(self->loc_a_position);
  glEnableVertexAttribArray(self->loc_a_rotation);
  glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, 0);
  glDisableVertexAttribArray(self->loc_a_rotation);
  glDisableVertexAttribArray(self->loc_a_position);
  glDisableVertexAttribArray(self->loc_a_texcoord);
  glDisableVertexAttribArray(self->loc_a_corner);
  GLERRORS("draw arrays");

  // In double buffering, all the drawing was to a screen buffer that
  // will now be displayed with this command:
  SDL_GL_SwapWindow(self->window);
}

// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "render.h"
#include <SDL2/SDL.h>

#include "gl.h"
#include "textures.h"

#include <iostream>
#include <vector>

void GLERRORS(const char* name) {
#ifndef __EMSCRIPTEN__
  while (true) {
    GLenum err = glGetError();
    if (err == GL_NO_ERROR) { break; }
    std::cerr << name << " glGetError returned " << err << std::endl;
  }
#endif
}

static void SDLFAIL(const char* name) {
  GLERRORS(name);
  std::cerr << name << " failed : " << SDL_GetError() << std::endl;
  exit(EXIT_FAILURE);
}


struct VertexBuffer {
  GLuint sid, did, iid; // static vertex, dynamic vertex, and index

  VertexBuffer();
  ~VertexBuffer();
};

struct ShaderProgram {
  GLuint id;

  GLint loc_u_camera_position;
  GLint loc_u_camera_scale;
  GLint loc_u_texture;
  GLint loc_a_corner;
  GLint loc_a_position;
  GLint loc_a_rotation;
  GLint loc_a_scale;

  ShaderProgram();
  ~ShaderProgram();
  void AttachShader(GLenum type, const GLchar* source);
};

struct AttributesStatic {
  float corner[2]; // location of corner relative to center
  float scale; // size of sprite, in world coordinates
};

struct AttributesDynamic {
  float position[2]; // location of center in world coordinates
  float rotation; // rotation in radians
};

struct RendererImpl {
  SDL_Window* window;
  SDL_GLContext context;
  GLuint texture_id;
  Textures textures;
  std::unique_ptr<VertexBuffer> vbo;
  std::unique_ptr<ShaderProgram> shader;

  RendererImpl(SDL_Window* window);
  ~RendererImpl();
};


Renderer::Renderer(SDL_Window* window): self(new RendererImpl(window)) {}
Renderer::~Renderer() {}


RendererImpl::RendererImpl(SDL_Window* window_): window(window_) {
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  context = SDL_GL_CreateContext(window);
  if (context == nullptr) { SDLFAIL("SDL_GL_CreateContext"); }
  
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  shader = std::unique_ptr<ShaderProgram>(new ShaderProgram);
  vbo = std::unique_ptr<VertexBuffer>(new VertexBuffer);

  auto surface = textures.GetSurface();
  if (surface == nullptr) { SDLFAIL("Loading red-blob.png"); }

  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexImage2D(GL_TEXTURE_2D, 0,
               GL_RGBA,
               surface->w, surface->h,
               0,
               surface->format->BytesPerPixel == 1? GL_ALPHA
               : surface->format->BytesPerPixel == 3? GL_RGB
               : GL_RGBA /* TODO: check for other formats */,
               GL_UNSIGNED_BYTE,
               surface->pixels
               );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  SDL_FreeSurface(surface);
  
  GLERRORS("Texture load");
}

RendererImpl::~RendererImpl() {
  shader = nullptr;
  vbo = nullptr;
  SDL_GL_DeleteContext(context);
  GLERRORS("~RendererImpl");
}


void Renderer::HandleResize() {
  // This is the easiest way to handle resize but it'd be better not
  // to regenerate the texture
  SDL_Window* window = self->window;
  self = nullptr;
  self = std::unique_ptr<RendererImpl>(new RendererImpl(window));
}

extern float rotation;
namespace {
  static int FRAME = -1;
  
  const int SIDE = 100;
  const int NUM = SIDE * SIDE;

  std::vector<AttributesStatic> vertices_static;
  std::vector<AttributesDynamic> vertices_dynamic;
  std::vector<GLushort> indices;
  static const float corner_vertex[4][2] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
  static const GLushort corner_index[6] = { 0, 1, 2, 2, 1, 3 };

  void FillVertexData() {
    static float t = 0.0;
    t += 0.025;

    if (FRAME % 100 == 0) {
      vertices_static.resize(NUM * 4);
      for (int i = 0; i < NUM * 4; i++) {
        AttributesStatic& record = vertices_static[i];
        record.corner[0] = corner_vertex[i % 4][0];
        record.corner[1] = corner_vertex[i % 4][1];
        record.scale = 3.0/SIDE;
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
      record.position[0] = -1.0 + (j % SIDE) * 2.0 / SIDE;
      record.position[1] = -1.0 + (j / SIDE) * 2.0 / SIDE;
      record.rotation = rotation + (j * 0.02) + t;
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
  glUniform2fv(self->shader->loc_u_camera_position, 1, u_camera_position);

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
  glUniform2fv(self->shader->loc_u_camera_scale, 1, u_camera_scale);
  
  GLERRORS("glUniform2fv");

  // Textures have an id and also a register (0 in this
  // case). We have to bind register 0 to the texture id:
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, self->texture_id);
  // and then we have to tell the shader which register (0) to use:
  glUniform1i(self->shader->loc_u_texture, 0);
  // It might be ok to hard-code the register number inside the shader.
  
  // The data for the vertex shader will be in a vertex buffer
  // ("VBO"). I'm going to use a single vertex buffer for all the
  // parameters, and I'm going to fill it each frame.
  FillVertexData();

  if (FRAME % 100 == 0) {
    glBindBuffer(GL_ARRAY_BUFFER, self->vbo->sid);
    glBufferData(GL_ARRAY_BUFFER, sizeof(AttributesStatic) * vertices_static.size(), vertices_static.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->vbo->iid);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices.size(), indices.data(), GL_STREAM_DRAW);
  }
  
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo->did);
  glBufferData(GL_ARRAY_BUFFER, sizeof(AttributesDynamic) * vertices_dynamic.size(), vertices_dynamic.data(), GL_STREAM_DRAW);
  
  // Tell the shader program where to find each of the input variables
  // ("attributes") in its vertex shader input. They're all coming
  // from the same vertex buffer, but they're different slices of that
  // data. The API is flexible enough to allow us to have a start
  // position and stride within the vertex buffer. This allows us to
  // bind the position and rotation inside the struct.
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo->sid);
  glVertexAttribPointer(self->shader->loc_a_corner,
                        2, GL_FLOAT, GL_FALSE, sizeof(AttributesStatic),
                        reinterpret_cast<GLvoid*>(offsetof(AttributesStatic, corner)));
  glVertexAttribPointer(self->shader->loc_a_scale,
                        1, GL_FLOAT, GL_FALSE, sizeof(AttributesStatic),
                        reinterpret_cast<GLvoid*>(offsetof(AttributesStatic, scale)));
  
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo->did);
  glVertexAttribPointer(self->shader->loc_a_position,
                        2, GL_FLOAT, GL_FALSE, sizeof(AttributesDynamic),
                        reinterpret_cast<GLvoid*>(offsetof(AttributesDynamic, position)));
  glVertexAttribPointer(self->shader->loc_a_rotation,
                        1, GL_FLOAT, GL_FALSE, sizeof(AttributesDynamic),
                        reinterpret_cast<GLvoid*>(offsetof(AttributesDynamic, rotation)));
  
  GLERRORS("glVertexAttribPointer");

  // Run the shader program. Enable the vertex attribs just while
  // running this program. Which ones are enabled is global state, and
  // we don't want to interfere with any other shader programs we want
  // to run elsewhere.
  glEnableVertexAttribArray(self->shader->loc_a_corner);
  glEnableVertexAttribArray(self->shader->loc_a_position);
  glEnableVertexAttribArray(self->shader->loc_a_rotation);
  glEnableVertexAttribArray(self->shader->loc_a_scale);
  glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, 0);
  glDisableVertexAttribArray(self->shader->loc_a_scale);
  glDisableVertexAttribArray(self->shader->loc_a_rotation);
  glDisableVertexAttribArray(self->shader->loc_a_position);
  glDisableVertexAttribArray(self->shader->loc_a_corner);
  GLERRORS("draw arrays");

  // In double buffering, all the drawing was to a screen buffer that
  // will now be displayed with this command:
  SDL_GL_SwapWindow(self->window);
}


// Vertex buffer stores the data needed to run the shader program

VertexBuffer::VertexBuffer() {
  glGenBuffers(1, &sid);
  glGenBuffers(1, &did);
  glGenBuffers(1, &iid);
  GLERRORS("VertexBuffer()");
}

VertexBuffer::~VertexBuffer() {
  glDeleteBuffers(1, &iid);
  glDeleteBuffers(1, &did);
  glDeleteBuffers(1, &sid);
}


// Shader program for drawing sprites

GLchar vertex_shader[] =
  "uniform vec2 u_camera_position;\n"
  "uniform vec2 u_camera_scale;\n"
  "attribute vec2 a_corner;\n"
  "attribute vec2 a_position;\n"
  "attribute float a_rotation;\n"
  "attribute float a_scale;\n"
  "varying vec2 loc;\n"
  "\n"
  "void main() {\n"
  "  mat2 rot = mat2(cos(a_rotation), -sin(a_rotation), sin(a_rotation), cos(a_rotation));\n"
  "  vec2 local_coords = (a_corner - vec2(0.5,0.5)) * rot * a_scale;\n"
  "  vec2 world_coords = local_coords + a_position;\n"
  "  vec2 screen_coords = (world_coords - u_camera_position) * u_camera_scale;\n"
  "  gl_Position = vec4(screen_coords, 0.0, 1.0);\n"
  "  loc = a_corner;\n"
  "}\n";

GLchar fragment_shader[] =
#ifdef __EMSCRIPTEN__
  // NOTE(amitp): WebGL requires precision hints but OpenGL 2.1 disallows them
  "precision mediump float;\n"
  "\n"
#endif
  "uniform sampler2D u_texture;\n"
  "varying vec2 loc;\n"
  "void main() {\n"
  "  gl_FragColor = texture2D(u_texture, loc);\n"
  "}\n";

ShaderProgram::ShaderProgram() {
  id = glCreateProgram();
  if (id == 0) { SDLFAIL("glCreateProgram"); }

  AttachShader(GL_VERTEX_SHADER, vertex_shader);
  AttachShader(GL_FRAGMENT_SHADER, fragment_shader);
  glLinkProgram(id);
  
  GLint link_status;
  glGetProgramiv(id, GL_LINK_STATUS, &link_status);
  if (!link_status) {
    GLint log_length;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
    GLchar log[1024];
    glGetProgramInfoLog(id, 1024, nullptr, log);
    std::cerr << log << std::endl;
    SDLFAIL("link shaders");
  }

  loc_u_camera_position = glGetUniformLocation(id, "u_camera_position");
  loc_u_camera_scale = glGetUniformLocation(id, "u_camera_scale");
  loc_u_texture = glGetUniformLocation(id, "u_texture");
  loc_a_corner = glGetAttribLocation(id, "a_corner");
  loc_a_position = glGetAttribLocation(id, "a_position");
  loc_a_rotation = glGetAttribLocation(id, "a_rotation");
  loc_a_scale = glGetAttribLocation(id, "a_scale");
}

ShaderProgram::~ShaderProgram() {
  glDeleteProgram(id);
}

void ShaderProgram::AttachShader(GLenum type, const GLchar* source) {
  GLuint shader_id = glCreateShader(type);
  if (shader_id == 0) { SDLFAIL("load shader"); }
           
  glShaderSource(shader_id, 1, &source, nullptr);
  glCompileShader(shader_id);

  GLint compile_status;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_status);
  if (!compile_status) {
    GLint log_length;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
    GLchar log[1024];
    glGetShaderInfoLog(shader_id, 1024, nullptr, log);
    std::cerr << log << std::endl;
    SDLFAIL("compile shader");
  }

  glAttachShader(id, shader_id);
  glDeleteShader(shader_id);
  GLERRORS("AttachShader()");
}

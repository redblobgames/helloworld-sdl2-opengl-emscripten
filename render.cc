// From http://www.redblobgames.com/x/1535-opengl-emscripten/
// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "render.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#ifdef __APPLE__
// NOTE(amitp): Mac doesn't have OpenGL ES headers, and the path to GL
// headers is different.
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <SDL2/SDL_opengles2.h>
#endif


#include <iostream>
#include <vector>

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500


static void GLERRORS(const char* name) {
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
  GLuint id;

  VertexBuffer();
  ~VertexBuffer();
};

struct ShaderProgram {
  GLuint id;
  
  ShaderProgram();
  ~ShaderProgram();
  void AttachShader(GLenum type, const GLchar* source);
};

struct Attributes {
  float corner[2]; // location of corner relative to center
  float position[2]; // location of center in world coordinates
  float rotation; // rotation in radians
  float scale; // size of sprite, in world coordinates
};

struct RendererImpl {
  SDL_Window* window;
  SDL_GLContext context;
  GLuint texture_id;
  std::unique_ptr<VertexBuffer> vbo;
  std::unique_ptr<ShaderProgram> shader;
};


Renderer::Renderer(): self(new RendererImpl) {
  // We always want double buffering
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  
  self->window = SDL_CreateWindow("Hello World",
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SCREEN_WIDTH,
                                 SCREEN_HEIGHT,
                                 SDL_WINDOW_OPENGL
                                 );
  if (self->window == nullptr) { SDLFAIL("SDL_CreateWindow"); }

  self->context = SDL_GL_CreateContext(self->window);
  if (self->context == nullptr) { SDLFAIL("SDL_GL_CreateContext"); }
  
  glClearColor(1.0, 1.0, 1.0, 1.0);

  self->shader = std::unique_ptr<ShaderProgram>(new ShaderProgram);
  self->vbo = std::unique_ptr<VertexBuffer>(new VertexBuffer);

  auto surface = IMG_Load("assets/red-blob.png");
  if (surface == nullptr) { SDLFAIL("Loading red-blob.png"); }

  glGenTextures(1, &self->texture_id);
  glBindTexture(GL_TEXTURE_2D, self->texture_id);
  glTexImage2D(GL_TEXTURE_2D, 0,
               GL_RGBA,
               surface->w, surface->h,
               0,
               GL_RGBA /* for red-blob.bmp only */,
               GL_UNSIGNED_BYTE,
               surface->pixels
               );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  SDL_FreeSurface(surface);
  
  GLERRORS("Texture load");
}

Renderer::~Renderer() {
  SDL_GL_DeleteContext(self->context);
  SDL_DestroyWindow(self->window);
  GLERRORS("~Renderer");
}


void Renderer::Render() {
  glClear(GL_COLOR_BUFFER_BIT);
  GLERRORS("Clear");
  
  glUseProgram(self->shader->id);
  GLERRORS("useProgram");

  // The uniforms are data that will be the same for all records. The
  // attributes are data in each record.
  extern float camera[2], rotation;
  GLfloat u_camera[2] = { camera[0], camera[1] };
  glUniform2fv(glGetUniformLocation(self->shader->id, "u_camera"), 1, u_camera);
  GLERRORS("glUniform2fv");

  // Textures have an id and also a register (0 in this
  // case). We have to bind register 0 to the texture id:
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, self->texture_id);
  // and then we have to tell the shader which register (0) to use:
  glUniform1i(glGetUniformLocation(self->shader->id, "u_texture"), 0);
  // It might be ok to hard-code the register number inside the shader.
  
  // The data for the vertex shader will be in a vertex buffer
  // ("VBO"). I'm going to use a single vertex buffer for all the
  // parameters, and I'm going to fill it each frame.
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo->id);
  static const float corners[6][2] = {
    { -0.5,  0.5 },
    {  0.5,  0.5 },
    {  0.5, -0.5 },
    {  0.5, -0.5 },
    { -0.5, -0.5 },
    { -0.5,  0.5 },
  };

  // Build the vertex buffer data. Most of this is per-sprite, but the
  // corner data is per-vertex and independent of the sprite.
  std::vector<Attributes> vertices(12);
  for (int i = 0; i < 6; i++) {
    vertices[i].corner[0] = corners[i % 6][0];
    vertices[i].corner[1] = corners[i % 6][1];
    vertices[i].position[0] = 0.0;
    vertices[i].position[1] = 0.0;
    vertices[i].rotation = rotation;
    vertices[i].scale = 0.5;
  }

  glBufferData(GL_ARRAY_BUFFER, sizeof(Attributes) * vertices.size(), vertices.data(), GL_STREAM_DRAW);
  
  // Tell the shader program where to find each of the input variables
  // ("attributes") in its vertex shader input. They're all coming
  // from the same vertex buffer, but they're different slices of that
  // data. The API is flexible enough to allow us to have a start
  // position and stride within the vertex buffer. This allows us to
  // bind the position and rotation inside the struct.
  auto loc_corner = glGetAttribLocation(self->shader->id, "a_corner");
  auto loc_position = glGetAttribLocation(self->shader->id, "a_position");
  auto loc_rotation = glGetAttribLocation(self->shader->id, "a_rotation");
  auto loc_scale = glGetAttribLocation(self->shader->id, "a_scale");
  glVertexAttribPointer(loc_corner, 2, GL_FLOAT, GL_FALSE, sizeof(Attributes),
                        reinterpret_cast<GLvoid*>(offsetof(Attributes, corner)));
  glVertexAttribPointer(loc_position, 2, GL_FLOAT, GL_FALSE, sizeof(Attributes),
                        reinterpret_cast<GLvoid*>(offsetof(Attributes, position)));
  glVertexAttribPointer(loc_rotation, 1, GL_FLOAT, GL_FALSE, sizeof(Attributes),
                        reinterpret_cast<GLvoid*>(offsetof(Attributes, rotation)));
  glVertexAttribPointer(loc_scale, 1, GL_FLOAT, GL_FALSE, sizeof(Attributes),
                        reinterpret_cast<GLvoid*>(offsetof(Attributes, scale)));
  GLERRORS("glVertexAttribPointer");

  // Run the shader program. Enable the vertex attribs just while
  // running this program. Which ones are enabled is global state, and
  // we don't want to interfere with any other shader programs we want
  // to run elsewhere.
  glEnableVertexAttribArray(loc_corner);
  glEnableVertexAttribArray(loc_position);
  glEnableVertexAttribArray(loc_rotation);
  glEnableVertexAttribArray(loc_scale);
  glDrawArrays(GL_TRIANGLES, 0, vertices.size());
  glDisableVertexAttribArray(loc_scale);
  glDisableVertexAttribArray(loc_rotation);
  glDisableVertexAttribArray(loc_position);
  glDisableVertexAttribArray(loc_corner);
  GLERRORS("draw arrays");

  // In double buffering, all the drawing was to a screen buffer that
  // will now be displayed with this command:
  SDL_GL_SwapWindow(self->window);
}


// Vertex buffer stores the data needed to run the shader program

VertexBuffer::VertexBuffer() {
  glGenBuffers(1, &id);
  GLERRORS("VertexBuffer()");
}

VertexBuffer::~VertexBuffer() {
  glDeleteBuffers(1, &id);
}


// Shader program for drawing sprites

GLchar vertex_shader[] =
  "uniform vec2 u_camera;\n"
  "attribute vec2 a_corner;\n"
  "attribute vec2 a_position;\n"
  "attribute float a_rotation;\n"
  "attribute float a_scale;\n"
  "varying vec2 loc;\n"
  "\n"
  "void main() {\n"
  "  mat2 rot = mat2(cos(a_rotation), sin(a_rotation), -sin(a_rotation), cos(a_rotation));\n"
  "  gl_Position = vec4(a_corner * rot * a_scale + a_position - u_camera, 0.0, 1.0);\n"
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
  "  gl_FragColor = texture2D(u_texture, loc + vec2(0.5,0.5));\n"
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

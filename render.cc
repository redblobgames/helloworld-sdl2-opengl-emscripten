// From http://www.redblobgames.com/x/1535-opengl-emscripten/
// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "render.h"
#include <SDL2/SDL.h>

#ifdef __APPLE__
// NOTE(amitp): Mac doesn't have OpenGL ES headers, and the path to GL
// headers is different.
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <SDL2/SDL_opengles2.h>
#endif


#include <iostream>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480


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
  float position[2];
  float rotation;
};

struct RendererImpl {
  SDL_Window* window;
  SDL_GLContext context;
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
  
  glClearColor(0.5, 0.0, 0.0, 1.0);

  self->shader = std::unique_ptr<ShaderProgram>(new ShaderProgram);
  self->vbo = std::unique_ptr<VertexBuffer>(new VertexBuffer);
}

Renderer::~Renderer() {
  SDL_GL_DeleteContext(self->context);
  SDL_DestroyWindow(self->window);
  SDL_Quit();
  GLERRORS("~Renderer");
}


void Renderer::Render() {
  glClear(GL_COLOR_BUFFER_BIT);
  GLERRORS("Clear");
  
  glUseProgram(self->shader->id);
  GLERRORS("useProgram");

  // The data for the vertex shader will be in a vertex buffer
  // ("VBO"). I'm going to use a single vertex buffer for all the
  // parameters, and I'm going to fill it each frame.
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo->id);
  const Attributes triangle[3] = {
    { { -0.5,  0.5 }, 0.0 },
    { {  0.5,  0.5 }, 0.0 },
    { {  0.5, -0.5 }, 0.0 },
    //    { { -0.5, -0.5 }, 0.0 },
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STREAM_DRAW);
  
  // Tell the shader program where to find each of the input variables
  // ("attributes") in its vertex shader input. They're all coming
  // from the same vertex buffer, but they're different slices of that
  // data. The API is flexible enough to allow us to have a start
  // position and stride within the vertex buffer. This allows us to
  // bind the position and rotation inside the struct.
  auto loc_position = glGetAttribLocation(self->shader->id, "a_position");
  auto loc_rotation = glGetAttribLocation(self->shader->id, "a_rotation");
  glVertexAttribPointer(loc_position, 2, GL_FLOAT, GL_FALSE, sizeof(triangle[0]), reinterpret_cast<GLvoid*>(offsetof(Attributes, position)));
  glVertexAttribPointer(loc_rotation, 1, GL_FLOAT, GL_FALSE, sizeof(triangle[0]), reinterpret_cast<GLvoid*>(offsetof(Attributes, rotation)));
  GLERRORS("glVertexAttribPointer");

  // Run the shader program. Enable the vertex attribs just while
  // running this program. Which ones are enabled is global state, and
  // we don't want to interfere with any other shader programs we want
  // to run elsewhere.
  glEnableVertexAttribArray(loc_position);
  glEnableVertexAttribArray(loc_rotation);
  glDrawArrays(GL_TRIANGLES, 0, 3 * 1);
  glDisableVertexAttribArray(loc_rotation);
  glDisableVertexAttribArray(loc_position);
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
  "attribute vec2 a_position;\n"
  "attribute float a_rotation;\n"
  "\n"
  "void main() {\n"
  "  gl_Position = vec4(a_position, sin(a_rotation), 1.0);\n"
  "}\n";

GLchar fragment_shader[] =
#ifdef __EMSCRIPTEN__
  // NOTE(amitp): WebGL requires this but OpenGL 2.1 disallows this, I think.
  "precision mediump float;\n"
  "\n"
#endif
  "void main() {\n"
  "  gl_FragColor = vec4(1.0,0.0,1.0,1.0);\n"
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

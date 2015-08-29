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
  GLuint program_id;
  GLuint loc_position;
  GLuint loc_rotation;
  
  ShaderProgram();
  ~ShaderProgram();
  GLuint LoadShader(GLenum type, const GLchar* source);
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
  
  glUseProgram(self->shader->program_id);
  GLERRORS("useProgram");

  // The data for the vertex shader will be in a vertex buffer
  // ("VBO"). I'm going to use a single vertex buffer for all the
  // parameters, and I'm going to fill it each frame.
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo->id);
  const uint32_t points = 4;
  const uint32_t floatsPerPoint = 3;
  const GLfloat square[points * floatsPerPoint] =
    { -0.5, 0.5, 0.0, 0.5, 0.5, 0.0, 0.5, -0.5, 0.0, -0.5, -0.5, 0.0 };
  uint32_t sizeInBytes = sizeof(square);

  glBufferData(GL_ARRAY_BUFFER, sizeInBytes, square, GL_STREAM_DRAW);
  
  // Tell the shader program where to find each of the input variables
  // ("attributes") in its vertex shader input. They're all coming
  // from the same vertex buffer, but they're different slices of that
  // data. The API is flexible enough to allow us to have a start
  // position and stride within the vertex buffer. That allows us to
  // use an array of struct { float pos[2]; float rotation; } and then
  // bind pos and rotation to separate vertex shader attributes.
  glVertexAttribPointer(self->shader->loc_position, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
  glVertexAttribPointer(self->shader->loc_rotation, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat) /* should be size of struct */, (GLvoid*)(2*sizeof(GLfloat)));
  GLERRORS("glVertexAttribPointer");

  // Run the shader program. Enable the vertex attribs just while
  // running this program. Which ones are enabled is global state, and
  // we don't want to interfere with any other shader programs we want
  // to run elsewhere.
  glEnableVertexAttribArray(self->shader->loc_position);
  glEnableVertexAttribArray(self->shader->loc_rotation);
  glDrawArrays(GL_TRIANGLES, 0, 3 * 1);
  glDisableVertexAttribArray(self->shader->loc_rotation);
  glDisableVertexAttribArray(self->shader->loc_position);
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
  auto vshader_id = LoadShader(GL_VERTEX_SHADER, vertex_shader);
  if (vshader_id == 0) { SDLFAIL("LoadShader(vertex)"); }
  auto fshader_id = LoadShader(GL_FRAGMENT_SHADER, fragment_shader);
  if (fshader_id == 0) { SDLFAIL("LoadShader(fragment)"); }

  program_id = glCreateProgram();
  if (program_id == 0) { SDLFAIL("glCreateProgram"); }

  glAttachShader(program_id, vshader_id);
  glAttachShader(program_id, fshader_id);
  glLinkProgram(program_id);
  
  GLint link_status;
  glGetProgramiv(program_id, GL_LINK_STATUS, &link_status);
  if (!link_status) {
    GLint log_length;
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
    GLchar log[1024];
    glGetProgramInfoLog(program_id, 1024, nullptr, log);
    std::cerr << log << std::endl;
    SDLFAIL("link shaders");
  }

  glDeleteShader(vshader_id);
  glDeleteShader(fshader_id);
  
  loc_position = glGetAttribLocation(program_id, "a_position");
  loc_rotation = glGetAttribLocation(program_id, "a_rotation");
}

ShaderProgram::~ShaderProgram() {
  if (program_id != 0) { glDeleteProgram(program_id); }
}

GLuint ShaderProgram::LoadShader(GLenum type, const GLchar* source) {
  GLuint id = glCreateShader(type);
  if (id == 0) { SDLFAIL("load shader"); }
           
  glShaderSource(id, 1, &source, nullptr);
  glCompileShader(id);

  GLint compile_status;
  glGetShaderiv(id, GL_COMPILE_STATUS, &compile_status);
  if (!compile_status) {
    GLint log_length;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
    GLchar log[1024];
    glGetShaderInfoLog(id, 1024, nullptr, log);
    std::cerr << log << std::endl;
    SDLFAIL("compile shader");
  }

  return id;
}

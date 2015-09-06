// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "glwrappers.h"
#include "common.h"

#include <iostream>


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


ShaderProgram::ShaderProgram(const char* vertex_shader, const char* fragment_shader) {
  id = glCreateProgram();
  if (id == 0) { FAIL("glCreateProgram"); }

#ifdef __EMSCRIPTEN__
  // WebGL requires precision specifiers but OpenGL 2.1 disallows
  // them, so I define the shader without it and then add it here.
  std::string new_fragment_shader = "precision mediump float;\n";
  new_fragment_shader += fragment_shader;
  fragment_shader = new_fragment_shader.c_str();
#endif
  
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
    FAIL("link shaders");
  }
}

ShaderProgram::~ShaderProgram() {
  glDeleteProgram(id);
}

void ShaderProgram::AttachShader(GLenum type, const GLchar* source) {
  GLuint shader_id = glCreateShader(type);
  if (shader_id == 0) { FAIL("load shader"); }
           
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
    FAIL("compile shader");
  }

  glAttachShader(id, shader_id);
  glDeleteShader(shader_id);
  GLERRORS("AttachShader()");
}


Texture::Texture(SDL_Surface* surface) {
  glGenTextures(1, &id);
  if (surface != nullptr) {
    CopyFrom(surface);
  }
}

void Texture::CopyFrom(SDL_Surface* surface) {
  glBindTexture(GL_TEXTURE_2D, id);
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
  GLERRORS("Texture creation");
}

Texture::~Texture() {
  glDeleteTextures(1, &id);
}


VertexBuffer::VertexBuffer() {
  glGenBuffers(1, &id);
}

VertexBuffer::~VertexBuffer() {
  glDeleteBuffers(1, &id);
}


GlContext::GlContext(SDL_Window* window) {
  id = SDL_GL_CreateContext(window);
  if (id == nullptr) { FAIL("SDL_GL_CreateContext"); }
}

GlContext::~GlContext() {
  SDL_GL_DeleteContext(id);
}

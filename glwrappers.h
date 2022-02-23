// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

/** Convenience functions, plus RAII wrappers for SDL and GL objects that
 * come in construct/destroy pairs.
 */

#ifndef GLWRAPPERS_H
#define GLWRAPPERS_H

#define GL_GLEXT_PROTOTYPES
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

#include "common.h"

// Check for any OpenGL errors and print them
void GLERRORS(const char* label);


SDL_Surface* CreateRGBASurface(int width, int height);


struct ShaderProgram: nocopy {
  GLuint id;
  ShaderProgram(const char* vertex_shader, const char* fragment_shader);
  ~ShaderProgram();
  
private:
  void AttachShader(GLenum type, const GLchar* source);
};


struct Texture: nocopy {
  GLuint id;
  Texture(SDL_Surface* surface = nullptr);
  ~Texture();
  void CopyFromPixels(int width, int height, GLenum format, void* pixels);
  void CopyFromSurface(SDL_Surface* surface);
};


struct VertexBuffer: nocopy {
  GLuint id;
  VertexBuffer();
  ~VertexBuffer();
};


struct GlContext: nocopy {
  SDL_GLContext id;
  GlContext(SDL_Window* window);
  ~GlContext();
};


#endif

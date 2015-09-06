// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

/** Wrappers for SDL and GL objects that come in construct/destroy pairs,
 * using C++ RAII.
 */

#ifndef GLWRAPPERS_H
#define GLWRAPPERS_H

// NOTE(amitp): Mac doesn't have OpenGL ES headers, and the path to GL
// headers is different. This header file handles the
// platform-specific paths.

#include <SDL2/SDL.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <SDL2/SDL_opengles2.h>
#endif

// Check for any OpenGL errors and print them
void GLERRORS(const char* label);

struct ShaderProgram {
  GLuint id;
  ShaderProgram(const char* vertex_shader, const char* fragment_shader);
  ~ShaderProgram();
  
private:
  void AttachShader(GLenum type, const GLchar* source);
};


struct Texture {
  GLuint id;
  Texture(SDL_Surface* surface = nullptr);
  ~Texture();
  void CopyFrom(SDL_Surface* surface);
};


struct VertexBuffer {
  GLuint id;
  VertexBuffer();
  ~VertexBuffer();
};


struct GlContext {
  SDL_GLContext id;
  GlContext(SDL_Window* window);
  ~GlContext();
};


#endif

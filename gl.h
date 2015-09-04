// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef GL_H
#define GL_H

// NOTE(amitp): Mac doesn't have OpenGL ES headers, and the path to GL
// headers is different. This header file handles the
// platform-specific paths.

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <SDL2/SDL_opengles2.h>
#endif

void GLERRORS(const char* name);

#endif

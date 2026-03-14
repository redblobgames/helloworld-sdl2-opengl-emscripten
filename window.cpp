// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "window.h"

#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "glwrappers.h"

#include <vector>


// NOTE: I need this implementation to be in a cpp file and not an h
// file so that the compiler knows where to put the vtable
IRenderLayer::~IRenderLayer() {}


struct WindowImpl {
    SDL_Window* window;
    bool context_initialized;
    GlContext context;
    const char* glsl_version;

    std::vector<IRenderLayer*> layers;
  
    WindowImpl(const char* glsl_version, SDL_Window* window_);
    ~WindowImpl();
};

int Window::FRAME = 0;

Window::Window(int width_, int height_)
    :visible(true), width(width_), height(height_) {
    // Decide GL+GLSL versions -- code from Dear ImGui example,
    // modified to make Mac build use OpenGL 1.2 instead of 3.2 so
    // that I can use my old shaders
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 1.2 + GLSL 120
    const char* glsl_version = "#version 120";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    self.reset(new WindowImpl(glsl_version,
                              SDL_CreateWindow("Hello World",
                                               SDL_WINDOWPOS_UNDEFINED,
                                               SDL_WINDOWPOS_UNDEFINED,
                                               width,
                                               height,
                                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
                                              )));
    HandleResize();
}

Window::~Window() {}


void Window::AddLayer(IRenderLayer* layer) {
    self->layers.push_back(layer);
}


void Window::HandleResize() {
    self->context_initialized = false;
    SDL_GL_GetDrawableSize(self->window, &width, &height);
    glViewport(0, 0, width, height);
}


void Window::Render() {
    if (visible) {
        glClear(GL_COLOR_BUFFER_BIT);
        for (auto layer : self->layers) {
            layer->Render(self->window, !self->context_initialized);
        }
        self->context_initialized = true;
        SDL_GL_SwapWindow(self->window);
        FRAME++;
    }
}


void Window::ProcessEvent(SDL_Event* event) {
    if (event->type == SDL_WINDOWEVENT) {
        switch (event->window.event) {
         case SDL_WINDOWEVENT_SHOWN: { visible = true; break; }
         case SDL_WINDOWEVENT_HIDDEN: { visible = false; break; }
         case SDL_WINDOWEVENT_SIZE_CHANGED: { HandleResize(); break; }
        }
    }

    for (auto layer : self->layers) {
        layer->ProcessEvent(event);
    }
}


SDL_Window* Window::GetSdlWindow() {
    return self->window;
}

GlContext* Window::GetGlContext() {
    return &self->context;
}

const char* Window::GetGlslVersion() {
    return self->glsl_version;
}

WindowImpl::WindowImpl(const char* glsl_version_, SDL_Window* window_)
    :window(window_), context_initialized(false), context(window), glsl_version(glsl_version_)
{
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    glClearColor(1.0, 1.0, 1.0, 1.0);
}

WindowImpl::~WindowImpl() {
    SDL_DestroyWindow(window);
}

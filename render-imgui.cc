// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "render-imgui.h"
#include "window.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "glwrappers.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#include "imgui/imgui.h"
#pragma clang diagnostic pop

#include <vector>


struct RenderImGuiImpl {
  ShaderProgram shader;
  Texture font_texture;
  VertexBuffer vbo, ibo;

  uint32_t timestamp;
  bool mouse_button_pressed;
  
  GLint loc_u_screensize;
  GLint loc_u_texture;
  GLint loc_a_xy;
  GLint loc_a_uv;
  GLint loc_a_rgba;
  
  RenderImGuiImpl();
};


namespace {
  const char* Wrap_SDL_GetClipboardText() { return SDL_GetClipboardText(); }
  void Wrap_SDL_SetClipboardText(const char* s) { SDL_SetClipboardText(s); }
}


RenderImGui::RenderImGui(): self(new RenderImGuiImpl) {
  // Code from the imgui sdl example
  ImGuiIO& io = ImGui::GetIO();
  io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
  io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
  io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
  io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
  io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
  io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
  io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
  io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
  io.KeyMap[ImGuiKey_A] = SDLK_a;
  io.KeyMap[ImGuiKey_C] = SDLK_c;
  io.KeyMap[ImGuiKey_V] = SDLK_v;
  io.KeyMap[ImGuiKey_X] = SDLK_x;
  io.KeyMap[ImGuiKey_Y] = SDLK_y;
  io.KeyMap[ImGuiKey_Z] = SDLK_z;
  io.SetClipboardTextFn = Wrap_SDL_SetClipboardText;
  io.GetClipboardTextFn = Wrap_SDL_GetClipboardText;
}


RenderImGui::~RenderImGui() {
  ImGui::Shutdown();
}


// Shader program for drawing a single quad
namespace {
  GLchar vertex_shader[] = R"(
  uniform vec2 u_screensize;
  attribute vec2 a_xy;
  attribute vec2 a_uv;
  attribute vec4 a_rgba;
  varying vec2 v_uv;
  varying vec4 v_rgba;
  void main() {
    v_uv = a_uv;
    v_rgba = a_rgba;
    gl_Position = vec4(vec2(2.0, -2.0) * (a_xy/u_screensize) + vec2(-1.0, 1.0), 0.0, 1.0);
  }
)";

  GLchar fragment_shader[] = R"(
  uniform sampler2D u_texture;
  varying vec2 v_uv;
  varying vec4 v_rgba;
  void main() {
    gl_FragColor = v_rgba * texture2D(u_texture, v_uv);
  }
)";
}


RenderImGuiImpl::RenderImGuiImpl()
  : shader(vertex_shader, fragment_shader),
    timestamp(SDL_GetTicks()),
    mouse_button_pressed(false)
{
  loc_u_screensize = glGetUniformLocation(shader.id, "u_screensize");
  loc_u_texture = glGetUniformLocation(shader.id, "u_texture");
  loc_a_xy = glGetAttribLocation(shader.id, "a_xy");
  loc_a_uv = glGetAttribLocation(shader.id, "a_uv");
  loc_a_rgba = glGetAttribLocation(shader.id, "a_rgba");

  ImGuiIO& io = ImGui::GetIO();
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  font_texture.CopyFromPixels(width, height, GL_RGBA, pixels);
  io.Fonts->TexID = (void *)(intptr_t)font_texture.id;
  io.Fonts->ClearInputData();
  io.Fonts->ClearTexData();
}


void RenderImGui::Render(SDL_Window* window, bool reset) {
  ImGuiIO& io = ImGui::GetIO();
  int width, height, fb_width, fb_height;
  SDL_GetWindowSize(window, &width, &height);
  SDL_GL_GetDrawableSize(window, &fb_width, &fb_height);
  io.DisplaySize.x = width;
  io.DisplaySize.y = height;
  io.DisplayFramebufferScale.x = float(fb_width) / width;
  io.DisplayFramebufferScale.y = float(fb_height) / height;

  // Pointer handling, in addition to event handler:
  if ((SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS) == 0) {
    io.MousePos = ImVec2(-1, -1);
  }
  Uint32 mouseMask = SDL_GetMouseState(nullptr, nullptr);
  io.MouseDown[0] = self->mouse_button_pressed || (mouseMask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
  self->mouse_button_pressed = false;
  SDL_ShowCursor(io.MouseDrawCursor ? 0 : 1);
  
  // Keyboard handling, in addition to event handler:
  io.KeyShift = (SDL_GetModState() & KMOD_SHIFT) != 0;
  io.KeyCtrl = (SDL_GetModState() & KMOD_CTRL) != 0;
  io.KeyAlt = (SDL_GetModState() & KMOD_ALT) != 0;

  // Time since last frame:
  uint32_t new_timestamp = SDL_GetTicks();
  io.DeltaTime = (new_timestamp - self->timestamp) / 1000.0;
  self->timestamp = new_timestamp;
  
  ImGui::NewFrame();
  // TODO: GUI should be defined elsewhere
  ImGui::Text("Hello");
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  static bool g_show_test_window = true;
  ImGui::ShowTestWindow(&g_show_test_window);

  ImGui::Render();
  ImDrawData* draw_data = ImGui::GetDrawData();
  draw_data->ScaleClipRects(io.DisplayFramebufferScale);
  
  glUseProgram(self->shader.id);
  glEnable(GL_SCISSOR_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glActiveTexture(GL_TEXTURE0);

  GLfloat screensize[2] = {1.0f*width, 1.0f*height};
  glUniform2fv(self->loc_u_screensize, 1, screensize);
  glUniform1i(self->loc_u_texture, 0);
    
  glBindBuffer(GL_ARRAY_BUFFER, self->vbo.id);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->ibo.id);
    
  glVertexAttribPointer(self->loc_a_xy,
                        2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                        reinterpret_cast<GLvoid*>(offsetof(ImDrawVert, pos)));
  glVertexAttribPointer(self->loc_a_uv,
                        2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                        reinterpret_cast<GLvoid*>(offsetof(ImDrawVert, uv)));
  glVertexAttribPointer(self->loc_a_rgba,
                        4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
                        reinterpret_cast<GLvoid*>(offsetof(ImDrawVert, col)));

  glEnableVertexAttribArray(self->loc_a_xy);
  glEnableVertexAttribArray(self->loc_a_uv);
  glEnableVertexAttribArray(self->loc_a_rgba);
  
  for (int n = 0; n < draw_data->CmdListsCount; n++) {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    const ImDrawIdx* idx_buffer_offset = 0;

    glBufferData(GL_ARRAY_BUFFER,
                 cmd_list->VtxBuffer.size() * sizeof(ImDrawVert),
                 &cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx),
                 &cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);

    for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++) {
      if (pcmd->UserCallback) {
        pcmd->UserCallback(cmd_list, pcmd);
      } else {
        glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
        glScissor(int(pcmd->ClipRect.x),
                  fb_height - int(pcmd->ClipRect.w),
                  int(pcmd->ClipRect.z - pcmd->ClipRect.x),
                  int(pcmd->ClipRect.w - pcmd->ClipRect.y));
        glDrawElements(GL_TRIANGLES, pcmd->ElemCount, GL_UNSIGNED_SHORT, idx_buffer_offset);
      }
      idx_buffer_offset += pcmd->ElemCount;
    }
  }
  
  glDisableVertexAttribArray(self->loc_a_rgba);
  glDisableVertexAttribArray(self->loc_a_uv);
  glDisableVertexAttribArray(self->loc_a_xy);
  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
}


void RenderImGui::ProcessEvent(SDL_Event* event) {
  ImGuiIO& io = ImGui::GetIO();
  switch (event->type) {
  case SDL_MOUSEMOTION: {
    io.MousePos.x = event->motion.x;
    io.MousePos.y = event->motion.y;
    break;
  }
  case SDL_MOUSEBUTTONDOWN: {
    // NOTE: I'm only handling left button here because that's all
    // ImGui uses for its standard widgets. It'd be simpelr to set
    // io.MouseDown to true on SDL_MOUSEBUTTONDOWN and false on
    // SDL_MOUSEBUTTONUP. However, because ImGui is polling instead of
    // event-driven, it will miss a quick click. The workaround is to
    // set mouse_button_pressed if it's ever clicked during the frame, and
    // *also* check if the mouse is down at the end of the frame.
    if (event->button.button == SDL_BUTTON_LEFT) {
      self->mouse_button_pressed = true;
    }
    break;
  }
  case SDL_MOUSEWHEEL: {
    io.MouseWheel += event->wheel.y;
    break;
  }
  case SDL_TEXTINPUT: {
    io.AddInputCharactersUTF8(event->text.text);
    break;
  }
  case SDL_KEYDOWN: case SDL_KEYUP: {
    int key = event->key.keysym.sym & ~SDLK_SCANCODE_MASK;
    io.KeysDown[key] = (event->type == SDL_KEYDOWN);
    break;
  }
  }
}

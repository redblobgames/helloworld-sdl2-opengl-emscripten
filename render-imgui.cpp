// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#include "render-imgui.h"

#include <SDL.h>
#include <SDL_image.h>
#include "SDL_scancode.h"
#include "glwrappers.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <vector>

#define SHOW_INPUT 1

struct RenderImGuiImpl {
    Window* window;
    Texture font_texture;
    VertexBuffer vbo, ibo;

    uint32_t timestamp;
    bool mouse_button_pressed;
    bool enter_pressed;

    SDL_Scancode most_recent_scancode = SDL_SCANCODE_UNKNOWN;
    SDL_Keycode most_recent_keycode;

    RenderImGuiImpl(Window* window);
    ~RenderImGuiImpl();
};


namespace {
    const char* Wrap_SDL_GetClipboardText(void*) { return SDL_GetClipboardText(); }
    void Wrap_SDL_SetClipboardText(void*, const char* s) { SDL_SetClipboardText(s); }
}


RenderImGui::RenderImGui(Window* window): self(new RenderImGuiImpl(window)) {
}


RenderImGui::~RenderImGui() {
}


RenderImGuiImpl::RenderImGuiImpl(Window* window_)
    : window(window_),
      timestamp(SDL_GetTicks()),
      mouse_button_pressed(false),
      enter_pressed(false)
{
    // Some of this code is from imgui/examples/example_sdl2_opengl3/main.cpp

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    GLERRORS("ImGui::CreateContext");
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0); // NOTE: for non-Apple, non-Emscripten, non-Android, non-Wayland
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends

    ImGui_ImplSDL2_InitForOpenGL(window->GetSdlWindow(), window->GetGlContext());
    GLERRORS("ImGui_ImplSDL2_InitForOpenGL");
    ImGui_ImplOpenGL3_Init(window->GetGlslVersion());
    GLERRORS("ImGui_ImplOpenGL3_Init");

    // Load fonts
    io.Fonts->AddFontFromFileTTF("imgui/misc/fonts/DroidSans.ttf", 15.0);
    GLERRORS("ImGui AddFontFromFileTTF");
}


RenderImGuiImpl::~RenderImGuiImpl() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}


void RenderImGui::Render(SDL_Window* /* already in impl */, bool reset) {
    ImGuiIO& io = ImGui::GetIO();

    // Time since last frame:
    uint32_t new_timestamp = SDL_GetTicks();
    io.DeltaTime = (new_timestamp - self->timestamp) / 1000.0f;
    self->timestamp = new_timestamp;
  
    ImGui_ImplOpenGL3_NewFrame();
    GLERRORS("ImGui_ImplOpenGL3_NewFrame");
    ImGui_ImplSDL2_NewFrame();
    GLERRORS("ImGui_ImplSDL2_NewFrame");
    ImGui::NewFrame();
    GLERRORS("ImGui::NewFrame");
  
    ImGui::SetNextWindowPos(ImVec2(0, 40));
    ImGui::SetNextWindowSize(ImVec2(400, 70 + 50 * SHOW_INPUT), ImGuiCond_FirstUseEver);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5f));
    ImGui::Begin("Status", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    // TODO: GUI should be defined elsewhere
    ImGui::Text("Hello from SDL %d.%d.%d ImGui %s",
                SDL_MAJOR_VERSION,
                SDL_MINOR_VERSION,
                SDL_PATCHLEVEL,
                IMGUI_VERSION
               );
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

#if SHOW_INPUT
    ImGui::Text("  capturemouse=%d capturekeyboard=%d textinput=%d",
                io.WantCaptureMouse,
                io.WantCaptureKeyboard,
                io.WantTextInput
               );
    // For localization of keyboard layouts, there's a "scan code"
    // indicating the physical position of the key and a "key code"
    // which is the label on that key. For games we often want a scan
    // code, e.g. 0x01a for "W" in "WASD". But to display the label
    // in the UI, we want to show the key name. SDL provides functions
    // to go from scan code to key code to key name. I put this here
    // to test those functions. Locally, it works fine. But *on the web*
    // a key event contains the correct keycode, but you cannot get the
    // keycode outside an event. There is a browser-specific API
    // https://developer.mozilla.org/en-US/docs/Web/API/Keyboard_API
    // that lets you get that information, but only supported on Blink.
    // SDL 2.30 does not appear to use that API.
    if (self->most_recent_scancode != SDL_SCANCODE_UNKNOWN) {
        ImGui::Text("with event: scancode 0x%03x key %d becomes key %d (%s)",
                    self->most_recent_scancode,
                    self->most_recent_keycode,
                    SDL_GetKeyFromScancode(self->most_recent_scancode) & ~SDLK_SCANCODE_MASK,
                    SDL_GetKeyName(self->most_recent_keycode)
                   );
    }
    ImGui::Text("without event: scancode 0x01a becomes key = %d (%s)",
                SDL_GetKeyFromScancode(SDL_Scancode(0x1a)) & ~SDLK_SCANCODE_MASK,
                SDL_GetKeyName(SDL_GetKeyFromScancode(SDL_Scancode(0x1a)))
               );
#endif

    ImGui::End();
    ImGui::PopStyleColor();
  
    // Chat window
    int chat_window_width = 500, chat_window_height = 200;
    static std::vector<std::string> chat_lines { "Welcome to chat. Press Enter to toggle focus." };
    static bool scroll_to_bottom = false;
    ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - chat_window_height));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0;
    style.Colors[ImGuiCol_TitleBg] = ImColor::HSV(0, 0.5f, 0.5f);
    style.Colors[ImGuiCol_TitleBgActive] = ImColor::HSV(0, 0.7f, 0.5f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImColor::HSV(0, 0.3f, 0.5f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImColor::HSV(0, 0.3f, 0.5f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor::HSV(0, 0.5f, 0.8f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImColor::HSV(0, 0.7f, 0.5f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, io.WantTextInput ? 1.0f : 0.7f));
    ImGui::SetNextWindowSize(ImVec2(chat_window_width, chat_window_height), ImGuiCond_FirstUseEver);
    ImGui::Begin("Output", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);
    ImGui::BeginChild("output-area", ImVec2(ImGui::GetContentRegionAvail().x, chat_window_height - 50), false, 0);
    for (auto line : chat_lines) {
        ImVec4 color = ImColor(255, 128, 64);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("[chat]");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextWrapped("%s", line.c_str());
    }
    if (scroll_to_bottom) {
        ImGui::SetScrollHereY();
        scroll_to_bottom = false;
    }
    ImGui::EndChild();
    ImGui::Separator();
    static char input[100] = "";
    ImGui::PushItemWidth(500);

    if (ImGui::InputText("Send", input, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (input[0] != '\0') { chat_lines.push_back(input); }
        scroll_to_bottom = true;
        input[0] = '\0';
    } else if (!io.WantTextInput && self->enter_pressed) {
        ImGui::SetKeyboardFocusHere(-1);
    }
    self->enter_pressed = false;

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

  
    static bool g_show_test_window = true;
    if (g_show_test_window) { ImGui::ShowDemoWindow(&g_show_test_window); }

  
    ImGui::Render();
    GLERRORS("ImGui::Render");
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    GLERRORS("ImGui_ImplOpenGL3_RenderDrawData");
}


void RenderImGui::ProcessEvent(SDL_Event* event) {
    switch (event->type) {
        case SDL_KEYDOWN: case SDL_KEYUP: {
            int key = event->key.keysym.sym & ~SDLK_SCANCODE_MASK;
            self->most_recent_keycode = key;
            self->most_recent_scancode = event->key.keysym.scancode;
            if (event->type == SDL_KEYDOWN && key == SDLK_RETURN) {
                // Used to activate the chat box
                self->enter_pressed = true;
            }
            break;
       }
    }
}

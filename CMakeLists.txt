cmake_minimum_required(VERSION 2.6)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

if (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
  # TODO(amitp): I don't know how to set this properly with add_compile_options
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2 -s USE_SDL_IMAGE=2")
  set(SDL2_LIBRARIES "-s USE_SDL=2")
  set(SDL2_IMAGE_LIBRARIES "-s USE_SDL_IMAGE=2")
else ()
  include(FindPkgConfig)
  pkg_search_module(SDL2 REQUIRED sdl2)
  pkg_search_module(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
endif ()


add_executable(main main.cc glwrappers.cc window.cc atlas.cc font.cc render-sprites.cc render-surface.cc render-imgui.cc imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_demo.cpp)
set_property(TARGET main PROPERTY CXX_STANDARD 11)
target_link_libraries(main ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} ${SDL2IMAGE_LIBRARIES})

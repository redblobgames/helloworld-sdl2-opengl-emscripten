# Makefile with autodependencies and separate output directories
# - $BUILDDIR/ is used for object and other build files
# - $BINDIR/ is used for native binaries
# - assets/ is used for assets needed when running native binaries
# - $WWWDIR/ is used for emscripten output
#
# For native (Mac OS X, Linux) builds, $(BINDIR)/ and assets/ are needed
# For emscripten builds, $(WWWDIR)/ is needed

MODULES = main glwrappers window atlas font render-sprites render-shapes render-surface render-imgui imgui/imgui imgui/imgui_draw imgui/imgui_widgets imgui/imgui_demo
ASSETS = assets/red-blob.png imgui/misc/fonts/DroidSans.ttf

UNAME = $(shell uname -s)
BUILDDIR = build
BINDIR = bin
WWWDIR = www
_MKDIRS := $(shell mkdir -p $(BINDIR) $(WWWDIR) $(BUILDDIR))

COMMONFLAGS = -std=c++11 -MMD -MP -isystem .
LOCALFLAGS = -g -O2 $(COMMONFLAGS) $(shell pkg-config --cflags sdl2)

# Choose the warnings I want, and disable when compiling third party code
NOWARNDIRS = imgui/ stb/
LOCALWARN = -Wall -Wextra -pedantic -Wpointer-arith -Wshadow -Wfloat-conversion -Wfloat-equal -Wno-unused-function -Wno-unused-parameter
# NOTE: also useful but noisy -Wconversion -Wshorten-64-to-32

LOCALLIBS = $(shell sdl2-config --libs) -lSDL2_image
ifeq ($(UNAME),Darwin)
	LOCALLIBS += -Wl,-dead_strip -framework OpenGL
else
	LOCALLIBS += -lGL
endif

EMXX = em++
EMXXFLAGS = $(COMMONFLAGS) -Oz -s USE_SDL=2 --use-preload-plugins -s USE_SDL_IMAGE=2 -s WASM=1
# -s SAFE_HEAP=1 -s ASSERTIONS=2 --profiling  -s DEMANGLE_SUPPORT=1
EMXXLINK = -s TOTAL_MEMORY=50331648

all: $(BINDIR)/main

$(WWWDIR): $(WWWDIR)/index.html $(WWWDIR)/_main.js

$(BINDIR)/main: $(MODULES:%=$(BUILDDIR)/%.o) Makefile
	$(CXX) $(LOCALFLAGS) $(filter %.o,$^) $(LOCALLIBS) -o $@

$(WWWDIR)/index.html: emscripten-shell.html
	cp emscripten-shell.html $(dir $@)index.html

$(WWWDIR)/_main.js: $(MODULES:%=$(BUILDDIR)/%.em.o) $(ASSETS) Makefile
	$(EMXX) $(EMXXFLAGS) $(EMXXLINK) $(filter %.o,$^) $(ASSETS:%=--preload-file %) -o $@

$(BUILDDIR)/%.em.o: %.cpp Makefile
	@mkdir -p $(dir $@)
	@echo $(EMXX) -c $< -o $@
	@$(EMXX) $(EMXXFLAGS) -c $< -o $@

# The $(if ...) uses my warning flags only in WARNDIRS
$(BUILDDIR)/%.o: %.cpp Makefile
	@mkdir -p $(dir $@)
	@echo $(CXX) -c $< -o $@
	@$(CXX) $(LOCALFLAGS) $(if $(filter-out $(NOWARNDIRS),$(dir $<)),$(LOCALWARN)) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)/* $(BINDIR)/* $(WWWDIR)/*

include $(shell find $(BUILDDIR) -name \*.d)

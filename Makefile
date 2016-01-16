# Makefile with autodependencies and separate output directories
# - $BUILDDIR/ is used for object and other build files
# - $LOCALOUTPUT/ is used for native binaries
# - assets/ is used for assets needed when running native binaries
# - $EMXXOUTPUT/ is used for emscripten output
#
# For native (Mac OS X) builds, $LOCALOUTPUT/ and assets/ are needed
# For emscripten builds, $EMXXOUTPUT/ is needed

MODULES = main glwrappers window atlas font render-sprites render-shapes render-surface render-imgui imgui/imgui imgui/imgui_draw imgui/imgui_demo
ASSETS = assets/red-blob.png assets/DroidSans.ttf

UNAME = $(shell uname -s)
BUILDDIR = build
LOCALOUTPUT = bin
EMXXOUTPUT = html

LOCALFLAGS = -std=c++11 -g -O2

# Choose the warnings I want, and disable when compiling third party code
NOWARNDIRS = imgui/ stb/
LOCALWARN = -Wall -Wextra -pedantic -Wpointer-arith -Wshadow -Wfloat-conversion -Wfloat-equal -Wno-unused-function -Wno-unused-parameter
# TODO: why doesn't $(NOWARNDIRS:%=--system-header-prefix=%) help? I
# instead put pragmas into the cc files that include the offending
# headers.
# NOTE: also useful but noisy -Wconversion -Wshorten-64-to-32

LOCALLIBS = $(shell sdl2-config --libs) -lSDL2_image
ifeq ($(UNAME),Darwin)
	LOCALLIBS += -Wl,-dead_strip -framework OpenGL
else
	LOCALLIBS += -lGL
endif

EMXX = em++
EMXXFLAGS = -std=c++11 -Oz -s USE_SDL=2 -s USE_SDL_IMAGE=2
# -s SAFE_HEAP=1 -s ASSERTIONS=2 --profiling  -s DEMANGLE_SUPPORT=1
EMXXLINK = -s TOTAL_MEMORY=50331648

all: $(LOCALOUTPUT)/main GTAGS

GTAGS: $(wildcard *.cc) $(wildcard *.h)
	@[ -r GTAGS ] || gtags --sqlite3
	@global -u

emscripten: $(EMXXOUTPUT)/index.html

$(LOCALOUTPUT)/main: $(MODULES:%=$(BUILDDIR)/%.o) Makefile
	@mkdir -p $(dir $@)
	$(CXX) $(LOCALFLAGS) $(filter %.o,$^) $(LOCALLIBS) -o $@

$(EMXXOUTPUT)/index.html: emscripten-shell.html $(EMXXOUTPUT)/_main.js
	cp emscripten-shell.html $(dir $@)index.html

$(EMXXOUTPUT)/_main.js: $(MODULES:%=$(BUILDDIR)/%.em.o) $(ASSETS) Makefile
	@mkdir -p $(dir $@)
	$(EMXX) $(EMXXFLAGS) $(EMXXLINK) $(filter %.o,$^) $(ASSETS:%=--preload-file %) -o $@

# My makefile assumes *.cc, so I make symlinks for *.cpp files
imgui/%.cc: imgui/%.cpp
	@ln -s $(<F) $@

$(BUILDDIR)/%.em.o: %.cc $(BUILDDIR)/%.o Makefile
	$(EMXX) $(EMXXFLAGS) -c $< -o $@

# The $(if ...) uses my warning flags only in WARNDIRS
$(BUILDDIR)/%.o: %.cc Makefile
	@mkdir -p $(dir $@)
	@echo $(CXX) -c $< -o $@
	@$(CXX) $(LOCALFLAGS) $(if $(filter-out $(NOWARNDIRS),$(dir $<)),$(LOCALWARN)) -MMD -c $< -o $@

include $(shell find $(BUILDDIR) -name \*.d)

clean:
	rm -rf GTAGS GRTAGS GPATH $(BUILDDIR)/* $(LOCALOUTPUT)/* $(EMXXOUTPUT)/*

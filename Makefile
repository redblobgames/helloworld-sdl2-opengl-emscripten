# Makefile with autodependencies and separate output directories
# - $BUILDDIR/ is used for object and other build files
# - $LOCALOUTPUT/ is used for native binaries
# - assets/ is used for assets needed when running native binaries
# - $EMXXOUTPUT/ is used for emscripten output
#
# For native builds, $LOCALOUTPUT/ and assets/ are needed
# For emscripten builds, $EMXXOUTPUT/ is needed

MODULES = main glwrappers window atlas font render-sprites render-surface
ASSETS = assets/red-blob.png assets/share-tech-mono.ttf

BUILDDIR = build
LOCALOUTPUT = bin
EMXXOUTPUT = html

LOCALFLAGS = -std=c++11 -g -O2 -Wall
LOCALINCLUDE = $(shell sdl2-config --cflags)
LOCALLIBS = $(shell sdl2-config --libs) -lSDL2_image -framework OpenGL

EMXX = em++
# --profiling
EMXXFLAGS = -std=c++11 -O2 -s USE_SDL=2 -s USE_SDL_IMAGE=2

all: $(LOCALOUTPUT)/main GTAGS

GTAGS: $(wildcard *.cc) $(wildcard *.h)
	@[ -r GTAGS ] || gtags --sqlite3
	@global -u

emscripten: $(EMXXOUTPUT)/index.html

$(LOCALOUTPUT)/main: $(MODULES:%=$(BUILDDIR)/%.o) Makefile
	@mkdir -p $(dir $@)
	$(CXX) $(LOCALFLAGS) $(filter %.o,$^) $(LOCALLIBS) -o $@

$(EMXXOUTPUT)/index.html: emscripten-shell.html $(EMXXOUTPUT)/main.js
	@mkdir -p $(dir $@)
	cp emscripten-shell.html $(dir $@)index.html

$(EMXXOUTPUT)/main.js: $(MODULES:%=$(BUILDDIR)/%.em.o) $(ASSETS) Makefile
	$(EMXX) $(EMXXFLAGS) $(filter %.o,$^) $(ASSETS:%=--preload-file %) -o $@

$(BUILDDIR)/%.em.o: %.cc $(BUILDDIR)/%.o Makefile
	$(EMXX) $(EMXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.cc Makefile
	@mkdir -p $(dir $@)
	$(CXX) $(LOCALFLAGS) $(LOCALINCLUDE) -MMD -c $< -o $@

include $(wildcard $(BUILDDIR)/*.d)

clean:
	rm -f GTAGS GRTAGS GPATH $(BUILDDIR)/* $(LOCALOUTPUT)/* $(EMXXOUTPUT)/*

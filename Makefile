MODULES = main render
ASSETS = assets/red-blob.png

BUILDDIR = build
OUTPUTDIR = bin

OBJS = $(MODULES:%=$(BUILDDIR)/%.o)

LOCALFLAGS = -std=c++11 -g -O2 -Wall
LOCALINCLUDE = $(shell sdl2-config --cflags)
LOCALLIBS = $(shell sdl2-config --libs) -lSDL2_image -framework OpenGL

EMXX = em++
EMXXFLAGS = -std=c++11 -O2 -s USE_SDL=2 -s USE_SDL_IMAGE=2

all: $(OUTPUTDIR)/main GTAGS

GTAGS: $(wildcard *.cc) $(wildcard *.h)
	@[ -r GTAGS ] || gtags --sqlite3
	@global -u

emscripten: $(OUTPUTDIR)/main.html

$(OUTPUTDIR)/main: $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(LOCALFLAGS) $^ $(LOCALLIBS) -o $@

$(OUTPUTDIR)/main.html: $(OBJS:%.o=%.em.o) $(ASSETS)
	$(EMXX) $(EMXXFLAGS) $(filter %.o,$^) $(ASSETS:%=--preload-file %) -o $@

$(BUILDDIR)/%.em.o: %.cc $(BUILDDIR)/%.o
	$(EMXX) $(EMXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) $(LOCALFLAGS) $(LOCALINCLUDE) -MMD -c $< -o $@

include $(wildcard $(BUILDDIR)/*.d)

clean:
	rm -f GTAGS GRTAGS GPATH $(BUILDDIR)/* $(OUTPUTDIR)/*

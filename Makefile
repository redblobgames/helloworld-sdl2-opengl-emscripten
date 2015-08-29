MODULES = main render

BUILDDIR = build
OUTPUTDIR = bin

OBJS = $(patsubst %,$(BUILDDIR)/%.o,$(MODULES))

CXXFLAGS = -std=c++11 -g -O2 -Wall
INCLUDE = $(shell sdl2-config --cflags)
EMXX = em++
EMXXFLAGS = -std=c++11 -O2 -s USE_SDL=2
LIBS = $(shell sdl2-config --libs) -framework OpenGL

all: $(OUTPUTDIR)/main

GTAGS: $(wildcard *.cc) $(wildcard *.h)
	gtags --sqlite3

emscripten: $(OUTPUTDIR)/main.html

$(OUTPUTDIR)/main: $(OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

$(OUTPUTDIR)/main.html: $(patsubst %.o,%.em.o,$(OBJS))
	$(EMXX) $(EMXXFLAGS) $(INCLUDE) $^ -o $@

$(BUILDDIR)/%.em.o: %.cc $(BUILDDIR)/%.o
	$(EMXX) $(EMXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.cc
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -c $< -o $@

include $(wildcard $(BUILDDIR)/*.d)

clean:
	rm -f $(BUILDDIR)/* $(OUTPUTDIR)/*

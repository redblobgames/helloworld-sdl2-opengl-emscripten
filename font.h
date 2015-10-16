// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

/** Load fonts so that they can be rendered onto an SDL_Surface. */

#ifndef FONT_H
#define FONT_H

#include <memory>

struct SDL_Surface;
struct FontImpl;

class Font {
public:
  // The font will be ceil(ptsize) pixels high. Use xadvance_adjust to
  // increase or decrease spacing between characters.
  Font(const char* filename, float ptsize, float xadvance_adjust=0.0);
  ~Font();

  // Draw text at x,y being the baseline. Drawing can happen both
  // above and below the baseline. For example, a font with ptsize=30
  // and y=100 might draw starting from y=80 and ending at y=120.
  void Draw(SDL_Surface* surface, int x, int y, const char* text) const;

  int Height() const;
  int Baseline() const;
  int Width(const char* text) const;
private:
  std::unique_ptr<FontImpl> self;
};

#endif

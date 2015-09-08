// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

/** Load fonts into an SDL_Surface, then render strings to another
 * SDL_Surface 
 */

#ifndef FONT_H
#define FONT_H

#include <memory>

struct SDL_Surface;
struct FontImpl;

class Font {
public:
  Font(const char* filename, float ptsize);
  ~Font();
  void Draw(SDL_Surface* surface, int x, int y, const char* text) const;
  
private:
  std::unique_ptr<FontImpl> self;
};

#endif

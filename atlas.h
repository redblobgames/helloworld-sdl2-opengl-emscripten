// Copyright 2015 Red Blob Games <redblobgames@gmail.com>
// License: Apache v2.0 <http://www.apache.org/licenses/LICENSE-2.0.html>

#ifndef ATLAS_H
#define ATLAS_H

#include <memory>

struct SDL_Surface;
struct AtlasImpl;

// World coordinates sized 1x1 for sprites and __x1 for fonts. 
struct SpriteLocation {
  float x0, y0, x1, y1; // Corners in world coordinates
  float s0, t0, s1, t1; // Corners in texture coordinates
  float xadvance;       // Only for font glyphs
};

class Atlas {
public:
  Atlas();
  ~Atlas();

  // An Atlas contains either images or a font.
  int LoadImage(const char* filename);
  void LoadFont(const char* filename, float ptsize);

  // Call this after images or a font is loaded.
  SDL_Surface* GetSurface();

  // Get image data for a given sprite id
  const SpriteLocation& GetLocation(int id) const;
  
private:
  std::unique_ptr<AtlasImpl> self;
};

#endif

#ifndef TerrainLayer_h
#define TerrainLayer_h

#include "Bitmap.h"

class Terrain;
class TerrainRegion;

class TerrainLayer
{
  friend class Terrain;
  friend class TerrainRegion;

  public:
    TerrainLayer()
      : tile_texture(0),
        detail_texture(0),
        min_height(0),
        max_height(-1) {}

    ~TerrainLayer() {}

    int operator <(const TerrainLayer& t) const { return min_height < t.min_height; }
    int operator <=(const TerrainLayer& t) const { return min_height <= t.min_height; }
    int operator ==(const TerrainLayer& t) const { return min_height == t.min_height; }

    // accessors:
    std::string GetTileName() const { return tile_name; }
    std::string GetDetailName() const { return detail_name; }
    Bitmap* GetTileTexture() const { return tile_texture; }
    Bitmap* GetDetailTexture() const { return detail_texture; }
    double GetMinHeight() const { return min_height; }
    double GetMaxHeight() const { return max_height; }

  private:
    std::string tile_name;
    std::string detail_name;
    Bitmap* tile_texture;
    Bitmap* detail_texture;
    double min_height;
    double max_height;
};

#endif TerrainLayer_h

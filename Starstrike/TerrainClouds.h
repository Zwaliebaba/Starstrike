#ifndef TerrainClouds_h
#define TerrainClouds_h

#include "Geometry.h"
#include "Graphic.h"
#include "List.h"
#include "Polygon.h"

class Terrain;
class TerrainRegion;

class TerrainClouds : public Graphic
{
  public:
    TerrainClouds(Terrain* terr, int type);
    ~TerrainClouds() override;

    void Render(Video* video, DWORD flags) override;
    void Update() override;

    // accessors:
    int CollidesWith(Graphic& o) override { return 0; }
    bool Luminous() const override { return true; }
    bool Translucent() const override { return true; }

    void Illuminate(Color ambient, List<Light>& lights);

  protected:
    void BuildClouds();

    Terrain* terrain;
    Vec3* mverts;
    VertexSet* verts;
    Poly* polys;
    Material mtl_cloud;
    Material mtl_shade;

    int type;
    int nbanks;
    int nverts;
    int npolys;
};

#endif TerrainClouds_h

#ifndef Sky_h
#define Sky_h

#include "Geometry.h"
#include "Solid.h"

class StarSystem;

class Stars : public Graphic
{
  public:
    Stars(int nstars);
    ~Stars() override;

    virtual void Illuminate(double scale);
    void Render(Video* video, DWORD flags) override;

  protected:
    VertexSet* vset;
    Color* colors;
};

class Dust : public Graphic
{
  public:
    Dust(int ndust, bool bright = false);
    ~Dust() override;

    void Render(Video* video, DWORD flags) override;
    virtual void Reset(const Point& ref);
    virtual void ExecFrame(double factor, const Point& ref);

    void Hide() override;
    void Show() override;

  protected:
    bool really_hidden;
    bool bright;
    VertexSet* vset;
};

class PlanetRep : public Solid
{
  public:
    PlanetRep(std::string_view img_west, std::string_view img_glow, double rad, const Vec3& pos, double tscale = 1,
              std::string_view rngname = {}, double minrad = 0, double maxrad = 0, Color atmos = Color::Black,
              std::string_view img_gloss = {});
    ~PlanetRep() override;

    virtual Color Atmosphere() const { return atmosphere; }
    virtual void SetAtmosphere(Color a) { atmosphere = a; }
    virtual void SetDaytime(bool d);
    virtual void SetStarSystem(StarSystem* system);

    void Render(Video* video, DWORD flags) override;

    int CheckRayIntersection(Point pt, Point vpn, double len, Point& ipt,
                             bool treat_translucent_polys_as_solid = true) override;

  protected:
    void CreateSphere(double radius, int nrings, int nsections, double minrad, double maxrad, int rsections,
                      double tscale);

    Material* mtl_surf;
    Material* mtl_limb;
    Material* mtl_ring;
    int has_ring;
    int ring_verts;
    int ring_polys;
    double ring_rad;
    double body_rad;
    Color atmosphere;
    bool daytime;

    StarSystem* star_system;
};

#endif Sky_h

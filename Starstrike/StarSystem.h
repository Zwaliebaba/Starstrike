#ifndef StarSystem_h
#define StarSystem_h

#include "Bitmap.h"
#include "Geometry.h"
#include "List.h"
#include "Solid.h"

#include "term.h"

class Orbital;
class OrbitalBody;
class OrbitalRegion;
class TerrainRegion;

class Graphic;
class Light;
class Scene;

class StarSystem
{
  public:
    
    StarSystem(std::string _name, Point loc, int iff = 0, int s = 4);
    virtual ~StarSystem();

    int operator ==(const StarSystem& s) const { return name == s.name; }

    // operations:
    virtual void Load();
    virtual void Create();
    virtual void Destroy();

    virtual void Activate(Scene& scene);
    virtual void Deactivate();

    virtual void ExecFrame();

    // accessors:
    std::string Name() const { return name; }
    std::string Govt() const { return govt; }
    std::string Description() const { return description; }
    int Affiliation() const { return affiliation; }
    int Sequence() const { return seq; }
    Point Location() const { return m_loc; }
    int NumStars() const { return sky_stars; }
    int NumDust() const { return sky_dust; }
    Color Ambient() const;

    List<OrbitalBody>& Bodies() { return bodies; }
    List<OrbitalRegion>& Regions() { return regions; }
    List<OrbitalRegion>& AllRegions() { return all_regions; }
    OrbitalRegion* ActiveRegion() { return active_region; }

    Orbital* FindOrbital(std::string_view name);
    OrbitalRegion* FindRegion(std::string_view name);

    void SetActiveRegion(OrbitalRegion* rgn);

    static void SetBaseTime(double t, bool absolute = false);
    static double GetBaseTime();
    static double Stardate() { return stardate; }
    static void CalcStardate();
    double Radius() const { return radius; }

    void SetSunlight(Color color, double brightness = 1);
    void SetBacklight(Color color, double brightness = 1);
    void RestoreTrueSunColor();
    bool HasLinkTo(StarSystem* s) const;
    const std::string& GetDataPath() const { return datapath; }

  protected:
    void ParseStar(TermStruct* val);
    void ParsePlanet(TermStruct* val);
    void ParseMoon(TermStruct* val);
    void ParseRegion(TermStruct* val);
    void ParseTerrain(TermStruct* val);
    void ParseLayer(TerrainRegion* rgn, TermStruct* val) const;

    void CreateBody(OrbitalBody& body);
    Point TerrainTransform(const Point& loc);

    std::string filename;
    std::string name;
    std::string govt;
    std::string description;
    std::string datapath;
    int affiliation;
    int seq;
    Point m_loc;
    static double stardate;
    double radius;
    bool instantiated;

    int sky_stars;
    int sky_dust;
    std::string sky_poly_stars;
    std::string sky_nebula;
    std::string sky_haze;
    double sky_uscale;
    double sky_vscale;
    Color ambient;
    Color sun_color;
    double sun_brightness;
    double sun_scale;
    List<Light> sun_lights;
    List<Light> back_lights;

    Graphic* point_stars;
    Solid* poly_stars;
    Solid* nebula;
    Solid* haze;

    List<OrbitalBody> bodies;
    List<OrbitalRegion> regions;
    List<OrbitalRegion> all_regions;

    Orbital* center;
    OrbitalRegion* active_region;

    Point tvpn, tvup, tvrt;
};

class Star
{
  public:
    
    Star(std::string_view n, const Point& l, int s)
      : name(n),
        loc(l),
        seq(s) {}

    virtual ~Star() = default;

    enum SPECTRAL_CLASS
    {
      BLACK_HOLE,
      WHITE_DWARF,
      RED_GIANT,
      O,
      B,
      A,
      F,
      G,
      K,
      M
    };

    int operator ==(const Star& s) const { return name == s.name; }

    // accessors:
    std::string Name() const { return name; }
    const Point& Location() const { return loc; }
    int Sequence() const { return seq; }
    Color GetColor() const;
    int GetSize() const;

    static Color GetColor(int spectral_class);
    static int GetSize(int spectral_class);

  protected:
    std::string name;
    Point loc;
    int seq;
};

class Orbital
{
  friend class StarSystem;

  public:
    
    enum OrbitalType
    {
      NOTHING,
      STAR,
      PLANET,
      MOON,
      REGION,
      TERRAIN
    };

    Orbital(StarSystem* sys, std::string_view n, OrbitalType t, double m, double r, double o, Orbital* p = nullptr);
    virtual ~Orbital();

    int operator ==(const Orbital& o) const { return type == o.type && name == o.name && system == o.system; }
    int operator <(const Orbital& o) const { return loc.length() < o.loc.length(); }
    int operator <=(const Orbital& o) const { return loc.length() <= o.loc.length(); }

    // operations:
    virtual void Update();
    Point PredictLocation(double delta_t);

    // accessors:
    std::string Name() const { return name; }
    OrbitalType Type() const { return type; }
    int SubType() const { return subtype; }

    std::string Description() const { return description; }
    double Mass() const { return mass; }
    double Radius() const { return radius; }
    double Rotation() const { return rotation; }
    double RotationPhase() const { return theta; }
    double Orbit() const { return orbit; }
    bool Retrograde() const { return retro; }
    double Phase() const { return phase; }
    double Period() const { return period; }
    Point Location() const { return loc; }
    Graphic* Rep() const { return rep; }

    const Bitmap& GetMapIcon() const { return map_icon; }
    void SetMapIcon(const Bitmap& img);

    StarSystem* System() const { return system; }
    Orbital* Primary() const { return primary; }
    ListIter<OrbitalRegion> Regions() { return regions; }

  protected:
    std::string name;
    OrbitalType type;
    int subtype;

    std::string description;
    double mass;
    double radius;
    double rotation;
    double theta;
    double orbit;
    double phase;
    double period;
    double velocity;
    Point loc;
    bool retro;
    Graphic* rep;
    Bitmap map_icon;

    StarSystem* system;
    Orbital* primary;

    List<OrbitalRegion> regions;
};

class OrbitalBody : public Orbital
{
  friend class StarSystem;

  public:
    
    OrbitalBody(StarSystem* sys, std::string_view n, OrbitalType t, double m, double r, double o, Orbital* prime = nullptr);
    ~OrbitalBody() override;

    // operations:
    void Update() override;

    // accessors:
    ListIter<OrbitalBody> Satellites() { return satellites; }

    double Tilt() const { return tilt; }
    double RingMin() const { return ring_min; }
    double RingMax() const { return ring_max; }

    double LightIntensity() const { return light; }
    Color LightColor() const { return color; }
    bool Luminous() const { return luminous; }

  protected:
    std::string map_name;
    std::string tex_name;
    std::string tex_high_res;
    std::string tex_ring;
    std::string tex_glow;
    std::string tex_glow_high_res;
    std::string tex_gloss;

    double tscale;
    double light;
    double ring_min;
    double ring_max;
    double tilt;
    Light* light_rep;
    Light* back_light;
    Color color;
    Color back;
    Color atmosphere;
    bool luminous;

    List<OrbitalBody> satellites;
};

class OrbitalRegion : public Orbital
{
  friend class StarSystem;

  public:
    
    OrbitalRegion(StarSystem* sys, std::string_view n, double m, double r, double o, Orbital* prime = nullptr);
    ~OrbitalRegion() override;

    double GridSpace() const { return grid; }
    double Inclination() const { return inclination; }
    int Asteroids() const { return asteroids; }
    std::vector<std::string>& Links() { return links; }

  protected:
    double grid;
    double inclination;
    int asteroids;
    std::vector<std::string> links;
};

#endif StarSystem_h

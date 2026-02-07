#ifndef TerrainRegion_h
#define TerrainRegion_h

#include "StarSystem.h"
#include "Weather.h"

constexpr double TERRAIN_ALTITUDE_LIMIT = 35e3;

class TerrainLayer;

class TerrainRegion : public OrbitalRegion
{
  friend class StarSystem;

  public:
    TerrainRegion(StarSystem* sys, std::string_view n, double r, Orbital* prime = nullptr);
    ~TerrainRegion() override;

    // operations:
    void Update() override;

    // accessors:
    std::string PatchName() const { return patch_name; }
    std::string PatchTexture() const { return patch_texture; }
    std::string ApronName() const { return apron_name; }
    std::string ApronTexture() const { return apron_texture; }
    std::string WaterTexture() const { return water_texture; }
    std::string DetailTexture0() const { return noise_tex0; }
    std::string DetailTexture1() const { return noise_tex1; }
    std::string HazeName() const { return haze_name; }
    std::string CloudsHigh() const { return clouds_high; }
    std::string ShadesHigh() const { return shades_high; }
    std::string CloudsLow() const { return clouds_low; }
    std::string ShadesLow() const { return shades_low; }
    std::string EnvironmentTexture(int n) const;

    Color SunColor() const { return sun_color[24]; }
    Color SkyColor() const { return sky_color[24]; }
    Color FogColor() const { return fog_color[24]; }
    Color Ambient() const { return ambient[24]; }
    Color Overcast() const { return overcast[24]; }
    Color CloudColor() const { return cloud_color[24]; }
    Color ShadeColor() const { return shade_color[24]; }

    double LateralScale() const { return scale; }
    double MountainScale() const { return mtnscale; }
    double FogDensity() const { return fog_density; }
    double FogScale() const { return fog_scale; }
    double DayPhase() const { return day_phase; }
    double HazeFade() const { return haze_fade; }
    double CloudAltHigh() const { return clouds_alt_high; }
    double CloudAltLow() const { return clouds_alt_low; }
    Weather& GetWeather() { return weather; }
    List<TerrainLayer>& GetLayers() { return layers; }

    bool IsEclipsed() const { return eclipsed; }
    void SetEclipsed(bool e) { eclipsed = e; }

    void LoadSkyColors(std::string_view bmp_name);
    void AddLayer(double h, std::string_view tile, std::string_view detail = {});

  protected:
    std::string patch_name;
    std::string patch_texture;
    std::string apron_name;
    std::string apron_texture;
    std::string water_texture;
    std::string env_texture_positive_x;
    std::string env_texture_negative_x;
    std::string env_texture_positive_y;
    std::string env_texture_negative_y;
    std::string env_texture_positive_z;
    std::string env_texture_negative_z;
    std::string noise_tex0;
    std::string noise_tex1;
    std::string haze_name;
    std::string clouds_high;
    std::string clouds_low;
    std::string shades_high;
    std::string shades_low;

    Color sun_color[25];
    Color sky_color[25];
    Color fog_color[25];
    Color ambient[25];
    Color overcast[25];
    Color cloud_color[25];
    Color shade_color[25];

    double scale;
    double mtnscale;

    double fog_density;
    double fog_scale;
    double day_phase;
    double haze_fade;
    double clouds_alt_high;
    double clouds_alt_low;

    Weather weather;
    bool eclipsed;

    List<TerrainLayer> layers;
};

#endif TerrainRegion_h

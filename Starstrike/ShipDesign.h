#ifndef ShipDesign_h
#define ShipDesign_h

#include "Bitmap.h"
#include "Geometry.h"
#include "List.h"
#include "term.h"

class ShipDesign;
class Model;
class Skin;
class PowerSource;
class Weapon;
class HardPoint;
class Computer;
class Drive;
class QuantumDrive;
class Farcaster;
class Thruster;
class Sensor;
class NavLight;
class NavSystem;
class Shield;
class FlightDeck;
class LandingGear;
class System;
class Sound;

class ShipLoad
{
  public:
    ShipLoad() = default;

    std::string name;
    int load[16] = {};
    double mass = 0;
};

class ShipSquadron
{
  public:
    ShipSquadron();

    std::string name;
    ShipDesign* design;
    int count;
    int avail;
};

class ShipExplosion
{
  public:
    ShipExplosion() = default;

    int type;
    float time;
    Vec3 loc;
    bool final;
};

class ShipDebris
{
  public:
    ShipDebris() = default;

    Model* model;
    int count;
    int life;
    Vec3 loc;
    float mass;
    float speed;
    float drag;
    int fire_type;
    Vec3 fire_loc[5];
};

// Used to share common information about ships of a single type.
// ShipDesign objects are loaded from a text file and stored in a
// static list (catalog) member for use by the Ship.

class ShipDesign
{
  public:
    
    enum CONSTANTS
    {
      MAX_DEBRIS     = 10,
      MAX_EXPLOSIONS = 10
    };

    ShipDesign();
    ShipDesign(std::string_view name, std::string_view path, std::string_view filename, bool secret = false);
    ~ShipDesign();

    // public interface:
    static void Initialize();
    static void Close();
    static bool CheckName(std::string_view name);
    static ShipDesign* Get(std::string_view _designName, std::string_view _designPath = {});
    static int GetDesignList(int type, std::vector<std::string>& designs);  

    static int ClassForName(std::string_view name);
    static std::string_view ClassName(int type);

    static int LoadCatalog(const char* _path, const char* _fname);
    static void PreloadCatalog(int index = -1);
    static int StandardCatalogSize();

    int operator ==(const ShipDesign& s) const { return m_name == s.m_name; }

    // Parser:
    void ParseShip(TermDef* def);

    void ParsePower(TermStruct* val);
    void ParseDrive(TermStruct* val);
    void ParseQuantumDrive(TermStruct* _val);
    void ParseFarcaster(TermStruct* val);
    void ParseThruster(TermStruct* val);
    void ParseNavlight(TermStruct* val);
    void ParseFlightDeck(TermStruct* val);
    void ParseLandingGear(TermStruct* val);
    void ParseWeapon(TermStruct* val);
    void ParseHardPoint(TermStruct* val);
    void ParseSensor(TermStruct* val);
    void ParseNavsys(TermStruct* val);
    void ParseComputer(TermStruct* val);
    void ParseShield(TermStruct* val);
    void ParseDeathSpiral(TermStruct* _val);
    void ParseExplosion(TermStruct* val, int index);
    void ParseDebris(TermStruct* val, int index);
    void ParseLoadout(TermStruct* val);
    void ParseMap(TermStruct* val);
    void ParseSquadron(TermStruct* _val);
    Skin* ParseSkin(TermStruct* _val);
    void ParseSkinMtl(TermStruct* _val, Skin* _skin);

    // general information:
    std::string DisplayName() const;

    std::string m_filename;
    std::string path_name;
    std::string m_name;
    std::string display_name;
    std::string m_abrv;
    int type = {0};
    float scale = {0};
    int auto_roll = {1};
    bool valid = {false};
    bool secret = {false};        // don't display in editor
    std::string description;   // background info for tactical reference

    // LOD representation:
    int lod_levels;
    List<Model> models[4];
    List<Point> offsets[4];
    float feature_size[4];
    List<Point> spin_rates;

    // player selectable skins:
    List<Skin> skins;
    const Skin* FindSkin(std::string_view skin_name) const;

    // virtual cockpit:
    Model* cockpit_model;
    float cockpit_scale;

    // performance:
    float vlimit;
    float agility;
    float air_factor;
    float roll_rate;
    float pitch_rate;
    float yaw_rate;
    float trans_x;
    float trans_y;
    float trans_z;
    float turn_bank;
    Vec3 chase_vec;
    Vec3 bridge_vec;
    Vec3 beauty_cam;

    float prep_time;

    // physical data:
    float drag, roll_drag, pitch_drag, yaw_drag;
    float arcade_drag;
    float mass, integrity, radius;

    // aero data:
    float CL, CD, stall;

    // weapons:
    int primary;
    int secondary;

    // drives:
    int main_drive;

    // visibility:
    float pcs;           // passive sensor cross section
    float acs;           // active sensor cross section
    float detet;         // maximum detection range
    float e_factor[3];   // pcs scaling by emcon setting

    // ai settings:
    float avoid_time;
    float avoid_fighter;
    float avoid_strike;
    float avoid_target;
    float commit_range;

    // death spriral sequence:
    float death_spiral_time;
    float explosion_scale;
    ShipExplosion explosion[MAX_EXPLOSIONS];
    ShipDebris debris[MAX_DEBRIS];

    List<PowerSource> reactors;
    List<Weapon> weapons;
    List<HardPoint> hard_points;
    List<Drive> drives;
    List<Computer> computers;
    List<FlightDeck> flight_decks;
    List<NavLight> navlights;
    QuantumDrive* quantum_drive;
    Farcaster* farcaster;
    Thruster* thruster;
    Sensor* sensor;
    NavSystem* navsys;
    Shield* shield;
    Model* shield_model;
    Weapon* decoy;
    Weapon* probe;
    LandingGear* gear;

    float splash_radius;
    float scuttle;
    float repair_speed;
    int repair_teams;
    bool repair_auto;
    bool repair_screen;
    bool wep_screen;

    std::string bolt_hit_sound;
    std::string beam_hit_sound;

    Sound* bolt_hit_sound_resource;
    Sound* beam_hit_sound_resource;

    List<ShipLoad> loadouts;
    List<Bitmap> map_sprites;
    List<ShipSquadron> squadrons;

    Bitmap beauty;
    Bitmap hud_icon;
};

#endif ShipDesign_h

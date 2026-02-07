#include "pch.h"
#include "ShipDesign.h"
#include "Bitmap.h"
#include "Computer.h"
#include "DataLoader.h"
#include "Drive.h"
#include "Farcaster.h"
#include "FlightDeck.h"
#include "Game.h"
#include "HardPoint.h"
#include "LandingGear.h"
#include "NavLight.h"
#include "NavSystem.h"
#include "ParseUtil.h"
#include "Power.h"
#include "QuantumDrive.h"
#include "Reader.h"
#include "Sensor.h"
#include "Shield.h"
#include "Ship.h"
#include "Skin.h"
#include "Solid.h"
#include "Sound.h"
#include "SystemDesign.h"
#include "Thruster.h"
#include "Weapon.h"
#include "WeaponDesign.h"

static std::string_view ship_design_class_name[32] = {
  "Drone",
  "Fighter",
  "Attack",
  "LCA",
  "Courier",
  "Cargo",
  "Corvette",
  "Freighter",
  "Frigate",
  "Destroyer",
  "Cruiser",
  "Battleship",
  "Carrier",
  "Dreadnaught",
  "Station",
  "Farcaster",
  "Mine",
  "DEFSAT",
  "COMSAT",
  "SWACS",
  "Building",
  "Factory",
  "SAM",
  "EWR",
  "C3I",
  "Starbase",
  "0x04000000",
  "0x08000000",
  "0x10000000",
  "0x20000000",
  "0x40000000",
  "0x80000000"
};

static constexpr int NAMELEN = 64;
static bool degrees = false;

struct ShipCatalogEntry
{
  ShipCatalogEntry() = default;

  ShipCatalogEntry(std::string_view n, std::string_view t, std::string_view p, std::string_view f, bool h = false)
    : name(n),
      type(t),
      path(p),
      file(f),
      hide(h)
  {}

  ~ShipCatalogEntry() { delete design; }

  std::string name;
  std::string type;
  std::string path;
  std::string file;
  bool hide = {false};

  ShipDesign* design = {};
};

static std::vector<ShipCatalogEntry> g_catalog;

#define GET_DEF_BOOL(n) if (defname==(#n)) GetDefBool((n),   def, m_filename)
#define GET_DEF_TEXT(n) if (defname==(#n)) GetDefText((n),   def, m_filename)
#define GET_DEF_NUM(n)  if (defname==(#n)) GetDefNumber((n), def, m_filename)
#define GET_DEF_VEC(n)  if (defname==(#n)) GetDefVec((n),    def, m_filename)

static std::string cockpit_name;
static std::array<std::vector<std::string>, 4> g_detail;
static List<Point> offset[4];

ShipSquadron::ShipSquadron()
{
  design = nullptr;
  count = 4;
  avail = 4;
}

static void PrepareModel(Model& model)
{
  bool uses_bumps = false;

  ListIter<Material> iter = model.GetMaterials();
  while (++iter && !uses_bumps)
  {
    Material* mtl = iter.value();
    if (mtl->tex_bumpmap != nullptr && mtl->bump != 0)
      uses_bumps = true;
  }

  if (uses_bumps)
    model.ComputeTangents();
}

ShipDesign::ShipDesign()
  : lod_levels(0),
    cockpit_model(nullptr),
    cockpit_scale(0),
    vlimit(0),
    agility(0),
    air_factor(0),
    roll_rate(0),
    pitch_rate(0),
    yaw_rate(0),
    trans_x(0),
    trans_y(0),
    trans_z(0),
    turn_bank(0),
    prep_time(0),
    drag(0),
    roll_drag(0),
    pitch_drag(0),
    yaw_drag(0),
    arcade_drag(0),
    mass(0),
    integrity(0),
    radius(0),
    CL(0),
    CD(0),
    stall(0),
    primary(0),
    secondary(0),
    main_drive(0),
    pcs(0),
    acs(0),
    detet(0),
    e_factor{},
    avoid_time(0),
    avoid_fighter(0),
    avoid_strike(0),
    avoid_target(0),
    commit_range(0),
    death_spiral_time(0),
    explosion_scale(0),
    quantum_drive(nullptr),
    farcaster(nullptr),
    thruster(nullptr),
    sensor(nullptr),
    navsys(nullptr),
    shield(nullptr),
    shield_model(nullptr),
    decoy(nullptr),
    probe(nullptr),
    gear(nullptr),
    splash_radius(0),
    scuttle(0),
    repair_speed(0),
    repair_teams(0),
    repair_auto(false),
    repair_screen(false),
    wep_screen(false),
    bolt_hit_sound_resource(nullptr),
    beam_hit_sound_resource(nullptr)
{
  for (int i = 0; i < 4; i++)
    feature_size[i] = 0.0f;
}

ShipDesign::ShipDesign(std::string_view n, std::string_view p, std::string_view fname, bool s)
  : m_name(n),
    secret(s),
    lod_levels(0),
    cockpit_model(nullptr),
    quantum_drive(nullptr),
    farcaster(nullptr),
    thruster(nullptr),
    sensor(nullptr),
    navsys(nullptr),
    shield(nullptr),
    shield_model(nullptr),
    decoy(nullptr),
    probe(nullptr),
    gear(nullptr),
    bolt_hit_sound_resource(nullptr),
    beam_hit_sound_resource(nullptr)
{
  if (!fname.contains(".def"))
    m_filename = std::string(fname) + std::string(".def");
  else
    m_filename = fname;

  for (int i = 0; i < 4; i++)
    feature_size[i] = 0.0f;

  scale = 1.0f;

  agility = 2e2f;
  air_factor = 0.1f;
  vlimit = 8e3f;
  drag = 2.5e-5f;
  arcade_drag = 1.0f;
  roll_drag = 5.0f;
  pitch_drag = 5.0f;
  yaw_drag = 5.0f;

  roll_rate = 0.0f;
  pitch_rate = 0.0f;
  yaw_rate = 0.0f;

  trans_x = 0.0f;
  trans_y = 0.0f;
  trans_z = 0.0f;

  turn_bank = static_cast<float>(PI / 8);

  CL = 0.0f;
  CD = 0.0f;
  stall = 0.0f;

  prep_time = 30.0f;
  avoid_time = 0.0f;
  avoid_fighter = 0.0f;
  avoid_strike = 0.0f;
  avoid_target = 0.0f;
  commit_range = 0.0f;

  splash_radius = -1.0f;
  scuttle = 5e3f;
  repair_speed = 1.0f;
  repair_teams = 2;
  repair_auto = true;
  repair_screen = true;
  wep_screen = true;

  chase_vec = Vec3(0, -100, 20);
  bridge_vec = Vec3(0, 0, 0);
  beauty_cam = Vec3(0, 0, 0);
  cockpit_scale = 1.0f;

  radius = 1.0f;
  integrity = 500.0f;

  primary = 0;
  secondary = 1;
  main_drive = -1;

  pcs = 3.0f;
  acs = 1.0f;
  detet = 250.0e3f;
  e_factor[0] = 0.1f;
  e_factor[1] = 0.3f;
  e_factor[2] = 1.0f;

  explosion_scale = 0.0f;
  death_spiral_time = 3.0f;

  if (!secret)
    DebugTrace("Loading ShipDesign '{}'\n", m_name);

  path_name = p;

  if (path_name[path_name.size() - 1] != '\\')
    path_name += "\\";

  // Load Design File:
  DataLoader* loader = DataLoader::GetLoader();
  loader->SetDataPath(path_name);

  BYTE* block;
  int blocklen = loader->LoadBuffer(m_filename, block, true);

  // file not found:
  if (blocklen <= 4)
  {
    valid = false;
    return;
  }

  Parser parser(NEW BlockReader((const char*)block));
  Term* term = parser.ParseTerm();

  if (!term)
  {
    DebugTrace("ERROR: could not parse '{}'\n", m_filename);
    valid = false;
    return;
  }
  TermText* file_type = term->isText();
  if (!file_type || file_type->value() != "SHIP")
  {
    DebugTrace("ERROR: invalid ship design file '{}'\n", m_filename);
    valid = false;
    return;
  }

  cockpit_name.clear();
  valid = true;
  degrees = false;

  do
  {
    delete term;

    term = parser.ParseTerm();

    if (term)
    {
      TermDef* def = term->isDef();
      if (def)
        ParseShip(def);
      else
      {
        DebugTrace("WARNING: term ignored in '{}'\n", m_filename);
        term->print();
      }
    }
  }
  while (term);

  for (int i = 0; i < 4; i++)
  {
    int n = 0;
    for (auto& model_name : g_detail[i])
    {
      auto model = NEW Model;
      if (!model->Load(model_name, scale))
      {
        DebugTrace("ERROR: Could not load detail {}, model '{}'\n", i, model_name);
        delete model;
        model = nullptr;
        valid = false;
      }
      else
      {
        lod_levels = i + 1;

        if (model->Radius() > radius)
          radius = static_cast<float>(model->Radius());

        models[i].append(model);
        PrepareModel(*model);

        if (offset[i].size())
        {
          *offset[i].at(n) *= scale;
          offsets[i].append(offset[i].at(n)); // transfer ownership
        }
        else
          offsets[i].append(NEW Point);

        n++;
      }
    }

    g_detail[i].clear();
  }

  if (!secret)
    DebugTrace("   Ship Design Radius = {}\n", radius);

  if (!cockpit_name.empty())
  {
    cockpit_model = NEW Model;
    if (!cockpit_model->Load(cockpit_name, cockpit_scale))
    {
      DebugTrace("ERROR: Could not load cockpit model '{}'\n", cockpit_name);
      delete cockpit_model;
      cockpit_model = nullptr;
    }
    else
    {
      if (!secret)
        DebugTrace("   Loaded cockpit model '{}', preparing tangents\n", cockpit_name);
      PrepareModel(*cockpit_model);
    }
  }

  if (beauty.Width() < 1 && loader->FindFile("beauty.pcx"))
    loader->LoadBitmap("beauty.pcx", beauty);

  if (hud_icon.Width() < 1 && loader->FindFile("hud_icon.pcx"))
    loader->LoadBitmap("hud_icon.pcx", hud_icon);

  loader->ReleaseBuffer(block);
  loader->SetDataPath();

  if (m_abrv[0] == 0)
  {
    switch (type)
    {
      case Ship::DRONE:
        m_abrv = "DR";
        break;
      case Ship::FIGHTER:
        m_abrv = "F";
        break;
      case Ship::ATTACK:
        m_abrv = "F/A";
        break;
      case Ship::LCA:
        m_abrv = "LCA";
        break;
      case Ship::CORVETTE:
        m_abrv = "FC";
        break;
      case Ship::COURIER:
      case Ship::CARGO:
      case Ship::FREIGHTER:
        m_abrv = "MV";
        break;
      case Ship::FRIGATE:
        m_abrv = "FF";
        break;
      case Ship::DESTROYER:
        m_abrv = "DD";
        break;
      case Ship::CRUISER:
        m_abrv = "CA";
        break;
      case Ship::BATTLESHIP:
        m_abrv = "BB";
        break;
      case Ship::CARRIER:
        m_abrv = "CV";
        break;
      case Ship::DREADNAUGHT:
        m_abrv = "DN";
        break;
      case Ship::MINE:
        m_abrv = "MINE";
        break;
      case Ship::COMSAT:
        m_abrv = "COMS";
        break;
      case Ship::DEFSAT:
        m_abrv = "DEFS";
        break;
      case Ship::SWACS:
        m_abrv = "SWAC";
        break;
      default:
        break;
    }
  }

  scuttle = std::max(scuttle, 1.0f);

  if (splash_radius < 0)
    splash_radius = radius * 12.0f;

  if (repair_speed <= 1e-6)
    repair_speed = 1.0e-6f;

  if (commit_range <= 0)
  {
    if (type <= Ship::LCA)
      commit_range = 80.0e3f;
    else
      commit_range = 200.0e3f;
  }

  // calc standard loadout weights:
  ListIter<ShipLoad> sl = loadouts;
  while (++sl)
  {
    for (int i = 0; i < hard_points.size(); i++)
    {
      HardPoint* hp = hard_points[i];
      sl->mass += hp->GetCarryMass(sl->load[i]);
    }
  }
}

ShipDesign::~ShipDesign()
{
  delete bolt_hit_sound_resource;
  delete beam_hit_sound_resource;
  delete cockpit_model;
  delete navsys;
  delete sensor;
  delete shield;
  delete thruster;
  delete farcaster;
  delete quantum_drive;
  delete decoy;
  delete probe;
  delete gear;

  navlights.destroy();
  flight_decks.destroy();
  hard_points.destroy();
  computers.destroy();
  weapons.destroy();
  drives.destroy();
  reactors.destroy();
  loadouts.destroy();
  map_sprites.destroy();

  delete shield_model;
  for (int i = 0; i < 4; i++)
  {
    models[i].destroy();
    offsets[i].destroy();
  }

  spin_rates.destroy();

  for (int i = 0; i < 10; i++)
    delete debris[i].model;
}

std::string ShipDesign::DisplayName() const
{
  if (!display_name.empty())
    return display_name;

  return m_name;
}

void ShipDesign::Initialize()
{
  if (g_catalog.size())
    return;

  LoadCatalog("Ships\\", "catalog.def");
}

void ShipDesign::Close() { g_catalog.clear(); }

int ShipDesign::LoadCatalog(const char* _path, const char* _fname)
{
  int result = 0;

  // Load Design Catalog File:
  DataLoader* loader = DataLoader::GetLoader();
  loader->SetDataPath(_path);

  char filename[NAMELEN] = {};
  strncpy(filename, _fname, NAMELEN - 1);

  DebugTrace("Loading ship design catalog: {}{}\n", _path, filename);

  BYTE* block;
  int blocklen = loader->LoadBuffer(filename, block, true);
  Parser parser(NEW BlockReader((const char*)block));
  Term* term = parser.ParseTerm();

  if (!term)
  {
    DebugTrace("ERROR: could not parse '{}'\n", filename);
    loader->ReleaseBuffer(block);
    loader->SetDataPath();
    return result;
  }
  TermText* file_type = term->isText();
  if (!file_type || file_type->value() != "SHIPCATALOG")
  {
    DebugTrace("ERROR: invalid ship catalog file '{}'\n", filename);
    loader->ReleaseBuffer(block);
    loader->SetDataPath();
    return result;
  }

  do
  {
    delete term;

    term = parser.ParseTerm();

    if (term)
    {
      TermDef* def = term->isDef();
      if (def && def->term() && def->term()->isStruct())
      {
        std::string path;
        std::string fname;
        bool hide = false;
        std::string type;
        std::string name;
        TermStruct* val = def->term()->isStruct();

        for (int i = 0; i < val->elements()->size(); i++)
        {
          TermDef* pdef = val->elements()->at(i)->isDef();
          if (pdef)
          {
            std::string defname = pdef->name()->value();

            if (defname == "name")
            {
              if (!GetDefText(name, pdef, filename))
                DebugTrace("WARNING: invalid or missing ship name in '{}'\n", filename);
            }
            else if (defname == "type")
            {
              if (!GetDefText(type, pdef, filename))
                DebugTrace("WARNING: invalid or missing ship type in '{}'\n", filename);
            }
            else if (defname == "path")
            {
              if (!GetDefText(path, pdef, filename))
                DebugTrace("WARNING: invalid or missing ship path in '{}'\n", filename);
            }
            else if (defname == "file")
            {
              if (!GetDefText(fname, pdef, filename))
                DebugTrace("WARNING: invalid or missing ship file in '{}'\n", filename);
            }
            else if (defname == "hide" || defname == "secret")
              GetDefBool(hide, pdef, filename);
          }
        }

        g_catalog.emplace_back(ShipCatalogEntry(name, type, path, fname, hide));

        result++;
      }
      else
      {
        DebugTrace("WARNING: term ignored in '{}'\n", filename);
        term->print();
      }
    }
  }
  while (term);

  loader->ReleaseBuffer(block);
  loader->SetDataPath();

  return result;
}

int ShipDesign::StandardCatalogSize() { return g_catalog.size(); }

void ShipDesign::PreloadCatalog(int index)
{
  if (index >= 0 && index < g_catalog.size())
  {
    ShipCatalogEntry* entry = &g_catalog[index];

    if (entry->hide)
      return;

    int ship_class = ClassForName(entry->type);
    if (ship_class > Ship::STARSHIPS)
      return;

    if (!entry->path.contains("Alliance_"))
      return;

    if (!entry->design)
      entry->design = NEW ShipDesign(entry->name, entry->path, entry->file, entry->hide);
  }

  else
  {
    for (auto& entry : g_catalog)
    {
      if (!entry.design)
        entry.design = NEW ShipDesign(entry.name, entry.path, entry.file, entry.hide);
    }
  }
}

bool ShipDesign::CheckName(std::string_view design_name)
{
  ShipCatalogEntry* entry = nullptr;

  for (int i = 0; i < g_catalog.size(); i++)
  {
    if (g_catalog[i].name == design_name)
    {
      entry = &g_catalog[i];
      break;
    }
  }

  return entry != nullptr;
}

ShipDesign* ShipDesign::Get(std::string_view _designName, std::string_view _designPath)
{
  if (_designName.empty())
    return nullptr;

  ShipCatalogEntry* entry = nullptr;

  for (auto& e : g_catalog)
  {
    if (e.name == _designName)
    {
      if (!_designPath.empty() && e.path != _designPath)
        continue;
      entry = &e;
      break;
    }
  }

  if (entry)
  {
    if (!entry->design)
      entry->design = NEW ShipDesign(entry->name, entry->path, entry->file, entry->hide);
    return entry->design;
  }
  DebugTrace("ShipDesign: no catalog entry for design '{}'\n", _designName);
  return nullptr;
}

int ShipDesign::GetDesignList(int type, std::vector<std::string>& designs)
{
  designs.clear();

  for (auto& e : g_catalog)
  {
    int etype = ClassForName(e.type);
    if (etype & type)
    {
      if (!e.design)
        e.design = NEW ShipDesign(e.name, e.path, e.file, e.hide);

      if (e.hide || !e.design || !e.design->valid || e.design->secret)
        continue;

      designs.emplace_back(e.name);
    }
  }

  return designs.size();
}

int ShipDesign::ClassForName(std::string_view name)
{
  if (name.empty())
    return 0;

  for (int i = 0; i < 32; i++)
  {
    if (name == ship_design_class_name[i])
      return 1 << i;
  }

  return 0;
}

std::string_view ShipDesign::ClassName(int type)
{
  if (type != 0)
  {
    int index = 0;

    while (!(type & 1))
    {
      type >>= 1;
      index++;
    }

    if (index >= 0 && index < 32)
      return ship_design_class_name[index];
  }

  return "Unknown";
}

void ShipDesign::ParseShip(TermDef* def)
{
  std::string detail_name;
  Vec3 off_loc;
  std::string defname = def->name()->value();

  if (defname == "cockpit_model")
  {
    if (!GetDefText(cockpit_name, def, m_filename))
      DebugTrace("WARNING: invalid or missing cockpit_model in '{}'\n", m_filename);
  }

  else if (defname == "model" || defname == "detail_0")
  {
    if (!GetDefText(detail_name, def, m_filename))
      DebugTrace("WARNING: invalid or missing model in '{}'\n", m_filename);

    g_detail[0].emplace_back(detail_name);
  }

  else if (defname == "detail_1")
  {
    if (!GetDefText(detail_name, def, m_filename))
      DebugTrace("WARNING: invalid or missing detail_1 in '{}'\n", m_filename);

    g_detail[1].emplace_back(detail_name);
  }

  else if (defname == "detail_2")
  {
    if (!GetDefText(detail_name, def, m_filename))
      DebugTrace("WARNING: invalid or missing detail_2 in '{}'\n", m_filename);

    g_detail[2].emplace_back(detail_name);
  }

  else if (defname == "detail_3")
  {
    if (!GetDefText(detail_name, def, m_filename))
      DebugTrace("WARNING: invalid or missing detail_3 in '{}'\n", m_filename);

    g_detail[3].emplace_back(detail_name);
  }

  else if (defname == "spin")
  {
    Vec3 spin;
    if (!GetDefVec(spin, def, m_filename))
      DebugTrace("WARNING: invalid or missing spin in '{}'\n", m_filename);

    spin_rates.append(NEW Point(spin));
  }

  else if (defname == "offset_0")
  {
    if (!GetDefVec(off_loc, def, m_filename))
      DebugTrace("WARNING: invalid or missing offset_0 in '{}'\n", m_filename);

    offset[0].append(NEW Point(off_loc));
  }

  else if (defname == "offset_1")
  {
    if (!GetDefVec(off_loc, def, m_filename))
      DebugTrace("WARNING: invalid or missing offset_1 in '{}'\n", m_filename);

    offset[1].append(NEW Point(off_loc));
  }

  else if (defname == "offset_2")
  {
    if (!GetDefVec(off_loc, def, m_filename))
      DebugTrace("WARNING: invalid or missing offset_2 in '{}'\n", m_filename);

    offset[2].append(NEW Point(off_loc));
  }

  else if (defname == "offset_3")
  {
    if (!GetDefVec(off_loc, def, m_filename))
      DebugTrace("WARNING: invalid or missing offset_3 in '{}'\n", m_filename);

    offset[3].append(NEW Point(off_loc));
  }

  else if (defname == "beauty")
  {
    if (def->term() && def->term()->isArray())
    {
      GetDefVec(beauty_cam, def, m_filename);

      if (degrees)
      {
        beauty_cam.x *= static_cast<float>(DEGREES);
        beauty_cam.y *= static_cast<float>(DEGREES);
      }
    }

    else
    {
      std::string beauty_name;
      if (!GetDefText(beauty_name, def, m_filename))
        DebugTrace("WARNING: invalid or missing beauty in '{}'\n", m_filename);

      DataLoader* loader = DataLoader::GetLoader();
      loader->LoadBitmap(beauty_name, beauty);
    }
  }

  else if (defname == "hud_icon")
  {
    std::string hud_icon_name;
    if (!GetDefText(hud_icon_name, def, m_filename))
      DebugTrace("WARNING: invalid or missing hud_icon in '{}'\n", m_filename);

    DataLoader* loader = DataLoader::GetLoader();
    loader->LoadBitmap(hud_icon_name, hud_icon);
  }

  else if (defname == "feature_0")
  {
    if (!GetDefNumber(feature_size[0], def, m_filename))
      DebugTrace("WARNING: invalid or missing feature_0 in '{}'\n", m_filename);
  }

  else if (defname == "feature_1")
  {
    if (!GetDefNumber(feature_size[1], def, m_filename))
      DebugTrace("WARNING: invalid or missing feature_1 in '{}'\n", m_filename);
  }

  else if (defname == "feature_2")
  {
    if (!GetDefNumber(feature_size[2], def, m_filename))
      DebugTrace("WARNING: invalid or missing feature_2 in '{}'\n", m_filename);
  }

  else if (defname == "feature_3")
  {
    if (!GetDefNumber(feature_size[3], def, m_filename))
      DebugTrace("WARNING: invalid or missing feature_3 in '{}'\n", m_filename);
  }

  else if (defname == "class")
  {
    std::string typestr;
    if (!GetDefText(typestr, def, m_filename))
      DebugTrace("WARNING: invalid or missing ship class in '{}'\n", m_filename);

    type = ClassForName(typestr);

    if (type <= Ship::LCA)
    {
      repair_auto = false;
      repair_screen = false;
      wep_screen = false;
    }
  }
  else if (defname == "name")
    GetDefText(m_name, def, m_filename);
  else if (defname == "description")
    GetDefText(description, def, m_filename);
  else if (defname == "display_name")
    GetDefText(display_name, def, m_filename);
  else if (defname == "abrv")
    GetDefText(m_abrv, def, m_filename);
  else if (defname == "pcs")
    GetDefNumber(pcs, def, m_filename);
  else if (defname == "acs")
    GetDefNumber(acs, def, m_filename);
  else if (defname == "detet")
    GetDefNumber(detet, def, m_filename);
  else if (defname == "scale")
    GetDefNumber(scale, def, m_filename);
  else if (defname == "explosion_scale")
    GetDefNumber(explosion_scale, def, m_filename);
  else if (defname == "mass")
    GetDefNumber(mass, def, m_filename);
  else if (defname == "vlimit")
    GetDefNumber(vlimit, def, m_filename);
  else if (defname == "agility")
    GetDefNumber(agility, def, m_filename);
  else if (defname == "air_factor")
    GetDefNumber(air_factor, def, m_filename);
  else if (defname == "roll_rate")
    GetDefNumber(roll_rate, def, m_filename);
  else if (defname == "pitch_rate")
    GetDefNumber(pitch_rate, def, m_filename);
  else if (defname == "yaw_rate")
    GetDefNumber(yaw_rate, def, m_filename);
  else
    GET_DEF_NUM(integrity);
    else
      GET_DEF_NUM(drag);
      else
        GET_DEF_NUM(arcade_drag);
        else
          GET_DEF_NUM(roll_drag);
          else
            GET_DEF_NUM(pitch_drag);
            else
              GET_DEF_NUM(yaw_drag);
              else
                GET_DEF_NUM(trans_x);
                else
                  GET_DEF_NUM(trans_y);
                  else
                    GET_DEF_NUM(trans_z);
                    else
                      GET_DEF_NUM(turn_bank);
                      else
                        GET_DEF_NUM(cockpit_scale);
                        else
                          GET_DEF_NUM(auto_roll);

                          else
                            GET_DEF_NUM(CL);
                            else
                              GET_DEF_NUM(CD);
                              else
                                GET_DEF_NUM(stall);

                                else
                                  GET_DEF_NUM(prep_time);
                                  else
                                    GET_DEF_NUM(avoid_time);
                                    else
                                      GET_DEF_NUM(avoid_fighter);
                                      else
                                        GET_DEF_NUM(avoid_strike);
                                        else
                                          GET_DEF_NUM(avoid_target);
                                          else
                                            GET_DEF_NUM(commit_range);

                                            else
                                              GET_DEF_NUM(splash_radius);
                                              else
                                                GET_DEF_NUM(scuttle);
                                                else
                                                  GET_DEF_NUM(repair_speed);
                                                  else
                                                    GET_DEF_NUM(repair_teams);
                                                    else
                                                      GET_DEF_BOOL(secret);
                                                      else
                                                        GET_DEF_BOOL(repair_auto);
                                                        else
                                                          GET_DEF_BOOL(repair_screen);
                                                          else
                                                            GET_DEF_BOOL(wep_screen);
                                                            else
                                                              GET_DEF_BOOL(degrees);

                                                              else if (defname == "emcon_1")
                                                              {
                                                                GetDefNumber(e_factor[0], def, m_filename);
                                                              }

                                                              else if (defname == "emcon_2")
                                                              {
                                                                GetDefNumber(e_factor[1], def, m_filename);
                                                              }

                                                              else if (defname == "emcon_3")
                                                              {
                                                                GetDefNumber(e_factor[2], def, m_filename);
                                                              }

                                                              else if (defname == "chase")
                                                              {
                                                                if (!GetDefVec(chase_vec, def, m_filename))
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: invalid or missing chase cam loc in '{}'\n",
                                                                    m_filename);
                                                                }

                                                                chase_vec *= scale;
                                                              }

                                                              else if (defname == "bridge")
                                                              {
                                                                if (!GetDefVec(bridge_vec, def, m_filename))
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: invalid or missing bridge cam loc in '{}'\n",
                                                                    m_filename);
                                                                }

                                                                bridge_vec *= scale;
                                                              }

                                                              else if (defname == "power")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: power source struct missing in '{}'\n",
                                                                    m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParsePower(val);
                                                                }
                                                              }

                                                              else if (defname == "main_drive" || defname == "drive")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: main drive struct missing in '{}'\n",
                                                                    m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseDrive(val);
                                                                }
                                                              }

                                                              else if (defname == "quantum" || defname ==
                                                                "quantum_drive")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: quantum_drive struct missing in '{}'\n",
                                                                    m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseQuantumDrive(val);
                                                                }
                                                              }

                                                              else if (defname == "sender" || defname == "farcaster")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: farcaster struct missing in '{}'\n",
                                                                    m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseFarcaster(val);
                                                                }
                                                              }

                                                              else if (defname == "thruster")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: thruster struct missing in '{}'\n",
                                                                    m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseThruster(val);
                                                                }
                                                              }

                                                              else if (defname == "navlight")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: navlight struct missing in '{}'\n",
                                                                    m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseNavlight(val);
                                                                }
                                                              }

                                                              else if (defname == "flightdeck")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: flightdeck struct missing in '{}'\n",
                                                                    m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseFlightDeck(val);
                                                                }
                                                              }

                                                              else if (defname == "gear")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace("WARNING: gear struct missing in '{}'\n",
                                                                             m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseLandingGear(val);
                                                                }
                                                              }

                                                              else if (defname == "weapon")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace("WARNING: weapon struct missing in '{}'\n",
                                                                             m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseWeapon(val);
                                                                }
                                                              }

                                                              else if (defname == "hardpoint")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: hardpoint struct missing in '{}'\n",
                                                                    m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseHardPoint(val);
                                                                }
                                                              }

                                                              else if (defname == "loadout")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: loadout struct missing in '{}'\n",
                                                                    m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseLoadout(val);
                                                                }
                                                              }

                                                              else if (defname == "decoy")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace("WARNING: decoy struct missing in '{}'\n",
                                                                             m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseWeapon(val);
                                                                }
                                                              }

                                                              else if (defname == "probe")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace("WARNING: probe struct missing in '{}'\n",
                                                                             m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseWeapon(val);
                                                                }
                                                              }

                                                              else if (defname == "sensor")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace("WARNING: sensor struct missing in '{}'\n",
                                                                             m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseSensor(val);
                                                                }
                                                              }

                                                              else if (defname == "nav")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace("WARNING: nav struct missing in '{}'\n",
                                                                             m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseNavsys(val);
                                                                }
                                                              }

                                                              else if (defname == "computer")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: computer struct missing in '{}'\n",
                                                                    m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseComputer(val);
                                                                }
                                                              }

                                                              else if (defname == "shield")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace("WARNING: shield struct missing in '{}'\n",
                                                                             m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseShield(val);
                                                                }
                                                              }

                                                              else if (defname == "death_spiral")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: death spiral struct missing in '{}'\n",
                                                                    m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseDeathSpiral(val);
                                                                }
                                                              }

                                                              else if (defname == "map")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace("WARNING: map struct missing in '{}'\n",
                                                                             m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseMap(val);
                                                                }
                                                              }

                                                              else if (defname == "squadron")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace(
                                                                    "WARNING: squadron struct missing in '{}'\n",
                                                                    m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseSquadron(val);
                                                                }
                                                              }

                                                              else if (defname == "skin")
                                                              {
                                                                if (!def->term() || !def->term()->isStruct())
                                                                {
                                                                  DebugTrace("WARNING: skin struct missing in '{}'\n",
                                                                             m_filename);
                                                                }
                                                                else
                                                                {
                                                                  TermStruct* val = def->term()->isStruct();
                                                                  ParseSkin(val);
                                                                }
                                                              }

                                                              else
                                                              {
                                                                DebugTrace("WARNING: unknown parameter '{}' in '{}'\n",
                                                                           defname.data(), m_filename);
                                                              }

  if (!description.empty())
    description = Game::GetText(description);
}

void ShipDesign::ParsePower(TermStruct* val)
{
  int stype = 0;
  float output = 1000.0f;
  float fuel = 0.0f;
  Vec3 loc(0.0f, 0.0f, 0.0f);
  float size = 0.0f;
  float hull = 0.5f;
  std::string design_name;
  std::string pname;
  std::string pabrv;
  int etype = 0;
  int emcon_1 = -1;
  int emcon_2 = -1;
  int emcon_3 = -1;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "type")
      {
        if (TermText* tname = pdef->term()->isText())
        {
          if (tname->value()[0] == 'B')
            stype = PowerSource::BATTERY;
          else if (tname->value()[0] == 'A')
            stype = PowerSource::AUX;
          else if (tname->value()[0] == 'F')
            stype = PowerSource::FUSION;
          else
            DebugTrace("WARNING: unknown power source type '{}' in '{}'\n", tname->value().data(), m_filename);
        }
      }

      else if (defname == "name")
        GetDefText(pname, pdef, m_filename);

      else if (defname == "abrv")
        GetDefText(pabrv, pdef, m_filename);

      else if (defname == "design")
        GetDefText(design_name, pdef, m_filename);

      else if (defname == "max_output")
        GetDefNumber(output, pdef, m_filename);
      else if (defname == "fuel_range")
        GetDefNumber(fuel, pdef, m_filename);

      else if (defname == "loc")
      {
        GetDefVec(loc, pdef, m_filename);
        loc *= scale;
      }
      else if (defname == "size")
      {
        GetDefNumber(size, pdef, m_filename);
        size *= scale;
      }
      else if (defname == "hull_factor")
        GetDefNumber(hull, pdef, m_filename);

      else if (defname == "explosion")
        GetDefNumber(etype, pdef, m_filename);

      else if (defname == "emcon_1")
        GetDefNumber(emcon_1, pdef, m_filename);

      else if (defname == "emcon_2")
        GetDefNumber(emcon_2, pdef, m_filename);

      else if (defname == "emcon_3")
        GetDefNumber(emcon_3, pdef, m_filename);
    }
  }

  auto source = NEW PowerSource(static_cast<PowerSource::SUBTYPE>(stype), output);
  if (!pname.empty())
    source->SetName(pname);
  if (!pabrv.empty())
    source->SetName(pabrv);
  source->SetFuelRange(fuel);
  source->Mount(loc, size, hull);
  source->SetExplosionType(etype);

  if (!design_name.empty())
  {
    SystemDesign* sd = SystemDesign::Find(design_name);
    if (sd)
      source->SetDesign(sd);
  }

  if (emcon_1 >= 0 && emcon_1 <= 100)
    source->SetEMCONPower(1, emcon_1);

  if (emcon_2 >= 0 && emcon_2 <= 100)
    source->SetEMCONPower(1, emcon_2);

  if (emcon_3 >= 0 && emcon_3 <= 100)
    source->SetEMCONPower(1, emcon_3);

  reactors.append(source);
}

void ShipDesign::ParseDrive(TermStruct* val)
{
  std::string dname;
  std::string dabrv;
  int dtype = 0;
  int etype = 0;
  float dthrust = 1.0f;
  float daug = 0.0f;
  float dscale = 1.0f;
  Vec3 loc(0.0f, 0.0f, 0.0f);
  float size = 0.0f;
  float hull = 0.5f;
  std::string design_name;
  int emcon_1 = -1;
  int emcon_2 = -1;
  int emcon_3 = -1;
  bool trail = true;
  Drive* drive = nullptr;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "type")
      {
        TermText* tname = pdef->term()->isText();

        if (tname)
        {
          std::string tval = tname->value();

          if (tval == "Plasma")
            dtype = Drive::PLASMA;
          else if (tval == "Fusion")
            dtype = Drive::FUSION;
          else if (tval == "Alien")
            dtype = Drive::GREEN;
          else if (tval == "Green")
            dtype = Drive::GREEN;
          else if (tval == "Red")
            dtype = Drive::RED;
          else if (tval == "Blue")
            dtype = Drive::BLUE;
          else if (tval == "Yellow")
            dtype = Drive::YELLOW;
          else if (tval == "Stealth")
            dtype = Drive::STEALTH;

          else
            DebugTrace("WARNING: unknown drive type '{}' in '{}'\n", tname->value().data(), m_filename);
        }
      }
      else if (defname == "name")
      {
        if (!GetDefText(dname, pdef, m_filename))
          DebugTrace("WARNING: invalid or missing name for drive in '{}'\n", m_filename);
      }

      else if (defname == "abrv")
      {
        if (!GetDefText(dabrv, pdef, m_filename))
          DebugTrace("WARNING: invalid or missing abrv for drive in '{}'\n", m_filename);
      }

      else if (defname == "design")
      {
        if (!GetDefText(design_name, pdef, m_filename))
          DebugTrace("WARNING: invalid or missing design for drive in '{}'\n", m_filename);
      }

      else if (defname == "thrust")
      {
        if (!GetDefNumber(dthrust, pdef, m_filename))
          DebugTrace("WARNING: invalid or missing thrust for drive in '{}'\n", m_filename);
      }

      else if (defname == "augmenter")
      {
        if (!GetDefNumber(daug, pdef, m_filename))
          DebugTrace("WARNING: invalid or missing augmenter for drive in '{}'\n", m_filename);
      }

      else if (defname == "scale")
      {
        if (!GetDefNumber(dscale, pdef, m_filename))
          DebugTrace("WARNING: invalid or missing scale for drive in '{}'\n", m_filename);
      }

      else if (defname == "port")
      {
        Vec3 port;
        float flare_scale = 0;

        if (pdef->term()->isArray())
        {
          GetDefVec(port, pdef, m_filename);
          port *= scale;
          flare_scale = dscale;
        }

        else if (pdef->term()->isStruct())
        {
          TermStruct* struc = pdef->term()->isStruct();

          for (int j = 0; j < struc->elements()->size(); j++)
          {
            TermDef* pdef2 = struc->elements()->at(j)->isDef();
            if (pdef2)
            {
              if (pdef2->name()->value() == "loc")
              {
                GetDefVec(port, pdef2, m_filename);
                port *= scale;
              }

              else if (pdef2->name()->value() == "scale")
                GetDefNumber(flare_scale, pdef2, m_filename);
            }
          }

          if (flare_scale <= 0)
            flare_scale = dscale;
        }

        if (!drive)
          drive = NEW Drive(static_cast<Drive::SUBTYPE>(dtype), dthrust, daug, trail);

        drive->AddPort(port, flare_scale);
      }

      else if (defname == "loc")
      {
        if (!GetDefVec(loc, pdef, m_filename))
          DebugTrace("WARNING: invalid or missing loc for drive in '{}'\n", m_filename);
        loc *= scale;
      }

      else if (defname == "size")
      {
        if (!GetDefNumber(size, pdef, m_filename))
          DebugTrace("WARNING: invalid or missing size for drive in '{}'\n", m_filename);
        size *= scale;
      }

      else if (defname == "hull_factor")
      {
        if (!GetDefNumber(hull, pdef, m_filename))
          DebugTrace("WARNING: invalid or missing hull_factor for drive in '{}'\n", m_filename);
      }

      else if (defname == "explosion")
      {
        if (!GetDefNumber(etype, pdef, m_filename))
          DebugTrace("WARNING: invalid or missing explosion for drive in '{}'\n", m_filename);
      }

      else if (defname == "emcon_1")
        GetDefNumber(emcon_1, pdef, m_filename);

      else if (defname == "emcon_2")
        GetDefNumber(emcon_2, pdef, m_filename);

      else if (defname == "emcon_3")
        GetDefNumber(emcon_3, pdef, m_filename);

      else if (defname == "trail" || defname == "show_trail")
        GetDefBool(trail, pdef, m_filename);
    }
  }

  if (!drive)
    drive = NEW Drive(static_cast<Drive::SUBTYPE>(dtype), dthrust, daug, trail);

  drive->SetSourceIndex(reactors.size() - 1);
  drive->Mount(loc, size, hull);
  if (!dname.empty())
    drive->SetName(dname);
  if (!dabrv.empty())
    drive->SetAbbreviation(dabrv);
  drive->SetExplosionType(etype);

  if (!design_name.empty())
  {
    SystemDesign* sd = SystemDesign::Find(design_name);
    if (sd)
      drive->SetDesign(sd);
  }

  if (emcon_1 >= 0 && emcon_1 <= 100)
    drive->SetEMCONPower(1, emcon_1);

  if (emcon_2 >= 0 && emcon_2 <= 100)
    drive->SetEMCONPower(1, emcon_2);

  if (emcon_3 >= 0 && emcon_3 <= 100)
    drive->SetEMCONPower(1, emcon_3);

  main_drive = drives.size();
  drives.append(drive);
}

void ShipDesign::ParseQuantumDrive(TermStruct* _val)
{
  double capacity = 250e3;
  double consumption = 1e3;
  Vec3 loc(0.0f, 0.0f, 0.0f);
  float size = 0.0f;
  float hull = 0.5f;
  float countdown = 5.0f;
  std::string design_name;
  std::string type_name;
  std::string abrv;
  int subtype = QuantumDrive::QUANTUM;
  int emcon_1 = -1;
  int emcon_2 = -1;
  int emcon_3 = -1;

  for (int i = 0; i < _val->elements()->size(); i++)
  {
    TermDef* pdef = _val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "design")
        GetDefText(design_name, pdef, m_filename);
      else if (defname == "abrv")
        GetDefText(abrv, pdef, m_filename);
      else if (defname == "type")
      {
        GetDefText(type_name, pdef, m_filename);

        if (type_name.contains("hyper"))
          subtype = QuantumDrive::HYPER;
      }
      else if (defname == "capacity")
        GetDefNumber(capacity, pdef, m_filename);
      else if (defname == "consumption")
        GetDefNumber(consumption, pdef, m_filename);
      else if (defname == "loc")
      {
        GetDefVec(loc, pdef, m_filename);
        loc *= scale;
      }
      else if (defname == "size")
      {
        GetDefNumber(size, pdef, m_filename);
        size *= scale;
      }
      else if (defname == "hull_factor")
        GetDefNumber(hull, pdef, m_filename);
      else if (defname == "jump_time")
        GetDefNumber(countdown, pdef, m_filename);
      else if (defname == "countdown")
        GetDefNumber(countdown, pdef, m_filename);

      else if (defname == "emcon_1")
        GetDefNumber(emcon_1, pdef, m_filename);

      else if (defname == "emcon_2")
        GetDefNumber(emcon_2, pdef, m_filename);

      else if (defname == "emcon_3")
        GetDefNumber(emcon_3, pdef, m_filename);
    }
  }

  auto drive = NEW QuantumDrive(static_cast<QuantumDrive::SUBTYPE>(subtype), capacity, consumption);
  drive->SetSourceIndex(reactors.size() - 1);
  drive->Mount(loc, size, hull);
  drive->SetCountdown(countdown);

  if (!design_name.empty())
  {
    SystemDesign* sd = SystemDesign::Find(design_name);
    if (sd)
      drive->SetDesign(sd);
  }

  if (!abrv.empty())
    drive->SetAbbreviation(abrv);

  if (emcon_1 >= 0 && emcon_1 <= 100)
    drive->SetEMCONPower(1, emcon_1);

  if (emcon_2 >= 0 && emcon_2 <= 100)
    drive->SetEMCONPower(1, emcon_2);

  if (emcon_3 >= 0 && emcon_3 <= 100)
    drive->SetEMCONPower(1, emcon_3);

  quantum_drive = drive;
}

void ShipDesign::ParseFarcaster(TermStruct* val)
{
  std::string design_name;
  double capacity = 300e3;
  double consumption = 15e3;  // twenty second recharge
  int napproach = 0;
  Vec3 approach[Farcaster::NUM_APPROACH_PTS];
  Vec3 loc(0.0f, 0.0f, 0.0f);
  Vec3 start(0.0f, 0.0f, 0.0f);
  Vec3 end(0.0f, 0.0f, 0.0f);
  float size = 0.0f;
  float hull = 0.5f;
  int emcon_1 = -1;
  int emcon_2 = -1;
  int emcon_3 = -1;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "design")
        GetDefText(design_name, pdef, m_filename);
      else if (defname == "capacity")
        GetDefNumber(capacity, pdef, m_filename);
      else if (defname == "consumption")
        GetDefNumber(consumption, pdef, m_filename);
      else if (defname == "loc")
      {
        GetDefVec(loc, pdef, m_filename);
        loc *= scale;
      }
      else if (defname == "size")
      {
        GetDefNumber(size, pdef, m_filename);
        size *= scale;
      }
      else if (defname == "hull_factor")
        GetDefNumber(hull, pdef, m_filename);

      else if (defname == "start")
      {
        GetDefVec(start, pdef, m_filename);
        start *= scale;
      }
      else if (defname == "end")
      {
        GetDefVec(end, pdef, m_filename);
        end *= scale;
      }
      else if (defname == "approach")
      {
        if (napproach < Farcaster::NUM_APPROACH_PTS)
        {
          GetDefVec(approach[napproach], pdef, m_filename);
          approach[napproach++] *= scale;
        }
        else
        {
          DebugTrace("WARNING: farcaster approach point ignored in '{}' (max=%d)\n", m_filename,
                     static_cast<int>(Farcaster::NUM_APPROACH_PTS));
        }
      }

      else if (defname == "emcon_1")
        GetDefNumber(emcon_1, pdef, m_filename);

      else if (defname == "emcon_2")
        GetDefNumber(emcon_2, pdef, m_filename);

      else if (defname == "emcon_3")
        GetDefNumber(emcon_3, pdef, m_filename);
    }
  }

  auto caster = NEW Farcaster(capacity, consumption);
  caster->SetSourceIndex(reactors.size() - 1);
  caster->Mount(loc, size, hull);

  if (!design_name.empty())
  {
    SystemDesign* sd = SystemDesign::Find(design_name);
    if (sd)
      caster->SetDesign(sd);
  }

  caster->SetStartPoint(start);
  caster->SetEndPoint(end);

  for (int i = 0; i < napproach; i++)
    caster->SetApproachPoint(i, approach[i]);

  if (emcon_1 >= 0 && emcon_1 <= 100)
    caster->SetEMCONPower(1, emcon_1);

  if (emcon_2 >= 0 && emcon_2 <= 100)
    caster->SetEMCONPower(1, emcon_2);

  if (emcon_3 >= 0 && emcon_3 <= 100)
    caster->SetEMCONPower(1, emcon_3);

  farcaster = caster;
}

void ShipDesign::ParseThruster(TermStruct* val)
{
  if (thruster)
  {
    DebugTrace("WARNING: additional thruster ignored in '{}'\n", m_filename);
    return;
  }

  double thrust = 100;

  Vec3 loc(0.0f, 0.0f, 0.0f);
  float size = 0.0f;
  float hull = 0.5f;
  std::string design_name;
  float tscale = 1.0f;
  int emcon_1 = -1;
  int emcon_2 = -1;
  int emcon_3 = -1;
  int dtype = 0;

  Thruster* drive = nullptr;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "type")
      {
        TermText* tname = pdef->term()->isText();

        if (tname)
        {
          std::string tval = tname->value();

          if (tval == "Plasma")
            dtype = Drive::PLASMA;
          else if (tval == "Fusion")
            dtype = Drive::FUSION;
          else if (tval == "Alien")
            dtype = Drive::GREEN;
          else if (tval == "Green")
            dtype = Drive::GREEN;
          else if (tval == "Red")
            dtype = Drive::RED;
          else if (tval == "Blue")
            dtype = Drive::BLUE;
          else if (tval == "Yellow")
            dtype = Drive::YELLOW;
          else if (tval == "Stealth")
            dtype = Drive::STEALTH;

          else
            DebugTrace("WARNING: unknown thruster type '{}' in '{}'\n", tname->value().data(), m_filename);
        }
      }

      else if (defname == "thrust")
        GetDefNumber(thrust, pdef, m_filename);

      else if (defname == "design")
        GetDefText(design_name, pdef, m_filename);

      else if (defname == "loc")
      {
        GetDefVec(loc, pdef, m_filename);
        loc *= scale;
      }
      else if (defname == "size")
      {
        GetDefNumber(size, pdef, m_filename);
        size *= scale;
      }
      else if (defname == "hull_factor")
        GetDefNumber(hull, pdef, m_filename);
      else if (defname == "scale")
        GetDefNumber(tscale, pdef, m_filename);
      else if (defname.contains("port") && pdef->term())
      {
        Vec3 port;
        float port_scale = 0;
        DWORD fire = 0;

        if (pdef->term()->isArray())
        {
          GetDefVec(port, pdef, m_filename);
          port *= scale;
          port_scale = tscale;
        }

        else if (pdef->term()->isStruct())
        {
          TermStruct* val = pdef->term()->isStruct();

          for (int j = 0; j < val->elements()->size(); j++)
          {
            TermDef* pdef2 = val->elements()->at(j)->isDef();
            if (pdef2)
            {
              if (pdef2->name()->value() == "loc")
              {
                GetDefVec(port, pdef2, m_filename);
                port *= scale;
              }

              else if (pdef2->name()->value() == "fire")
                GetDefNumber(fire, pdef2, m_filename);

              else if (pdef2->name()->value() == "scale")
                GetDefNumber(port_scale, pdef2, m_filename);
            }
          }

          if (port_scale <= 0)
            port_scale = tscale;
        }

        if (!drive)
          drive = NEW Thruster(dtype, thrust, tscale);

        if (defname == "port" || defname == "port_bottom")
          drive->AddPort(Thruster::BOTTOM, port, fire, port_scale);

        else if (defname == "port_top")
          drive->AddPort(Thruster::TOP, port, fire, port_scale);

        else if (defname == "port_left")
          drive->AddPort(Thruster::LEFT, port, fire, port_scale);

        else if (defname == "port_right")
          drive->AddPort(Thruster::RIGHT, port, fire, port_scale);

        else if (defname == "port_fore")
          drive->AddPort(Thruster::FORE, port, fire, port_scale);

        else if (defname == "port_aft")
          drive->AddPort(Thruster::AFT, port, fire, port_scale);
      }

      else if (defname == "emcon_1")
        GetDefNumber(emcon_1, pdef, m_filename);

      else if (defname == "emcon_2")
        GetDefNumber(emcon_2, pdef, m_filename);

      else if (defname == "emcon_3")
        GetDefNumber(emcon_3, pdef, m_filename);
    }
  }

  if (!drive)
    drive = NEW Thruster(dtype, thrust, tscale);
  drive->SetSourceIndex(reactors.size() - 1);
  drive->Mount(loc, size, hull);

  if (!design_name.empty())
  {
    SystemDesign* sd = SystemDesign::Find(design_name);
    if (sd)
      drive->SetDesign(sd);
  }

  if (emcon_1 >= 0 && emcon_1 <= 100)
    drive->SetEMCONPower(1, emcon_1);

  if (emcon_2 >= 0 && emcon_2 <= 100)
    drive->SetEMCONPower(1, emcon_2);

  if (emcon_3 >= 0 && emcon_3 <= 100)
    drive->SetEMCONPower(1, emcon_3);

  thruster = drive;
}

void ShipDesign::ParseNavlight(TermStruct* val)
{
  std::string dname;
  std::string dabrv;
  std::string design_name;
  int nlights = 0;
  float dscale = 1.0f;
  float period = 10.0f;
  Vec3 bloc[NavLight::MAX_LIGHTS];
  int btype[NavLight::MAX_LIGHTS];
  DWORD pattern[NavLight::MAX_LIGHTS];

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "name")
        GetDefText(dname, pdef, m_filename);
      else if (defname == "abrv")
        GetDefText(dabrv, pdef, m_filename);

      else if (defname == "design")
        GetDefText(design_name, pdef, m_filename);

      else if (defname == "scale")
        GetDefNumber(dscale, pdef, m_filename);
      else if (defname == "period")
        GetDefNumber(period, pdef, m_filename);
      else if (defname == "light")
      {
        if (!pdef->term() || !pdef->term()->isStruct())
          DebugTrace("WARNING: light struct missing for ship '{}' in '{}'\n", m_name, m_filename);
        else
        {
          TermStruct* val = pdef->term()->isStruct();

          Vec3 loc;
          int t = 0;
          DWORD ptn = 0;

          for (int j = 0; j < val->elements()->size(); j++)
          {
            if (TermDef* def = val->elements()->at(j)->isDef())
            {
              std::string defval = def->name()->value();

              if (defval == "type")
                GetDefNumber(t, def, m_filename);
              else if (defval == "loc")
                GetDefVec(loc, def, m_filename);
              else if (defval == "pattern")
                GetDefNumber(ptn, def, m_filename);
            }
          }

          if (t < 1 || t > 4)
            t = 1;

          if (nlights < NavLight::MAX_LIGHTS)
          {
            bloc[nlights] = loc * scale;
            btype[nlights] = t - 1;
            pattern[nlights] = ptn;
            nlights++;
          }
          else
            DebugTrace("WARNING: Too many lights ship '{}' in '{}'\n", m_name, m_filename);
        }
      }
    }
  }

  auto nav = NEW NavLight(period, dscale);
  if (!dname.empty())
    nav->SetName(dname);
  if (!dabrv.empty())
    nav->SetAbbreviation(dabrv);

  if (!design_name.empty())
  {
    SystemDesign* sd = SystemDesign::Find(design_name);
    if (sd)
      nav->SetDesign(sd);
  }

  for (int i = 0; i < nlights; i++)
    nav->AddBeacon(bloc[i], pattern[i], btype[i]);

  navlights.append(nav);
}

void ShipDesign::ParseFlightDeck(TermStruct* val)
{
  std::string dname;
  std::string dabrv;
  std::string design_name;
  float az = 0.0f;
  int etype = 0;

  bool launch = false;
  bool recovery = false;
  int nslots = 0;
  int napproach = 0;
  int nrunway = 0;
  DWORD filters[10];
  Vec3 spots[10];
  Vec3 approach[FlightDeck::NUM_APPROACH_PTS];
  Vec3 runway[2];
  Vec3 loc(0, 0, 0);
  Vec3 start(0, 0, 0);
  Vec3 end(0, 0, 0);
  Vec3 cam(0, 0, 0);
  Vec3 box(0, 0, 0);
  float cycle_time = 0.0f;
  float size = 0.0f;
  float hull = 0.5f;

  float light = 0.0f;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "name")
        GetDefText(dname, pdef, m_filename);
      else if (defname == "abrv")
        GetDefText(dabrv, pdef, m_filename);
      else if (defname == "design")
        GetDefText(design_name, pdef, m_filename);

      else if (defname == "start")
      {
        GetDefVec(start, pdef, m_filename);
        start *= scale;
      }
      else if (defname == "end")
      {
        GetDefVec(end, pdef, m_filename);
        end *= scale;
      }
      else if (defname == "cam")
      {
        GetDefVec(cam, pdef, m_filename);
        cam *= scale;
      }
      else if (defname == "box" || defname == "bounding_box")
      {
        GetDefVec(box, pdef, m_filename);
        box *= scale;
      }
      else if (defname == "approach")
      {
        if (napproach < FlightDeck::NUM_APPROACH_PTS)
        {
          GetDefVec(approach[napproach], pdef, m_filename);
          approach[napproach++] *= scale;
        }
        else
        {
          DebugTrace("WARNING: flight deck approach point ignored in '{}' (max=%d)\n", m_filename,
                     static_cast<int>(FlightDeck::NUM_APPROACH_PTS));
        }
      }
      else if (defname == "runway")
      {
        GetDefVec(runway[nrunway], pdef, m_filename);
        runway[nrunway++] *= scale;
      }
      else if (defname == "spot")
      {
        if (pdef->term()->isStruct())
        {
          TermStruct* s = pdef->term()->isStruct();
          for (int j = 0; j < s->elements()->size(); j++)
          {
            TermDef* d = s->elements()->at(j)->isDef();
            if (d)
            {
              if (d->name()->value() == "loc")
              {
                GetDefVec(spots[nslots], d, m_filename);
                spots[nslots] *= scale;
              }
              else if (d->name()->value() == "filter")
                GetDefNumber(filters[nslots], d, m_filename);
            }
          }

          nslots++;
        }

        else if (pdef->term()->isArray())
        {
          GetDefVec(spots[nslots], pdef, m_filename);
          spots[nslots] *= scale;
          filters[nslots++] = 0xf;
        }
      }

      else if (defname == "light")
        GetDefNumber(light, pdef, m_filename);

      else if (defname == "cycle_time")
        GetDefNumber(cycle_time, pdef, m_filename);

      else if (defname == "launch")
        GetDefBool(launch, pdef, m_filename);

      else if (defname == "recovery")
        GetDefBool(recovery, pdef, m_filename);

      else if (defname == "azimuth")
      {
        GetDefNumber(az, pdef, m_filename);
        if (degrees)
          az *= static_cast<float>(DEGREES);
      }

      else if (defname == "loc")
      {
        GetDefVec(loc, pdef, m_filename);
        loc *= scale;
      }
      else if (defname == "size")
      {
        GetDefNumber(size, pdef, m_filename);
        size *= scale;
      }
      else if (defname == "hull_factor")
        GetDefNumber(hull, pdef, m_filename);
      else if (defname == "explosion")
        GetDefNumber(etype, pdef, m_filename);
    }
  }

  auto deck = NEW FlightDeck();
  deck->Mount(loc, size, hull);
  if (!dname.empty())
    deck->SetName(dname);
  if (!dabrv.empty())
    deck->SetAbbreviation(dabrv);

  if (!design_name.empty())
  {
    SystemDesign* sd = SystemDesign::Find(design_name);
    if (sd)
      deck->SetDesign(sd);
  }

  if (launch)
    deck->SetLaunchDeck();
  else if (recovery)
    deck->SetRecoveryDeck();

  deck->SetAzimuth(az);
  deck->SetBoundingBox(box);
  deck->SetStartPoint(start);
  deck->SetEndPoint(end);
  deck->SetCamLoc(cam);
  deck->SetExplosionType(etype);

  if (light > 0)
    deck->SetLight(light);

  for (int i = 0; i < napproach; i++)
    deck->SetApproachPoint(i, approach[i]);

  for (int i = 0; i < nrunway; i++)
    deck->SetRunwayPoint(i, runway[i]);

  for (int i = 0; i < nslots; i++)
    deck->AddSlot(spots[i], filters[i]);

  if (cycle_time > 0)
    deck->SetCycleTime(cycle_time);

  flight_decks.append(deck);
}

void ShipDesign::ParseLandingGear(TermStruct* val)
{
  std::string dname;
  std::string dabrv;
  std::string design_name;
  int ngear = 0;
  Vec3 start[LandingGear::MAX_GEAR];
  Vec3 end[LandingGear::MAX_GEAR];
  Model* model[LandingGear::MAX_GEAR];

  for (int i = 0; i < val->elements()->size(); i++)
  {
    if (TermDef* pdef = val->elements()->at(i)->isDef())
    {
      std::string defname = pdef->name()->value();

      if (defname == "name")
        GetDefText(dname, pdef, m_filename);
      else if (defname == "abrv")
        GetDefText(dabrv, pdef, m_filename);

      else if (defname == "design")
        GetDefText(design_name, pdef, m_filename);

      else if (defname == "gear")
      {
        if (!pdef->term() || !pdef->term()->isStruct())
          DebugTrace("WARNING: gear struct missing for ship '{}' in '{}'\n", m_name, m_filename);
        else
        {
          TermStruct* struc = pdef->term()->isStruct();

          Vec3 v1, v2;
          std::string modName;

          for (int j = 0; j < struc->elements()->size(); j++)
          {
            if (TermDef* def = struc->elements()->at(j)->isDef())
            {
              defname = def->name()->value();

              if (defname == "model")
                GetDefText(modName, def, m_filename);
              else if (defname == "start")
                GetDefVec(v1, def, m_filename);
              else if (defname == "end")
                GetDefVec(v2, def, m_filename);
            }
          }

          if (ngear < LandingGear::MAX_GEAR)
          {
            auto m = NEW Model;
            if (!m->Load(modName, scale))
            {
              DebugTrace("WARNING: Could not load landing gear model '{}'\n", modName);
              delete m;
              m = nullptr;
            }
            else
            {
              model[ngear] = m;
              start[ngear] = v1 * scale;
              end[ngear] = v2 * scale;
              ngear++;
            }
          }
          else
            DebugTrace("WARNING: Too many landing gear ship '{}' in '{}'\n", m_name, m_filename);
        }
      }
    }
  }

  gear = NEW LandingGear();
  if (!dname.empty())
    gear->SetName(dname);
  if (!dabrv.empty())
    gear->SetAbbreviation(dabrv);

  if (!design_name.empty())
  {
    SystemDesign* sd = SystemDesign::Find(design_name);
    if (sd)
      gear->SetDesign(sd);
  }

  for (int i = 0; i < ngear; i++)
    gear->AddGear(model[i], start[i], end[i]);
}

void ShipDesign::ParseWeapon(TermStruct* val)
{
  std::string wtype;
  std::string wname;
  std::string wabrv;
  std::string design_name;
  std::string group_name;

  int nmuz = 0;
  Vec3 muzzles[Weapon::MAX_BARRELS];
  Vec3 loc(0.0f, 0.0f, 0.0f);
  float size = 0.0f;
  float hull = 0.5f;
  float az = 0.0f;
  float el = 0.0f;
  float az_max = 1e6f;
  float az_min = 1e6f;
  float el_max = 1e6f;
  float el_min = 1e6f;
  float az_rest = 1e6f;
  float el_rest = 1e6f;
  int etype = 0;
  int emcon_1 = -1;
  int emcon_2 = -1;
  int emcon_3 = -1;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "type")
        GetDefText(wtype, pdef, m_filename);
      else if (defname == "name")
        GetDefText(wname, pdef, m_filename);
      else if (defname == "abrv")
        GetDefText(wabrv, pdef, m_filename);
      else if (defname == "design")
        GetDefText(design_name, pdef, m_filename);
      else if (defname == "group")
        GetDefText(group_name, pdef, m_filename);

      else if (defname == "muzzle")
      {
        if (nmuz < Weapon::MAX_BARRELS)
        {
          GetDefVec(muzzles[nmuz], pdef, m_filename);
          nmuz++;
        }
        else
        {
          DebugTrace("WARNING: too many muzzles (max=%d) for weapon in '{}'\n", m_filename,
                     static_cast<int>(Weapon::MAX_BARRELS));
        }
      }
      else if (defname == "loc")
      {
        GetDefVec(loc, pdef, m_filename);
        loc *= scale;
      }
      else if (defname == "size")
      {
        GetDefNumber(size, pdef, m_filename);
        size *= scale;
      }
      else if (defname == "hull_factor")
        GetDefNumber(hull, pdef, m_filename);
      else if (defname == "azimuth")
      {
        GetDefNumber(az, pdef, m_filename);
        if (degrees)
          az *= static_cast<float>(DEGREES);
      }
      else if (defname == "elevation")
      {
        GetDefNumber(el, pdef, m_filename);
        if (degrees)
          el *= static_cast<float>(DEGREES);
      }

      else if (defname == "aim_az_max")
      {
        GetDefNumber(az_max, pdef, m_filename);
        if (degrees)
          az_max *= static_cast<float>(DEGREES);
        az_min = 0.0f - az_max;
      }

      else if (defname == "aim_el_max")
      {
        GetDefNumber(el_max, pdef, m_filename);
        if (degrees)
          el_max *= static_cast<float>(DEGREES);
        el_min = 0.0f - el_max;
      }

      else if (defname == "aim_az_min")
      {
        GetDefNumber(az_min, pdef, m_filename);
        if (degrees)
          az_min *= static_cast<float>(DEGREES);
      }

      else if (defname == "aim_el_min")
      {
        GetDefNumber(el_min, pdef, m_filename);
        if (degrees)
          el_min *= static_cast<float>(DEGREES);
      }

      else if (defname == "aim_az_rest")
      {
        GetDefNumber(az_rest, pdef, m_filename);
        if (degrees)
          az_rest *= static_cast<float>(DEGREES);
      }

      else if (defname == "aim_el_rest")
      {
        GetDefNumber(el_rest, pdef, m_filename);
        if (degrees)
          el_rest *= static_cast<float>(DEGREES);
      }

      else if (defname == "rest_azimuth")
      {
        GetDefNumber(az_rest, pdef, m_filename);
        if (degrees)
          az_rest *= static_cast<float>(DEGREES);
      }
      else if (defname == "rest_elevation")
      {
        GetDefNumber(el_rest, pdef, m_filename);
        if (degrees)
          el_rest *= static_cast<float>(DEGREES);
      }
      else if (defname == "explosion")
        GetDefNumber(etype, pdef, m_filename);

      else if (defname == "emcon_1")
        GetDefNumber(emcon_1, pdef, m_filename);

      else if (defname == "emcon_2")
        GetDefNumber(emcon_2, pdef, m_filename);

      else if (defname == "emcon_3")
        GetDefNumber(emcon_3, pdef, m_filename);
      else
        DebugTrace("WARNING: unknown weapon parameter '{}' in '{}'\n", defname.data(), m_filename);
    }
  }

  WeaponDesign* meta = WeaponDesign::Find(wtype);
  if (!meta)
    DebugTrace("WARNING: unusual weapon name '{}' in '{}'\n", wtype, m_filename);
  else
  {
    // non-turret weapon muzzles are relative to ship scale:
    if (meta->turret_model == nullptr)
    {
      for (int i = 0; i < nmuz; i++)
        muzzles[i] *= scale;
    }

    // turret weapon muzzles are relative to weapon scale:
    else
    {
      for (int i = 0; i < nmuz; i++)
        muzzles[i] *= meta->scale;
    }

    auto gun = NEW Weapon(meta, nmuz, muzzles, az, el);
    gun->SetSourceIndex(reactors.size() - 1);
    gun->Mount(loc, size, hull);

    if (az_max < 1e6)
      gun->SetAzimuthMax(az_max);
    if (az_min < 1e6)
      gun->SetAzimuthMin(az_min);
    if (az_rest < 1e6)
      gun->SetRestAzimuth(az_rest);

    if (el_max < 1e6)
      gun->SetElevationMax(el_max);
    if (el_min < 1e6)
      gun->SetElevationMin(el_min);
    if (el_rest < 1e6)
      gun->SetRestElevation(el_rest);

    if (emcon_1 >= 0 && emcon_1 <= 100)
      gun->SetEMCONPower(1, emcon_1);

    if (emcon_2 >= 0 && emcon_2 <= 100)
      gun->SetEMCONPower(1, emcon_2);

    if (emcon_3 >= 0 && emcon_3 <= 100)
      gun->SetEMCONPower(1, emcon_3);

    if (!wname.empty())
      gun->SetName(wname);
    if (!wabrv.empty())
      gun->SetAbbreviation(wabrv);

    if (!design_name.empty())
    {
      SystemDesign* sd = SystemDesign::Find(design_name);
      if (sd)
        gun->SetDesign(sd);
    }

    if (!group_name.empty())
      gun->SetGroup(group_name);

    gun->SetExplosionType(etype);

    if (meta->decoy_type && !decoy)
      decoy = gun;
    else if (meta->probe && !probe)
      probe = gun;
    else
      weapons.append(gun);
  }

  DataLoader* loader = DataLoader::GetLoader();
  loader->SetDataPath(path_name);
}

void ShipDesign::ParseHardPoint(TermStruct* val)
{
  std::string wtypes[8];
  std::string wname;
  std::string wabrv;
  std::string design;
  Vec3 muzzle;
  Vec3 loc(0.0f, 0.0f, 0.0f);
  float size = 0.0f;
  float hull = 0.5f;
  float az = 0.0f;
  float el = 0.0f;
  int ntypes = 0;
  int emcon_1 = -1;
  int emcon_2 = -1;
  int emcon_3 = -1;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "type")
        GetDefText(wtypes[ntypes++], pdef, m_filename);
      else if (defname == "name")
        GetDefText(wname, pdef, m_filename);
      else if (defname == "abrv")
        GetDefText(wabrv, pdef, m_filename);
      else if (defname == "design")
        GetDefText(design, pdef, m_filename);

      else if (defname == "muzzle")
      {
        GetDefVec(muzzle, pdef, m_filename);
        muzzle *= scale;
      }
      else if (defname == "loc")
      {
        GetDefVec(loc, pdef, m_filename);
        loc *= scale;
      }
      else if (defname == "size")
      {
        GetDefNumber(size, pdef, m_filename);
        size *= scale;
      }
      else if (defname == "hull_factor")
        GetDefNumber(hull, pdef, m_filename);
      else if (defname == "azimuth")
      {
        GetDefNumber(az, pdef, m_filename);
        if (degrees)
          az *= static_cast<float>(DEGREES);
      }
      else if (defname == "elevation")
      {
        GetDefNumber(el, pdef, m_filename);
        if (degrees)
          el *= static_cast<float>(DEGREES);
      }

      else if (defname == "emcon_1")
        GetDefNumber(emcon_1, pdef, m_filename);

      else if (defname == "emcon_2")
        GetDefNumber(emcon_2, pdef, m_filename);

      else if (defname == "emcon_3")
        GetDefNumber(emcon_3, pdef, m_filename);
      else
        DebugTrace("WARNING: unknown weapon parameter '{}' in '{}'\n", defname.data(), m_filename);
    }
  }

  auto hp = NEW HardPoint(muzzle, az, el);
  if (hp)
  {
    for (int i = 0; i < ntypes; i++)
    {
      WeaponDesign* meta = WeaponDesign::Find(wtypes[i]);
      if (!meta)
        DebugTrace("WARNING: unusual weapon name '{}' in '{}'\n", wtypes[i], m_filename);
      else
        hp->AddDesign(meta);
    }

    hp->Mount(loc, size, hull);
    if (!wname.empty())
      hp->SetName(wname);
    if (!wabrv.empty())
      hp->SetAbbreviation(wabrv);
    if (!design.empty())
      hp->SetDesign(design);

    hard_points.append(hp);
  }

  DataLoader* loader = DataLoader::GetLoader();
  loader->SetDataPath(path_name);
}

void ShipDesign::ParseLoadout(TermStruct* val)
{
  auto load = NEW ShipLoad;

  if (!load)
    return;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "name")
        GetDefText(load->name, pdef, m_filename);

      else if (defname == "stations")
        GetDefArray(load->load, 16, pdef, m_filename);

      else
        DebugTrace("WARNING: unknown loadout parameter '{}' in '{}'\n", defname.data(), m_filename);
    }
  }

  loadouts.append(load);
}

void ShipDesign::ParseSensor(TermStruct* val)
{
  std::string design_name;
  Vec3 loc(0.0f, 0.0f, 0.0f);
  float size = 0.0f;
  float hull = 0.5f;
  int nranges = 0;
  float ranges[8];
  int emcon_1 = -1;
  int emcon_2 = -1;
  int emcon_3 = -1;

  ZeroMemory(ranges, sizeof(ranges));

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "range")
        GetDefNumber(ranges[nranges++], pdef, m_filename);
      else if (defname == "loc")
      {
        GetDefVec(loc, pdef, m_filename);
        loc *= scale;
      }
      else if (defname == "size")
      {
        size *= scale;
        GetDefNumber(size, pdef, m_filename);
      }
      else if (defname == "hull_factor")
        GetDefNumber(hull, pdef, m_filename);
      else if (defname == "design")
        GetDefText(design_name, pdef, m_filename);
      else if (defname == "emcon_1")
        GetDefNumber(emcon_1, pdef, m_filename);

      else if (defname == "emcon_2")
        GetDefNumber(emcon_2, pdef, m_filename);

      else if (defname == "emcon_3")
        GetDefNumber(emcon_3, pdef, m_filename);
    }
  }

  if (!sensor)
  {
    sensor = NEW Sensor();

    if (!design_name.empty())
    {
      SystemDesign* sd = SystemDesign::Find(design_name);
      if (sd)
        sensor->SetDesign(sd);
    }

    for (int i = 0; i < nranges; i++)
      sensor->AddRange(ranges[i]);

    if (emcon_1 >= 0 && emcon_1 <= 100)
      sensor->SetEMCONPower(1, emcon_1);

    if (emcon_2 >= 0 && emcon_2 <= 100)
      sensor->SetEMCONPower(1, emcon_2);

    if (emcon_3 >= 0 && emcon_3 <= 100)
      sensor->SetEMCONPower(1, emcon_3);

    sensor->Mount(loc, size, hull);
    sensor->SetSourceIndex(reactors.size() - 1);
  }
  else
    DebugTrace("WARNING: additional sensor ignored in '{}'\n", m_filename);
}

void ShipDesign::ParseNavsys(TermStruct* val)
{
  std::string design_name;
  Vec3 loc(0.0f, 0.0f, 0.0f);
  float size = 0.0f;
  float hull = 0.5f;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    if (TermDef* pdef = val->elements()->at(i)->isDef())
    {
      std::string defname = pdef->name()->value();

      if (defname == "loc")
      {
        GetDefVec(loc, pdef, m_filename);
        loc *= scale;
      }
      else if (defname == "size")
      {
        size *= scale;
        GetDefNumber(size, pdef, m_filename);
      }
      else if (defname == "hull_factor")
        GetDefNumber(hull, pdef, m_filename);
      else if (defname == "design")
        GetDefText(design_name, pdef, m_filename);
    }
  }

  if (!navsys)
  {
    navsys = NEW NavSystem;

    if (!design_name.empty())
    {
      SystemDesign* sd = SystemDesign::Find(design_name);
      if (sd)
        navsys->SetDesign(sd);
    }

    navsys->Mount(loc, size, hull);
    navsys->SetSourceIndex(reactors.size() - 1);
  }
  else
    DebugTrace("WARNING: additional nav system ignored in '{}'\n", m_filename);
}

void ShipDesign::ParseComputer(TermStruct* val)
{
  std::string comp_name("Computer");
  std::string comp_abrv("Comp");
  std::string design_name;
  int comp_type = 1;
  Vec3 loc(0.0f, 0.0f, 0.0f);
  float size = 0.0f;
  float hull = 0.5f;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "name")
        GetDefText(comp_name, pdef, m_filename);
      else if (defname == "abrv")
        GetDefText(comp_abrv, pdef, m_filename);
      else if (defname == "design")
        GetDefText(design_name, pdef, m_filename);
      else if (defname == "type")
        GetDefNumber(comp_type, pdef, m_filename);
      else if (defname == "loc")
      {
        GetDefVec(loc, pdef, m_filename);
        loc *= scale;
      }
      else if (defname == "size")
      {
        size *= scale;
        GetDefNumber(size, pdef, m_filename);
      }
      else if (defname == "hull_factor")
        GetDefNumber(hull, pdef, m_filename);
    }
  }

  auto comp = NEW Computer(comp_type, comp_name);
  comp->Mount(loc, size, hull);
  comp->SetAbbreviation(comp_abrv);
  comp->SetSourceIndex(reactors.size() - 1);

  if (!design_name.empty())
  {
    SystemDesign* sd = SystemDesign::Find(design_name);
    if (sd)
      comp->SetDesign(sd);
  }

  computers.append(comp);
}

void ShipDesign::ParseShield(TermStruct* val)
{
  std::string dname;
  std::string dabrv;
  std::string design_name;
  std::string model_name;
  double factor = 0;
  double capacity = 0;
  double consumption = 0;
  double cutoff = 0;
  double curve = 0;
  double def_cost = 1;
  int shield_type = 0;
  Vec3 loc(0.0f, 0.0f, 0.0f);
  float size = 0.0f;
  float hull = 0.5f;
  int etype = 0;
  bool shield_capacitor = false;
  bool shield_bubble = false;
  int emcon_1 = -1;
  int emcon_2 = -1;
  int emcon_3 = -1;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "type")
        GetDefNumber(shield_type, pdef, m_filename);
      else if (defname == "name")
        GetDefText(dname, pdef, m_filename);
      else if (defname == "abrv")
        GetDefText(dabrv, pdef, m_filename);
      else if (defname == "design")
        GetDefText(design_name, pdef, m_filename);
      else if (defname == "model")
        GetDefText(model_name, pdef, m_filename);

      else if (defname == "loc")
      {
        GetDefVec(loc, pdef, m_filename);
        loc *= scale;
      }
      else if (defname == "size")
      {
        GetDefNumber(size, pdef, m_filename);
        size *= scale;
      }
      else if (defname == "hull_factor")
        GetDefNumber(hull, pdef, m_filename);

      else if (defname.contains("factor"))
        GetDefNumber(factor, pdef, m_filename);
      else if (defname.contains("cutoff"))
        GetDefNumber(cutoff, pdef, m_filename);
      else if (defname.contains("curve"))
        GetDefNumber(curve, pdef, m_filename);
      else if (defname.contains("capacitor"))
        GetDefBool(shield_capacitor, pdef, m_filename);
      else if (defname.contains("bubble"))
        GetDefBool(shield_bubble, pdef, m_filename);
      else if (defname == "capacity")
        GetDefNumber(capacity, pdef, m_filename);
      else if (defname == "consumption")
        GetDefNumber(consumption, pdef, m_filename);
      else if (defname == "deflection_cost")
        GetDefNumber(def_cost, pdef, m_filename);
      else if (defname == "explosion")
        GetDefNumber(etype, pdef, m_filename);

      else if (defname == "emcon_1")
        GetDefNumber(emcon_1, pdef, m_filename);

      else if (defname == "emcon_2")
        GetDefNumber(emcon_2, pdef, m_filename);

      else if (defname == "emcon_3")
        GetDefNumber(emcon_3, pdef, m_filename);

      else if (defname == "bolt_hit_sound")
        GetDefText(bolt_hit_sound, pdef, m_filename);

      else if (defname == "beam_hit_sound")
        GetDefText(beam_hit_sound, pdef, m_filename);
    }
  }

  if (!shield)
  {
    if (shield_type)
    {
      shield = NEW Shield(static_cast<Shield::SUBTYPE>(shield_type));
      shield->SetSourceIndex(reactors.size() - 1);
      shield->Mount(loc, size, hull);
      if (!dname.empty())
        shield->SetName(dname);
      if (!dabrv.empty())
        shield->SetAbbreviation(dabrv);

      if (!design_name.empty())
      {
        SystemDesign* sd = SystemDesign::Find(design_name);
        if (sd)
          shield->SetDesign(sd);
      }

      shield->SetExplosionType(etype);
      shield->SetShieldCapacitor(shield_capacitor);
      shield->SetShieldBubble(shield_bubble);

      if (factor > 0)
        shield->SetShieldFactor(factor);
      if (capacity > 0)
        shield->SetCapacity(capacity);
      if (cutoff > 0)
        shield->SetShieldCutoff(cutoff);
      if (consumption > 0)
        shield->SetConsumption(consumption);
      if (def_cost > 0)
        shield->SetDeflectionCost(def_cost);
      if (curve > 0)
        shield->SetShieldCurve(curve);

      if (emcon_1 >= 0 && emcon_1 <= 100)
        shield->SetEMCONPower(1, emcon_1);

      if (emcon_2 >= 0 && emcon_2 <= 100)
        shield->SetEMCONPower(1, emcon_2);

      if (emcon_3 >= 0 && emcon_3 <= 100)
        shield->SetEMCONPower(1, emcon_3);

      if (!model_name.empty())
      {
        shield_model = NEW Model;
        if (!shield_model->Load(model_name, scale))
        {
          DebugTrace("ERROR: Could not load shield model '{}'\n", model_name.data());
          delete shield_model;
          shield_model = nullptr;
          valid = false;
        }
        else
        {
          shield_model->SetDynamic(true);
          shield_model->SetLuminous(true);
        }
      }

      DataLoader* loader = DataLoader::GetLoader();
      DWORD SOUND_FLAGS = Sound::LOCALIZED | Sound::LOC_3D;

      if (!bolt_hit_sound.empty())
      {
        if (!loader->LoadSound(bolt_hit_sound, bolt_hit_sound_resource, SOUND_FLAGS, true))
        {
          loader->SetDataPath("Sounds/");
          loader->LoadSound(bolt_hit_sound, bolt_hit_sound_resource, SOUND_FLAGS);
          loader->SetDataPath(path_name);
        }
      }

      if (!beam_hit_sound.empty())
      {
        if (!loader->LoadSound(beam_hit_sound, beam_hit_sound_resource, SOUND_FLAGS, true))
        {
          loader->SetDataPath("Sounds/");
          loader->LoadSound(beam_hit_sound, beam_hit_sound_resource, SOUND_FLAGS);
          loader->SetDataPath(path_name);
        }
      }
    }
    else
      DebugTrace("WARNING: invalid shield type in '{}'\n", m_filename);
  }
  else
    DebugTrace("WARNING: additional shield ignored in '{}'\n", m_filename);
}

void ShipDesign::ParseDeathSpiral(TermStruct* _val)
{
  int exp_index = -1;
  int debris_index = -1;
  int fire_index = -1;

  for (int i = 0; i < _val->elements()->size(); i++)
  {
    TermDef* def = _val->elements()->at(i)->isDef();
    if (def)
    {
      std::string defname = def->name()->value();

      if (defname == "time")
        GetDefNumber(death_spiral_time, def, m_filename);

      else if (defname == "explosion")
      {
        if (!def->term() || !def->term()->isStruct())
          DebugTrace("WARNING: explosion struct missing in '{}'\n", m_filename);
        else
        {
          TermStruct* val = def->term()->isStruct();
          ParseExplosion(val, ++exp_index);
        }
      }

      // BACKWARD COMPATIBILITY:
      else if (defname == "explosion_type")
        GetDefNumber(explosion[++exp_index].type, def, m_filename);

      else if (defname == "explosion_time")
        GetDefNumber(explosion[exp_index].time, def, m_filename);

      else if (defname == "explosion_loc")
      {
        GetDefVec(explosion[exp_index].loc, def, m_filename);
        explosion[exp_index].loc *= scale;
      }

      else if (defname == "final_type")
      {
        GetDefNumber(explosion[++exp_index].type, def, m_filename);
        explosion[exp_index].final = true;
      }

      else if (defname == "final_loc")
      {
        GetDefVec(explosion[exp_index].loc, def, m_filename);
        explosion[exp_index].loc *= scale;
      }

      else if (defname == "debris")
      {
        if (def->term() && def->term()->isText())
        {
          std::string model_name;
          GetDefText(model_name, def, m_filename);
          auto model = NEW Model;
          if (!model->Load(model_name, scale))
          {
            DebugTrace("Could not load debris model '{}'\n", model_name.data());
            delete model;
            return;
          }

          PrepareModel(*model);
          debris[++debris_index].model = model;
          fire_index = -1;
        }
        else if (!def->term() || !def->term()->isStruct())
          DebugTrace("WARNING: debris struct missing in '{}'\n", m_filename);
        else
        {
          TermStruct* val = def->term()->isStruct();
          ParseDebris(val, ++debris_index);
        }
      }

      else if (defname == "debris_mass")
        GetDefNumber(debris[debris_index].mass, def, m_filename);

      else if (defname == "debris_speed")
        GetDefNumber(debris[debris_index].speed, def, m_filename);

      else if (defname == "debris_drag")
        GetDefNumber(debris[debris_index].drag, def, m_filename);

      else if (defname == "debris_loc")
      {
        GetDefVec(debris[debris_index].loc, def, m_filename);
        debris[debris_index].loc *= scale;
      }

      else if (defname == "debris_count")
        GetDefNumber(debris[debris_index].count, def, m_filename);

      else if (defname == "debris_life")
        GetDefNumber(debris[debris_index].life, def, m_filename);

      else if (defname == "debris_fire")
      {
        if (++fire_index < 5)
        {
          GetDefVec(debris[debris_index].fire_loc[fire_index], def, m_filename);
          debris[debris_index].fire_loc[fire_index] *= scale;
        }
      }

      else if (defname == "debris_fire_type")
        GetDefNumber(debris[debris_index].fire_type, def, m_filename);
    }
  }
}

void ShipDesign::ParseExplosion(TermStruct* val, int index)
{
  ShipExplosion* exp = &explosion[index];

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* def = val->elements()->at(i)->isDef();
    if (def)
    {
      std::string defname = def->name()->value();

      if (defname == "time")
        GetDefNumber(exp->time, def, m_filename);

      else if (defname == "type")
        GetDefNumber(exp->type, def, m_filename);

      else if (defname == "loc")
      {
        GetDefVec(exp->loc, def, m_filename);
        exp->loc *= scale;
      }

      else if (defname == "final")
        GetDefBool(exp->final, def, m_filename);
    }
  }
}

void ShipDesign::ParseDebris(TermStruct* val, int index)
{
  std::string model_name;
  int fire_index = 0;
  ShipDebris* deb = &debris[index];

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* def = val->elements()->at(i)->isDef();
    if (def)
    {
      std::string defname = def->name()->value();

      if (defname == "model")
      {
        GetDefText(model_name, def, m_filename);
        auto model = NEW Model;
        if (!model->Load(model_name, scale))
        {
          DebugTrace("Could not load debris model '{}'\n", model_name);
          delete model;
          return;
        }

        PrepareModel(*model);
        deb->model = model;
      }

      else if (defname == "mass")
        GetDefNumber(deb->mass, def, m_filename);

      else if (defname == "speed")
        GetDefNumber(deb->speed, def, m_filename);

      else if (defname == "drag")
        GetDefNumber(deb->drag, def, m_filename);

      else if (defname == "loc")
      {
        GetDefVec(deb->loc, def, m_filename);
        deb->loc *= scale;
      }

      else if (defname == "count")
        GetDefNumber(deb->count, def, m_filename);

      else if (defname == "life")
        GetDefNumber(deb->life, def, m_filename);

      else if (defname == "fire")
      {
        if (fire_index < 5)
        {
          GetDefVec(deb->fire_loc[fire_index], def, m_filename);
          deb->fire_loc[fire_index] *= scale;
          fire_index++;
        }
      }

      else if (defname == "fire_type")
        GetDefNumber(deb->fire_type, def, m_filename);
    }
  }
}

void ShipDesign::ParseMap(TermStruct* val)
{
  std::string sprite_name;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "sprite")
      {
        GetDefText(sprite_name, pdef, m_filename);

        auto sprite = NEW Bitmap();
        DataLoader* loader = DataLoader::GetLoader();
        loader->LoadBitmap(sprite_name, *sprite, Bitmap::BMP_TRANSLUCENT);

        map_sprites.append(sprite);
      }
    }
  }
}

void ShipDesign::ParseSquadron(TermStruct* _val)
{
  std::string name;
  std::string design;
  int count = 4;
  int avail = 4;

  for (int i = 0; i < _val->elements()->size(); i++)
  {
    TermDef* pdef = _val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "name")
        GetDefText(name, pdef, m_filename);
      else if (defname == "design")
        GetDefText(design, pdef, m_filename);
      else if (defname == "count")
        GetDefNumber(count, pdef, m_filename);
      else if (defname == "avail")
        GetDefNumber(avail, pdef, m_filename);
    }
  }

  auto s = NEW ShipSquadron;
  s->name = name;

  s->design = Get(design);
  s->count = count;
  s->avail = avail;

  squadrons.append(s);
}

Skin* ShipDesign::ParseSkin(TermStruct* _val)
{
  Skin* skin = nullptr;
  std::string name;

  for (int i = 0; i < _val->elements()->size(); i++)
  {
    TermDef* def = _val->elements()->at(i)->isDef();
    if (def)
    {
      std::string defname = def->name()->value();

      if (defname == "name")
      {
        GetDefText(name, def, m_filename);

        skin = NEW Skin(name);
      }
      else if (defname == "material" || defname == "mtl")
      {
        if (!def->term() || !def->term()->isStruct())
          DebugTrace("WARNING: skin struct missing in '{}'\n", m_filename);
        else
        {
          TermStruct* val = def->term()->isStruct();
          ParseSkinMtl(val, skin);
        }
      }
    }
  }

  if (skin && skin->NumCells())
    skins.append(skin);

  else if (skin)
  {
    delete skin;
    skin = nullptr;
  }

  return skin;
}

void ShipDesign::ParseSkinMtl(TermStruct* _val, Skin* _skin)
{
  auto mtl = NEW Material;
  if (mtl == nullptr)
    return;

  for (int i = 0; i < _val->elements()->size(); i++)
  {
    TermDef* def = _val->elements()->at(i)->isDef();
    if (def)
    {
      std::string defname = def->name()->value();

      if (defname == "name")
        GetDefText(mtl->name, def, m_filename);
      else if (defname == "Ka")
        GetDefColor(mtl->Ka, def, m_filename);
      else if (defname == "Kd")
        GetDefColor(mtl->Kd, def, m_filename);
      else if (defname == "Ks")
        GetDefColor(mtl->Ks, def, m_filename);
      else if (defname == "Ke")
        GetDefColor(mtl->Ke, def, m_filename);
      else if (defname == "Ns" || defname == "power")
        GetDefNumber(mtl->power, def, m_filename);
      else if (defname == "bump")
        GetDefNumber(mtl->bump, def, m_filename);
      else if (defname == "luminous")
        GetDefBool(mtl->luminous, def, m_filename);

      else if (defname == "blend")
      {
        if (def->term() && def->term()->isNumber())
          GetDefNumber(mtl->blend, def, m_filename);

        else if (def->term() && def->term()->isText())
        {
          std::string val;
          GetDefText(val, def, m_filename);

          if (val == "alpha" || val == "translucent")
            mtl->blend = Material::MTL_TRANSLUCENT;

          else if (val == "additive")
            mtl->blend = Material::MTL_ADDITIVE;

          else
            mtl->blend = Material::MTL_SOLID;
        }
      }

      else if (defname.starts_with("tex_d"))
      {
        std::string tex_name;
        if (!GetDefText(tex_name, def, m_filename))
          DebugTrace("WARNING: invalid or missing tex_diffuse in '{}'\n", m_filename);

        DataLoader* loader = DataLoader::GetLoader();
        loader->LoadTexture(tex_name, mtl->tex_diffuse);
      }

      else if (defname.starts_with("tex_s"))
      {
        std::string tex_name;
        if (!GetDefText(tex_name, def, m_filename))
          DebugTrace("WARNING: invalid or missing tex_specular in '{}'\n", m_filename);

        DataLoader* loader = DataLoader::GetLoader();
        loader->LoadTexture(tex_name, mtl->tex_specular);
      }

      else if (defname.starts_with("tex_b"))
      {
        std::string tex_name;
        if (!GetDefText(tex_name, def, m_filename))
          DebugTrace("WARNING: invalid or missing tex_bumpmap in '{}'\n", m_filename);

        DataLoader* loader = DataLoader::GetLoader();
        loader->LoadTexture(tex_name, mtl->tex_bumpmap);
      }

      else if (defname.starts_with("tex_e"))
      {
        std::string tex_name;
        if (!GetDefText(tex_name, def, m_filename))
          DebugTrace("WARNING: invalid or missing tex_emissive in '{}'\n", m_filename);

        DataLoader* loader = DataLoader::GetLoader();

        loader->LoadTexture(tex_name, mtl->tex_emissive);
      }
    }
  }

  if (_skin && mtl)
    _skin->AddMaterial(mtl);
}

const Skin* ShipDesign::FindSkin(std::string_view skin_name) const
{
  int n = skins.size();

  for (int i = 0; i < n; i++)
  {
    Skin* s = skins[n - 1 - i];

    if (s->Name() == skin_name)
      return s;
  }

  return nullptr;
}

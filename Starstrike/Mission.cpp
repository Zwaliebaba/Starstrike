#include "pch.h"
#include "Mission.h"
#include "DataLoader.h"
#include "FormatUtil.h"
#include "Galaxy.h"
#include "Game.h"
#include "Instruction.h"
#include "Intel.h"
#include "MissionEvent.h"
#include "ParseUtil.h"
#include "Random.h"
#include "Reader.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Sim.h"
#include "Skin.h"
#include "StarSystem.h"

Mission::Mission(int identity, std::string_view fname, std::string_view _path)
  : id(identity),
  type(0),
  team(1),
  start(33 * 3600),
  stardate(0),
  ok(false),
  active(false),
  complete(false),
  degrees(false),
  star_system(nullptr),
  target(nullptr),
  ward(nullptr),
  current(nullptr)
{
  objective = Game::GetText("Mission.unspecified");
  sitrep = Game::GetText("Mission.unknown");

  filename = fname;

  if (!_path.empty())
    path = _path;
  else
    path = "Missions\\";
}

Mission::~Mission()
{
  DebugTrace("Mission::~Mission() id = {} name = '{}'\n", id, m_name.data());
  elements.destroy();
  events.destroy();
}

std::string Mission::Subtitles() const { return subtitles; }

void Mission::AddElement(MissionElement* elem)
{
  if (elem)
    elements.append(elem);
}

MissionElement* Mission::FindElement(std::string_view name)
{
  ListIter<MissionElement> iter = elements;
  while (++iter)
  {
    MissionElement* elem = iter.value();

    if (elem->Name() == name)
      return elem;
  }

  return nullptr;
}

void Mission::IncreaseElemPriority(int elem_index)
{
  if (elem_index > 0 && elem_index < elements.size())
  {
    MissionElement* elem1 = elements.at(elem_index - 1);
    MissionElement* elem2 = elements.at(elem_index);

    elements.at(elem_index - 1) = elem2;
    elements.at(elem_index) = elem1;
  }
}

void Mission::DecreaseElemPriority(int elem_index)
{
  if (elem_index >= 0 && elem_index < elements.size() - 1)
  {
    MissionElement* elem1 = elements.at(elem_index);
    MissionElement* elem2 = elements.at(elem_index + 1);

    elements.at(elem_index) = elem2;
    elements.at(elem_index + 1) = elem1;
  }
}

void Mission::IncreaseEventPriority(int event_index)
{
  if (event_index > 0 && event_index < events.size())
  {
    MissionEvent* event1 = events.at(event_index - 1);
    MissionEvent* event2 = events.at(event_index);

    events.at(event_index - 1) = event2;
    events.at(event_index) = event1;
  }
}

void Mission::DecreaseEventPriority(int event_index)
{
  if (event_index >= 0 && event_index < events.size() - 1)
  {
    MissionEvent* event1 = events.at(event_index);
    MissionEvent* event2 = events.at(event_index + 1);

    events.at(event_index) = event2;
    events.at(event_index + 1) = event1;
  }
}

void Mission::SetStarSystem(StarSystem* s)
{
  if (star_system != s)
  {
    star_system = s;

    if (!system_list.contains(s))
      system_list.append(s);
  }
}

void Mission::ClearSystemList()
{
  star_system = nullptr;
  system_list.clear();
}

void Mission::SetPlayer(MissionElement* player_element)
{
  ListIter<MissionElement> elem = elements;
  while (++elem)
  {
    MissionElement* element = elem.value();
    if (element == player_element)
      element->player = 1;
    else
      element->player = 0;
  }
}

MissionElement* Mission::GetPlayer()
{
  MissionElement* p = nullptr;

  ListIter<MissionElement> elem = elements;
  while (++elem)
  {
    if (elem->player > 0)
      p = elem.value();
  }

  return p;
}

MissionEvent* Mission::FindEvent(int event_type) const
{
  auto pThis = (Mission*)this;
  ListIter<MissionEvent> iter = pThis->events;
  while (++iter)
  {
    MissionEvent* event = iter.value();

    if (event->Event() == event_type)
      return event;
  }

  return nullptr;
}

void Mission::AddEvent(MissionEvent* event)
{
  if (event)
    events.append(event);
}

bool Mission::Load(std::string_view fname, std::string_view pname)
{
  ok = false;

  if (!fname.empty())
    filename = fname;

  if (!pname.empty())
    path = pname;

  if (filename.empty())
  {
    DebugTrace("\nCan't Load Mission, script unspecified.\n");
    return ok;
  }

  // wipe existing mission before attempting to load...
  elements.destroy();
  events.destroy();

  DebugTrace("\nLoad Mission: '{}'\n", filename);

  DataLoader* loader = DataLoader::GetLoader();
  BYTE* block = nullptr;

  loader->SetDataPath(path);
  loader->LoadBuffer(filename, block, true);
  loader->SetDataPath();

  ok = ParseMission((const char*)block);

  loader->ReleaseBuffer(block);
  DebugTrace("Mission Loaded.\n\n");

  if (ok)
    Validate();

  return ok;
}

bool Mission::ParseMission(std::string_view block)
{
  Parser parser(NEW BlockReader(block));
  Term* term = parser.ParseTerm();
  char err[256];

  if (!term)
  {
    sprintf_s(err, "ERROR: could not parse '%s'\n", filename.c_str());
    AddError(err);
    return ok;
  }
  TermText* file_type = term->isText();
  if (!file_type || file_type->value() != "MISSION")
  {
    sprintf_s(err, "ERROR: invalid mission file '%s'\n", filename.c_str());
    AddError(err);
    term->print(10);
    return ok;
  }

  ok = true;

  std::string target_name;
  std::string ward_name;

  do
  {
    delete term;
    term = nullptr;
    term = parser.ParseTerm();

    if (term)
    {
      TermDef* def = term->isDef();
      if (def)
      {
        std::string defname = def->name()->value();

        if (defname == "name")
        {
          GetDefText(m_name, def, filename);
          m_name = Game::GetText(m_name);
        }

        else if (defname == "desc")
        {
          GetDefText(desc, def, filename);
          if (!desc.empty() && desc.length() < 32)
            desc = Game::GetText(desc);
        }

        else if (defname == "type")
        {
          std::string typestr;
          GetDefText(typestr, def, filename);
          type = TypeFromName(typestr);
        }

        else if (defname == "system")
        {
          std::string sysname;
          GetDefText(sysname, def, filename);

          Galaxy* galaxy = Galaxy::GetInstance();

          if (galaxy)
            SetStarSystem(galaxy->GetSystem(sysname));
        }

        else if (defname == "degrees")
          GetDefBool(degrees, def, filename);

        else if (defname == "region")
          GetDefText(m_region, def, filename);

        else if (defname == "objective")
        {
          GetDefText(objective, def, filename);
          if (objective.length() > 0 && objective.length() < 32)
            objective = Game::GetText(objective);
        }

        else if (defname == "sitrep")
        {
          GetDefText(sitrep, def, filename);
          if (sitrep.length() > 0 && sitrep.length() < 32)
            sitrep = Game::GetText(sitrep);
        }

        else if (defname == "subtitles")
        {
          std::string subtitles_path;
          DataLoader* loader = DataLoader::GetLoader();
          BYTE* block = nullptr;

          GetDefText(subtitles_path, def, filename);
          loader->SetDataPath();
          loader->LoadBuffer(subtitles_path, block, true);

          subtitles = std::string("\n") + (const char*)block;

          loader->ReleaseBuffer(block);
        }

        else if (defname == "start")
          GetDefTime(start, def, filename);

        else if (defname == "stardate")
          GetDefNumber(stardate, def, filename);

        else if (defname == "team")
          GetDefNumber(team, def, filename);

        else if (defname == "target")
          GetDefText(target_name, def, filename);

        else if (defname == "ward")
          GetDefText(ward_name, def, filename);

        else if ((defname == "element") || (defname == "ship") || (defname == "station"))
        {
          if (!def->term() || !def->term()->isStruct())
          {
            sprintf_s(err, "ERROR: element struct missing in '%s'\n", filename.c_str());
            AddError(err);
          }
          else
          {
            TermStruct* val = def->term()->isStruct();
            MissionElement* elem = ParseElement(val);
            AddElement(elem);
          }
        }

        else if (defname == "event")
        {
          if (!def->term() || !def->term()->isStruct())
          {
            sprintf_s(err, "ERROR: event struct missing in '%s'\n", filename.c_str());
            AddError(err);
          }
          else
          {
            TermStruct* val = def->term()->isStruct();
            MissionEvent* event = ParseEvent(val);
            AddEvent(event);
          }
        }
      }     // def
    }        // term
  } while (term);

  if (ok)
  {
    if (target_name[0])
      target = FindElement(target_name);

    if (ward_name[0])
      ward = FindElement(ward_name);
  }

  return ok;
}

bool Mission::Save()
{
  Validate();

  if (!filename[0] || !path[0])
  {
    AddError(Game::GetText("Mission.error.no-file"));
    return ok;
  }

  std::string content = Serialize();

  if (content.length() < 8)
  {
    AddError(Game::GetText("Mission.error.no-serial"));
    return ok;
  }

  if (!_stricmp(path.c_str(), "mods\\missions\\"))
  {
    CreateDirectoryA("Mods", nullptr);
    CreateDirectoryA("Mods\\Missions", nullptr);
  }

  else if (!_stricmp(path.c_str(), "multiplayer\\"))
    CreateDirectoryA("Multiplayer", nullptr);

  char fname[256];
  sprintf_s(fname, "%s%s", path.c_str(), filename.c_str());
  FILE* f;
  fopen_s(&f, fname, "w");
  if (f)
  {
    fwrite(content.data(), content.length(), 1, f);
    fclose(f);
  }

  return ok;
}

void Mission::Validate()
{
  char err[256];

  ok = true;

  if (elements.isEmpty())
  {
    sprintf_s(err, Game::GetText("Mission.error.no-elem").data(), filename.c_str());
    AddError(err);
  }
  else
  {
    bool found_player = false;

    for (int i = 0; i < elements.size(); i++)
    {
      MissionElement* elem = elements.at(i);

      if (elem->Name().length() < 1)
      {
        sprintf_s(err, Game::GetText("Mission.error.unnamed-elem").data(), filename.c_str());
        AddError(err);
      }

      if (elem->Player() > 0)
      {
        if (!found_player)
        {
          found_player = true;

          if (elem->Region() != GetRegion())
          {
            sprintf_s(err, Game::GetText("Mission.error.wrong-sector").data(), elem->Name().c_str(), GetRegion().c_str());
            AddError(err);
          }
        }
        else
        {
          sprintf_s(err, Game::GetText("Mission.error.extra-player").data(), elem->Name().c_str(), filename.c_str());
          AddError(err);
        }
      }
    }

    if (!found_player)
    {
      sprintf_s(err, Game::GetText("Mission.error.no-player").c_str(), filename.c_str());
      AddError(err);
    }
  }
}

void Mission::AddError(const std::string& err)
{
  DebugTrace(err.data());
  errmsg += err;

  ok = false;
}

#define MSN_CHECK(x)   if (!_stricmp(n.data(), #x)) result = Mission::x;

int Mission::TypeFromName(std::string_view n)
{
  int result = -1;

  MSN_CHECK(PATROL)
else
MSN_CHECK(SWEEP)
    else
      MSN_CHECK(INTERCEPT)
      else
        MSN_CHECK(AIR_PATROL)
        else
          MSN_CHECK(AIR_SWEEP)
          else
            MSN_CHECK(AIR_INTERCEPT)
            else
              MSN_CHECK(STRIKE)
              else
                MSN_CHECK(ASSAULT)
                else
                  MSN_CHECK(DEFEND)
                  else
                    MSN_CHECK(ESCORT)
                    else
                      MSN_CHECK(ESCORT_FREIGHT)
                      else
                        MSN_CHECK(ESCORT_SHUTTLE)
                        else
                          MSN_CHECK(ESCORT_STRIKE)
                          else
                            MSN_CHECK(INTEL)
                            else
                              MSN_CHECK(SCOUT)
                              else
                                MSN_CHECK(RECON)
                                else
                                  MSN_CHECK(BLOCKADE)
                                  else
                                    MSN_CHECK(FLEET)
                                    else
                                      MSN_CHECK(BOMBARDMENT)
                                      else
                                        MSN_CHECK(FLIGHT_OPS)
                                        else
                                          MSN_CHECK(TRANSPORT)
                                          else
                                            MSN_CHECK(CARGO)
                                            else
                                              MSN_CHECK(TRAINING)
                                              else
                                                MSN_CHECK(OTHER)

                                                if (result < PATROL)
                                                {
                                                  for (int i = PATROL; i <= OTHER && result < PATROL; i++)
                                                  {
                                                    if (!_stricmp(n.data(), RoleName(i).c_str()))
                                                      result = i;
                                                  }
                                                }

                                                return result;
}

static int elem_id = 351;

MissionElement* Mission::ParseElement(TermStruct* val)
{
  std::string design;
  std::string skin_name;
  std::string role_name;
  char err[256];

  auto element = NEW MissionElement();
  element->rgn_name = m_region;
  element->elem_id = elem_id++;

  current = element;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "name")
        GetDefText(element->name, pdef, filename);

      else if (defname == "carrier")
        GetDefText(element->carrier, pdef, filename);

      else if (defname == "commander")
        GetDefText(element->commander, pdef, filename);

      else if (defname == "squadron")
        GetDefText(element->squadron, pdef, filename);

      else if (defname == "path")
        GetDefText(element->path, pdef, filename);

      else if (defname == "design")
      {
        GetDefText(design, pdef, filename);
        element->m_design = ShipDesign::Get(design, element->path);

        if (!element->m_design)
        {
          sprintf_s(err, Game::GetText("Mission.error.unknown-ship").data(), design.c_str(), filename.c_str());
          AddError(err);
        }
      }

      else if (defname == "skin")
      {
        if (!element->m_design)
        {
          sprintf_s(err, Game::GetText("Mission.error.out-of-order").data(), filename.c_str());
          AddError(err);
        }

        else if (pdef->term()->isText())
        {
          GetDefText(skin_name, pdef, filename);
          element->skin = element->m_design->FindSkin(skin_name);
        }

        else if (pdef->term()->isStruct())
        {
          sprintf_s(err, Game::GetText("Mission.error.bad-skin", filename).c_str());
          AddError(err);
        }
      }

      else if (defname == "mission")
      {
        GetDefText(role_name, pdef, filename);
        element->mission_role = TypeFromName(role_name);
      }

      else if (defname == "intel")
      {
        GetDefText(role_name, pdef, filename);
        element->intel = Intel::IntelFromName(role_name);
      }

      else if (defname == "loc")
      {
        Vec3 loc;
        GetDefVec(loc, pdef, filename);
        element->SetLocation(loc);
      }

      else if (defname == "rloc")
      {
        if (pdef->term()->isStruct())
        {
          RLoc* rloc = ParseRLoc(pdef->term()->isStruct());
          element->SetRLoc(*rloc);
          delete rloc;
        }
      }

      else if (defname.starts_with("head"))
      {
        if (pdef->term()->isArray())
        {
          Vec3 head;
          GetDefVec(head, pdef, filename);
          if (degrees)
            head.z *= static_cast<float>(DEGREES);
          element->heading = head.z;
        }
        else if (pdef->term()->isNumber())
        {
          double heading = 0;
          GetDefNumber(heading, pdef, filename);
          if (degrees)
            heading *= DEGREES;
          element->heading = heading;
        }
      }

      else if (defname == "region" || defname == "rgn")
        GetDefText(element->rgn_name, pdef, filename);

      else if (defname == "iff")
        GetDefNumber(element->IFF_code, pdef, filename);

      else if (defname == "count")
        GetDefNumber(element->count, pdef, filename);

      else if (defname == "maint_count")
        GetDefNumber(element->maint_count, pdef, filename);

      else if (defname == "dead_count")
        GetDefNumber(element->dead_count, pdef, filename);

      else if (defname == "player")
        GetDefNumber(element->player, pdef, filename);

      else if (defname == "alert")
        GetDefBool(element->alert, pdef, filename);

      else if (defname == "playable")
        GetDefBool(element->playable, pdef, filename);

      else if (defname == "rogue")
        GetDefBool(element->rogue, pdef, filename);

      else if (defname == "invulnerable")
        GetDefBool(element->invulnerable, pdef, filename);

      else if (defname == "command_ai")
        GetDefNumber(element->command_ai, pdef, filename);

      else if (defname.starts_with("respawn"))
        GetDefNumber(element->respawns, pdef, filename);

      else if (defname.starts_with("hold"))
        GetDefNumber(element->hold_time, pdef, filename);

      else if (defname.starts_with("zone"))
      {
        if (pdef->term() && pdef->term()->isBool())
        {
          bool locked = false;
          GetDefBool(locked, pdef, filename);
          element->zone_lock = locked;
        }
        else
          GetDefNumber(element->zone_lock, pdef, filename);
      }

      else if (defname == "objective")
      {
        if (!pdef->term() || !pdef->term()->isStruct())
        {
          AddError(Game::GetText("Mission.error.no-objective", element->name, filename));
        }
        else
        {
          TermStruct* val = pdef->term()->isStruct();
          Instruction* obj = ParseInstruction(val, element);
          element->objectives.append(obj);
        }
      }

      else if (defname == "instr")
      {
        std::string obj;
        if (GetDefText(obj, pdef, filename))
          element->instructions.emplace_back(obj);
      }

      else if (defname == "ship")
      {
        if (!pdef->term() || !pdef->term()->isStruct())
        {
          AddError(Game::GetText("Mission.error.no-ship", element->name, filename));
        }
        else
        {
          TermStruct* val = pdef->term()->isStruct();
          MissionShip* s = ParseShip(val, element);
          element->ships.append(s);

          if (s->Integrity() < 0 && element->m_design)
            s->SetIntegrity(element->m_design->integrity);
        }
      }

      else if (defname == "order" || defname == "navpt")
      {
        if (!pdef->term() || !pdef->term()->isStruct())
        {
          AddError(Game::GetText("Mission.error.no-navpt", element->name, filename));
        }
        else
        {
          TermStruct* val = pdef->term()->isStruct();
          Instruction* npt = ParseInstruction(val, element);
          element->navlist.append(npt);
        }
      }

      else if (defname == "loadout")
      {
        if (!pdef->term() || !pdef->term()->isStruct())
        {
          AddError(Game::GetText("Mission.error.no-loadout", element->name, filename));
        }
        else
        {
          TermStruct* val = pdef->term()->isStruct();
          ParseLoadout(val, element);
        }
      }
    }
  }

  if (element->name.empty())
  {
    AddError(Game::GetText("Mission.error.unnamed-elem", filename));
  }

  else if (element->m_design == nullptr)
  {
    AddError(Game::GetText("Mission.error.unknown-ship", element->name, filename));
  }

  current = nullptr;

  return element;
}

MissionEvent* Mission::ParseEvent(TermStruct* val)
{
  auto event = NEW MissionEvent;
  std::string event_name;
  std::string trigger_name;
  static int event_id = 1;
  static double event_time = 0;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "event")
      {
        GetDefText(event_name, pdef, filename);
        event->event = MissionEvent::EventForName(event_name);
      }

      else if (defname == "trigger")
      {
        GetDefText(trigger_name, pdef, filename);
        event->trigger = MissionEvent::TriggerForName(trigger_name);
      }

      else if (defname == "id")
        GetDefNumber(event_id, pdef, filename);

      else if (defname == "time")
        GetDefNumber(event_time, pdef, filename);

      else if (defname == "delay")
        GetDefNumber(event->delay, pdef, filename);

      else if (defname == "event_param" || defname == "param" || defname == "color")
      {
        ZeroMemory(event->event_param, sizeof(event->event_param));

        if (pdef->term()->isNumber())
        {
          GetDefNumber(event->event_param[0], pdef, filename);
          event->event_nparams = 1;
        }

        else if (pdef->term()->isArray())
        {
          std::vector<float> plist;
          GetDefArray(plist, pdef, filename);

          for (int i = 0; i < 10 && i < static_cast<int>(plist.size()); i++)
          {
            float f = plist[i];
            event->event_param[i] = static_cast<int>(f);
            event->event_nparams = i + 1;
          }
        }
      }

      else if (defname == "trigger_param")
      {
        ZeroMemory(event->trigger_param, sizeof(event->trigger_param));

        if (pdef->term()->isNumber())
        {
          GetDefNumber(event->trigger_param[0], pdef, filename);
          event->trigger_nparams = 1;
        }

        else if (pdef->term()->isArray())
        {
          std::vector<float> plist;
          GetDefArray(plist, pdef, filename);

          for (int i = 0; i < 10 && i < static_cast<int>(plist.size()); i++)
          {
            float f = plist[i];
            event->trigger_param[i] = static_cast<int>(f);
            event->trigger_nparams = i + 1;
          }
        }
      }

      else if (defname == "event_ship" || defname == "ship")
        GetDefText(event->event_ship, pdef, filename);

      else if (defname == "event_source" || defname == "source" || defname == "font")
        GetDefText(event->event_source, pdef, filename);

      else if (defname == "event_target" || defname == "target" || defname == "image")
        GetDefText(event->event_target, pdef, filename);

      else if (defname == "event_message" || defname == "message")
      {
        std::string raw_msg;
        GetDefText(raw_msg, pdef, filename);
        raw_msg = Game::GetText(raw_msg);
        event->event_message = FormatTextEscape(raw_msg);
      }

      else if (defname == "event_chance" || defname == "chance")
        GetDefNumber(event->event_chance, pdef, filename);

      else if (defname == "event_sound" || defname == "sound")
        GetDefText(event->event_sound, pdef, filename);

      else if (defname == "loc" || defname == "vec" || defname == "fade")
        GetDefVec(event->event_point, pdef, filename);

      else if (defname == "rect")
        GetDefRect(event->event_rect, pdef, filename);

      else if (defname == "trigger_ship")
        GetDefText(event->trigger_ship, pdef, filename);

      else if (defname == "trigger_target")
        GetDefText(event->trigger_target, pdef, filename);
    }
  }

  event->id = event_id++;
  event->time = event_time;
  return event;
}

MissionShip* Mission::ParseShip(TermStruct* val, MissionElement* element)
{
  auto msn_ship = NEW MissionShip;

  std::string name;
  std::string skin_name;
  std::string regnum;
  std::string region;
  char err[256];
  Vec3 loc(-1.0e9f, -1.0e9f, -1.0e9f);
  Vec3 vel(-1.0e9f, -1.0e9f, -1.0e9f);
  int respawns = -1;
  double heading = -1e9;
  double integrity = -1;
  int ammo[16];
  int fuel[4];
  int i;

  for (i = 0; i < 16; i++)
    ammo[i] = -10;

  for (i = 0; i < 4; i++)
    fuel[i] = -10;

  for (i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "name")
        GetDefText(name, pdef, filename);

      else if (defname == "skin")
      {
        if (!element || !element->m_design)
        {
          sprintf_s(err, Game::GetText("Mission.error.out-of-order").data(), filename.c_str());
          AddError(err);
        }

        else if (pdef->term()->isText())
        {
          GetDefText(skin_name, pdef, filename);
          msn_ship->skin = element->m_design->FindSkin(skin_name);
        }

        else if (pdef->term()->isStruct())
        {
          sprintf_s(err, Game::GetText("Mission.error.bad-skin").data(), filename.c_str());
          AddError(err);
        }
      }

      else if (defname == "regnum")
        GetDefText(regnum, pdef, filename);

      else if (defname == "region")
        GetDefText(region, pdef, filename);

      else if (defname == "loc")
        GetDefVec(loc, pdef, filename);

      else if (defname == "velocity")
        GetDefVec(vel, pdef, filename);

      else if (defname == "respawns")
        GetDefNumber(respawns, pdef, filename);

      else if (defname == "heading")
      {
        if (pdef->term()->isArray())
        {
          Vec3 h;
          GetDefVec(h, pdef, filename);
          if (degrees)
            h.z *= static_cast<float>(DEGREES);
          heading = h.z;
        }
        else if (pdef->term()->isNumber())
        {
          double h = 0;
          GetDefNumber(h, pdef, filename);
          if (degrees)
            h *= DEGREES;
          heading = h;
        }
      }

      else if (defname == "integrity")
        GetDefNumber(integrity, pdef, filename);

      else if (defname == "ammo")
        GetDefArray(ammo, 16, pdef, filename);

      else if (defname == "fuel")
        GetDefArray(fuel, 4, pdef, filename);
    }
  }

  msn_ship->SetName(name);
  msn_ship->SetRegNum(regnum);
  msn_ship->SetRegion(region);
  msn_ship->SetIntegrity(integrity);

  if (loc.x > -1e9)
    msn_ship->SetLocation(loc);

  if (vel.x > -1e9)
    msn_ship->SetVelocity(vel);

  if (respawns > -1)
    msn_ship->SetRespawns(respawns);

  if (heading > -1e9)
    msn_ship->SetHeading(heading);

  if (ammo[0] > -10)
    msn_ship->SetAmmo(ammo);

  if (fuel[0] > -10)
    msn_ship->SetFuel(fuel);

  return msn_ship;
}

Instruction* Mission::ParseInstruction(TermStruct* val, MissionElement* element)
{
  int order = Instruction::VECTOR;
  int status = Instruction::PENDING;
  int formation = 0;
  int speed = 0;
  int priority = 1;
  int farcast = 0;
  int hold = 0;
  int emcon = 0;
  Vec3 loc(0, 0, 0);
  RLoc* rloc = nullptr;
  std::string order_name;
  std::string status_name;
  std::string order_rgn_name;
  std::string tgt_name;
  std::string tgt_desc;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "cmd")
      {
        GetDefText(order_name, pdef, filename);

        for (int cmd = 0; cmd < Instruction::NUM_ACTIONS; cmd++)
        {
          if (!_stricmp(order_name.c_str(), Instruction::ActionName(cmd).c_str()))
            order = cmd;
        }
      }

      else if (defname == "status")
      {
        GetDefText(status_name, pdef, filename);

        for (int n = 0; n < Instruction::NUM_STATUS; n++)
        {
          if (!_stricmp(status_name.c_str(), Instruction::StatusName(n).c_str()))
            status = n;
        }
      }

      else if (defname == "loc")
        GetDefVec(loc, pdef, filename);

      else if (defname == "rloc")
      {
        if (pdef->term()->isStruct())
          rloc = ParseRLoc(pdef->term()->isStruct());
      }

      else if (defname == "rgn")
        GetDefText(order_rgn_name, pdef, filename);
      else if (defname == "speed")
        GetDefNumber(speed, pdef, filename);
      else if (defname == "formation")
        GetDefNumber(formation, pdef, filename);
      else if (defname == "emcon")
        GetDefNumber(emcon, pdef, filename);
      else if (defname == "priority")
        GetDefNumber(priority, pdef, filename);
      else if (defname == "farcast")
      {
        if (pdef->term()->isBool())
        {
          bool f = false;
          GetDefBool(f, pdef, filename);
          farcast = f;
        }
        else
          GetDefNumber(farcast, pdef, filename);
      }
      else if (defname == "tgt")
        GetDefText(tgt_name, pdef, filename);
      else if (defname == "tgt_desc")
        GetDefText(tgt_desc, pdef, filename);
      else if (defname.starts_with("hold"))
        GetDefNumber(hold, pdef, filename);
    }
  }

  std::string rgn;

  if (order_rgn_name.length() > 0)
    rgn = order_rgn_name;

  else if (element->navlist.size() > 0)
    rgn = element->navlist[element->navlist.size() - 1]->RegionName();

  else
    rgn = m_region;

  if (tgt_desc.length() && tgt_name.length())
    tgt_desc = tgt_desc + " " + tgt_name;

  auto instr = NEW Instruction(rgn, loc, order);

  instr->SetStatus(status);
  instr->SetEMCON(emcon);
  instr->SetFormation(formation);
  instr->SetSpeed(speed);
  instr->SetTarget(tgt_name);
  instr->SetTargetDesc(tgt_desc);
  instr->SetPriority(priority - 1);
  instr->SetFarcast(farcast);
  instr->SetHoldTime(hold);

  if (rloc)
  {
    instr->GetRLoc() = *rloc;
    delete rloc;
  }

  return instr;
}

void Mission::ParseLoadout(TermStruct* val, MissionElement* element)
{
  int ship = -1;
  int stations[16];
  std::string name;

  ZeroMemory(stations, sizeof(stations));

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "ship")
        GetDefNumber(ship, pdef, filename);
      else if (defname == "name")
        GetDefText(name, pdef, filename);
      else if (defname == "stations")
        GetDefArray(stations, 16, pdef, filename);
    }
  }

  auto load = NEW MissionLoad(ship);

  if (name.length())
    load->SetName(name);

  for (int i = 0; i < 16; i++)
    load->SetStation(i, stations[i]);

  element->loadouts.append(load);
}

RLoc* Mission::ParseRLoc(TermStruct* val)
{
  Vec3 base_loc;
  auto rloc = NEW RLoc;
  RLoc* ref = nullptr;

  double dex = 0;
  double dex_var = 5e3;
  double az = 0;
  double az_var = PI;
  double el = 0;
  double el_var = 0.1;

  for (int i = 0; i < val->elements()->size(); i++)
  {
    TermDef* pdef = val->elements()->at(i)->isDef();
    if (pdef)
    {
      std::string defname = pdef->name()->value();

      if (defname == "dex")
      {
        GetDefNumber(dex, pdef, filename);
        rloc->SetDistance(dex);
      }
      else if (defname == "dex_var")
      {
        GetDefNumber(dex_var, pdef, filename);
        rloc->SetDistanceVar(dex_var);
      }
      else if (defname == "az")
      {
        GetDefNumber(az, pdef, filename);
        if (degrees)
          az *= DEGREES;
        rloc->SetAzimuth(az);
      }
      else if (defname == "az_var")
      {
        GetDefNumber(az_var, pdef, filename);
        if (degrees)
          az_var *= DEGREES;
        rloc->SetAzimuthVar(az_var);
      }
      else if (defname == "el")
      {
        GetDefNumber(el, pdef, filename);
        if (degrees)
          el *= DEGREES;
        rloc->SetElevation(el);
      }
      else if (defname == "el_var")
      {
        GetDefNumber(el_var, pdef, filename);
        if (degrees)
          el_var *= DEGREES;
        rloc->SetElevationVar(el_var);
      }
      else if (defname == "loc")
      {
        GetDefVec(base_loc, pdef, filename);
        rloc->SetBaseLocation(base_loc);
      }

      else if (defname == "ref")
      {
        std::string refstr;
        GetDefText(refstr, pdef, filename);

        if (size_t sep = refstr.find_first_of(':'); sep != std::string::npos)
        {
          std::string elem_name = refstr.substr(0, sep);
          std::string nav_name = refstr.substr(sep + 1, refstr.length());
          MissionElement* elem;

          if (elem_name == "this")
            elem = current;
          else
            elem = FindElement(elem_name);

          if (elem && elem->NavList().size() > 0)
          {
            int index = atoi(nav_name.c_str()) - 1;
            if (index < 0)
              index = 0;
            else if (index >= elem->NavList().size())
              index = elem->NavList().size() - 1;

            ref = &elem->NavList()[index]->GetRLoc();
            rloc->SetReferenceLoc(ref);
          }
          else
          {
            DebugTrace("Warning: no ref found for rloc '{}' in elem '{}'\n", refstr.data(), current->Name().data());
            rloc->SetBaseLocation(RandomPoint());
          }
        }
        else
        {
          MissionElement* elem;

          if (refstr == "this")
            elem = current;
          else
            elem = FindElement(refstr);

          if (elem)
          {
            ref = &elem->GetRLoc();
            rloc->SetReferenceLoc(ref);
          }
          else
          {
            DebugTrace("Warning: no ref found for rloc '{}' in elem '{}'\n", refstr.data(), current->Name().data());
            rloc->SetBaseLocation(RandomPoint());
          }
        }
      }
    }
  }

  return rloc;
}

std::string Mission::RoleName(int role)
{
  switch (role)
  {
  case PATROL:
    return "Patrol";
  case SWEEP:
    return "Sweep";
  case INTERCEPT:
    return "Intercept";
  case AIR_PATROL:
    return "Airborne Patrol";
  case AIR_SWEEP:
    return "Airborne Sweep";
  case AIR_INTERCEPT:
    return "Airborne Intercept";
  case STRIKE:
    return "Strike";
  case ASSAULT:
    return "Assault";
  case DEFEND:
    return "Defend";
  case ESCORT:
    return "Escort";
  case ESCORT_FREIGHT:
    return "Freight Escort";
  case ESCORT_SHUTTLE:
    return "Shuttle Escort";
  case ESCORT_STRIKE:
    return "Strike Escort";
  case INTEL:
    return "Intel";
  case SCOUT:
    return "Scout";
  case RECON:
    return "Recon";
  case BLOCKADE:
    return "Blockade";
  case FLEET:
    return "Fleet";
  case BOMBARDMENT:
    return "Attack";
  case FLIGHT_OPS:
    return "Flight Ops";
  case TRANSPORT:
    return "Transport";
  case CARGO:
    return "Cargo";
  case TRAINING:
    return "Training";
  default: case OTHER:
    return "Misc";
  }
}

std::string Mission::Serialize(std::string_view player_elem, int player_index)
{
  std::string s = "MISSION\n\nname:   \"";
  s += SafeString(Name().c_str());

  if (!desc.empty())
  {
    s += "\"\ndesc:   \"";
    s += SafeString(desc.data());
  }

  s += "\"\ntype:   \"";
  s += SafeString(TypeName());

  ListIter<StarSystem> sys_iter = system_list;
  while (++sys_iter)
  {
    StarSystem* sys = sys_iter.value();

    if (sys != star_system)
    {
      s += "\"\nsystem: \"";
      s += sys->Name();
    }
  }

  s += "\"\nsystem: \"";
  if (GetStarSystem())
    s += SafeString(GetStarSystem()->Name().c_str());
  else
    s += "null";

  ListIter<MissionElement> iter = GetElements();

  Sim* sim = Sim::GetSim();
  if (sim && sim->GetElements().size() > 0)
    iter = sim->GetMissionElements();

  s += "\"\n";

  bool region_set = false;
  if (!player_elem.empty())
  {
    // set the mission region to that of the active player
    while (++iter)
    {
      MissionElement* e = iter.value();
      if (e->Name() == player_elem)
      {
        char buf[32];
        sprintf_s(buf, "team: %d\n", e->GetIFF());
        s += buf;

        s += "region: \"";
        s += SafeString(e->Region().c_str());
        s += "\"\n\n";

        region_set = true;
        break;
      }
    }

    iter.reset();
  }

  if (!region_set)
  {
    s += "region: \"";
    s += SafeString(GetRegion().c_str());
    s += "\"\n\n";
  }

  if (!Objective().empty())
  {
    s += "objective: \"";
    s += SafeString(Objective().c_str());
    s += "\"\n\n";
  }

  if (!Situation().empty())
  {
    s += "sitrep: \"";
    s += SafeString(Situation().c_str());
    s += "\"\n\n";
  }

  char buffer[256];
  FormatTime(buffer, Start());

  s += "start: \"";
  s += buffer;
  s += "\"\n\n";
  s += "degrees: true\n\n";

  while (++iter)
  {
    MissionElement* elem = iter.value();

    s += "element: {\n";
    s += "   name:      \"";
    s += SafeString(elem->Name().c_str());
    s += "\"\n";

    if (elem->Path().length())
    {
      s += "   path:      \"";
      s += SafeString(elem->Path());
      s += "\"\n";
    }

    if (elem->GetDesign())
    {
      s += "   design:    \"";
      s += SafeString(elem->GetDesign()->m_name.c_str());
      s += "\"\n";
    }

    if (elem->GetSkin())
    {
      s += "   skin:      \"";
      s += SafeString(elem->GetSkin()->Name());
      s += "\"\n";
    }

    if (elem->Squadron().length())
    {
      s += "   squadron:  \"";
      s += SafeString(elem->Squadron());
      s += "\"\n";
    }

    if (elem->Carrier().length())
    {
      s += "   carrier:   \"";
      s += SafeString(elem->Carrier());
      s += "\"\n";
    }

    if (elem->Commander().length())
    {
      s += "   commander: \"";
      s += SafeString(elem->Commander());
      s += "\"\n";
    }

    s += "   mission:   \"";
    s += elem->RoleName();
    s += "\"\n\n";

    if (elem->IntelLevel())
    {
      s += "   intel:     \"";
      s += Intel::NameFromIntel(elem->IntelLevel());
      s += "\"\n";
    }

    sprintf_s(buffer, "   count:     %d\n", elem->Count());
    s += buffer;

    if (elem->MaintCount())
    {
      sprintf_s(buffer, "   maint_count: %d\n", elem->MaintCount());
      s += buffer;
    }

    if (elem->DeadCount())
    {
      sprintf_s(buffer, "   dead_count:  %d\n", elem->DeadCount());
      s += buffer;
    }

    if (elem->RespawnCount())
    {
      sprintf_s(buffer, "   respawn_count:  %d\n", elem->RespawnCount());
      s += buffer;
    }

    if (elem->HoldTime())
    {
      sprintf_s(buffer, "   hold_time:  %d\n", elem->HoldTime());
      s += buffer;
    }

    if (elem->ZoneLock())
    {
      sprintf_s(buffer, "   zone_lock:  %d\n", elem->ZoneLock());
      s += buffer;
    }

    if (elem->IsAlert())
      s += "   alert:     true\n";

    if (!elem->IsSquadron())
    {
      sprintf_s(buffer, "   command_ai:%d\n", elem->CommandAI());
      s += buffer;
    }

    sprintf_s(buffer, "   iff:       %d\n", elem->GetIFF());
    s += buffer;

    if (!player_elem.empty())
    {
      if (elem->Name() == player_elem)
      {
        player_index = std::max(player_index, 1);

        sprintf_s(buffer, "   player:    %d\n", player_index);
        s += buffer;
      }
    }

    else
    {
      if (elem->Player())
      {
        sprintf_s(buffer, "   player:    %d\n", elem->Player());
        s += buffer;
      }
    }

    if (!elem->IsSquadron())
    {
      if (elem->IsPlayable())
        s += "   playable:  true\n";
      else
        s += "   playable:  false\n";
    }

    s += "   region:    \"";
    s += elem->Region();
    s += "\"\n";

    sprintf_s(buffer, "   loc:       (%.0f, %.0f, %.0f)\n", elem->Location().x, elem->Location().y, elem->Location().z);
    s += buffer;

    if (elem->Heading() != 0)
    {
      sprintf_s(buffer, "   head:      %d\n", static_cast<int>(elem->Heading() / DEGREES));
      s += buffer;
    }

    if (elem->Loadouts().size())
    {
      s += "\n";

      ListIter<MissionLoad> load_iter = elem->Loadouts();
      while (++load_iter)
      {
        MissionLoad* load = load_iter.value();

        sprintf_s(buffer, "   loadout:   { ship: %d, ", load->GetShip());
        s += buffer;

        if (load->GetName().length())
        {
          s += "name: \"";
          s += SafeString(load->GetName());
          s += "\" }\n";
        }
        else
        {
          s += "stations: (";

          for (int i = 0; i < 16; i++)
          {
            sprintf_s(buffer, "%d", load->GetStation(i));
            s += buffer;

            if (i < 15)
              s += ", ";
          }

          s += ") }\n";
        }
      }
    }

    if (elem->Objectives().size())
    {
      s += "\n";

      ListIter<Instruction> obj_iter = elem->Objectives();
      while (++obj_iter)
      {
        Instruction* inst = obj_iter.value();

        s += "   objective: { cmd: ";
        s += Instruction::ActionName(inst->Action());
        s += ", tgt: \"";
        s += SafeString(inst->TargetName());
        s += "\" }\n";
      }
    }

    if (elem->NavList().size())
    {
      s += "\n";

      ListIter<Instruction> nav_iter = elem->NavList();
      while (++nav_iter)
      {
        Instruction* inst = nav_iter.value();

        s += "   navpt:     { cmd: ";
        s += Instruction::ActionName(inst->Action());
        s += ", status: ";
        s += Instruction::StatusName(inst->Status());

        if (!inst->TargetName().empty())
        {
          s += ", tgt: \"";
          s += SafeString(inst->TargetName());
          s += "\"";
        }

        sprintf_s(buffer, ", loc: (%.0f, %.0f, %.0f), speed: %d", inst->Location().x, inst->Location().y,
          inst->Location().z, inst->Speed());
        s += buffer;

        if (!inst->RegionName().empty())
        {
          s += ", rgn: \"";
          s += inst->RegionName();
          s += "\"";
        }

        if (inst->HoldTime())
        {
          sprintf_s(buffer, ", hold: %d", static_cast<int>(inst->HoldTime()));
          s += buffer;
        }

        if (inst->Farcast())
          s += ", farcast: true";

        if (inst->Formation() > Instruction::DIAMOND)
        {
          sprintf_s(buffer, ", formation: %d", inst->Formation());
          s += buffer;
        }

        if (inst->Priority() > Instruction::PRIMARY)
        {
          sprintf_s(buffer, ", priority: %d", inst->Priority());
          s += buffer;
        }

        s += " }\n";
      }
    }

    if (elem->Instructions().size())
    {
      s += "\n";

      for (auto& ins : elem->Instructions())
      {
        s += "   instr:     \"";
        s += SafeString(ins);
        s += "\"\n";
      }
    }

    if (elem->Ships().size())
    {
      ListIter<MissionShip> s_iter = elem->Ships();
      while (++s_iter)
      {
        MissionShip* ship = s_iter.value();

        s += "\n   ship: {\n";

        if (ship->Name().length())
        {
          s += "      name:      \"";
          s += SafeString(ship->Name());
          s += "\"\n";
        }

        if (ship->RegNum().length())
        {
          s += "      regnum:    \"";
          s += SafeString(ship->RegNum());
          s += "\"\n";
        }

        if (ship->Region().length())
        {
          s += "      region:    \"";
          s += SafeString(ship->Region());
          s += "\"\n";
        }

        if (fabs(ship->Location().x) < 1e9)
        {
          sprintf_s(buffer, "      loc:       (%.0f, %.0f, %.0f),\n", ship->Location().x, ship->Location().y,
            ship->Location().z);
          s += buffer;
        }

        if (fabs(ship->Velocity().x) < 1e9)
        {
          sprintf_s(buffer, "      velocity:  (%.1f, %.1f, %.1f),\n", ship->Velocity().x, ship->Velocity().y,
            ship->Velocity().z);
          s += buffer;
        }

        if (ship->Respawns() > -1)
        {
          sprintf_s(buffer, "      respawns:  %d,\n", ship->Respawns());
          s += buffer;
        }

        if (ship->Heading() > -1e9)
        {
          sprintf_s(buffer, "      heading:   %d,\n", static_cast<int>(ship->Heading() / DEGREES));
          s += buffer;
        }

        if (ship->Integrity() > -1)
        {
          sprintf_s(buffer, "      integrity: %d,\n", static_cast<int>(ship->Integrity()));
          s += buffer;
        }

        if (ship->Decoys() > -1)
        {
          sprintf_s(buffer, "      decoys:    %d,\n", ship->Decoys());
          s += buffer;
        }

        if (ship->Probes() > -1)
        {
          sprintf_s(buffer, "      probes:    %d,\n", ship->Probes());
          s += buffer;
        }

        if (ship->Ammo()[0] > -10)
        {
          s += "\n      ammo: (";

          for (int i = 0; i < 16; i++)
          {
            sprintf_s(buffer, "%d", ship->Ammo()[i]);
            s += buffer;

            if (i < 15)
              s += ", ";
          }

          s += ")\n";
        }

        if (ship->Fuel()[0] > -10)
        {
          s += "\n      fuel: (";

          for (int i = 0; i < 4; i++)
          {
            sprintf_s(buffer, "%d", ship->Fuel()[i]);
            s += buffer;

            if (i < 3)
              s += ", ";
          }

          s += ")\n";
        }

        s += "   }\n";
      }
    }

    s += "}\n\n";
  }

  ListIter<MissionEvent> iter2 = GetEvents();
  while (++iter2)
  {
    MissionEvent* event = iter2.value();

    s += "event: {\n";

    s += "   id:              ";
    sprintf_s(buffer, "%d", event->EventID());
    s += buffer;
    s += ",\n   time:            ";
    sprintf_s(buffer, "%.1f", event->Time());
    s += buffer;
    s += ",\n   delay:           ";
    sprintf_s(buffer, "%.1f", event->Delay());
    s += buffer;
    s += ",\n   event:           ";
    s += event->EventName();
    s += "\n";

    if (event->EventShip().length())
    {
      s += "   event_ship:      \"";
      s += SafeString(event->EventShip());
      s += "\"\n";
    }

    if (event->EventSource().length())
    {
      s += "   event_source:    \"";
      s += SafeString(event->EventSource());
      s += "\"\n";
    }

    if (event->EventTarget().length())
    {
      s += "   event_target:    \"";
      s += SafeString(event->EventTarget());
      s += "\"\n";
    }

    if (event->EventSound().length())
    {
      s += "   event_sound:     \"";
      s += SafeString(event->EventSound());
      s += "\"\n";
    }

    if (event->EventMessage().length())
    {
      s += "   event_message:   \"";
      s += SafeString(event->EventMessage());
      s += "\"\n";
    }

    if (event->EventParam())
    {
      sprintf_s(buffer, "%d", event->EventParam());
      s += "   event_param:     ";
      s += buffer;
      s += "\n";
    }

    if (event->EventChance())
    {
      sprintf_s(buffer, "%d", event->EventChance());
      s += "   event_chance:    ";
      s += buffer;
      s += "\n";
    }

    s += "   trigger:         \"";
    s += event->TriggerName();
    s += "\"\n";

    if (event->TriggerShip().length())
    {
      s += "   trigger_ship:    \"";
      s += SafeString(event->TriggerShip());
      s += "\"\n";
    }

    if (event->TriggerTarget().length())
    {
      s += "   trigger_target:  \"";
      s += SafeString(event->TriggerTarget());
      s += "\"\n";
    }

    std::string param_str = event->TriggerParamStr();

    if (param_str.length())
    {
      s += "   trigger_param:   ";
      s += param_str;
      s += "\n";
    }

    s += "}\n\n";
  }

  s += "// EOF\n";

  return s;
}

static int elem_idkey = 1;

MissionElement::MissionElement()
  : m_id(elem_idkey++),
  elem_id(0),
  m_design(nullptr),
  skin(nullptr),
  count(1),
  maint_count(0),
  dead_count(0),
  IFF_code(0),
  mission_role(Mission::OTHER),
  intel(Intel::SECRET),
  respawns(0),
  hold_time(0),
  zone_lock(0),
  player(0),
  command_ai(1),
  alert(false),
  playable(false),
  rogue(false),
  invulnerable(false),
  heading(0),
  combat_group(nullptr),
  combat_unit(nullptr) {
}

MissionElement::~MissionElement()
{
  ships.destroy();
  objectives.destroy();
  navlist.destroy();
  loadouts.destroy();
}

std::string MissionElement::Abbreviation() const
{
  if (m_design)
    return m_design->m_abrv;

  return "UNK";
}

std::string MissionElement::GetShipName(size_t index) const
{
  if (index >= ships.size())
  {
    if (count > 1)
    {
      return std::format("{} {}", name.data(), index + 1);
    }
    return name;
  }

  return ships[index]->Name();
}

std::string MissionElement::GetRegistry(size_t index) const
{
  if (index >= ships.size())
    return {};

  return ships.at(index)->RegNum();
}

std::string MissionElement::RoleName() const { return Mission::RoleName(mission_role); }

Color MissionElement::MarkerColor() const { return Ship::IFFColor(IFF_code); }

bool MissionElement::IsStatic() const
{
  int design_type = 0;
  if (GetDesign())
    design_type = GetDesign()->type;

  return design_type >= Ship::STATION;
}

bool MissionElement::IsGroundUnit() const
{
  int design_type = 0;
  if (GetDesign())
    design_type = GetDesign()->type;

  return (design_type & Ship::GROUND_UNITS) ? true : false;
}

bool MissionElement::IsStarship() const
{
  int design_type = 0;
  if (GetDesign())
    design_type = GetDesign()->type;

  return (design_type & Ship::STARSHIPS) ? true : false;
}

bool MissionElement::IsDropship() const
{
  int design_type = 0;
  if (GetDesign())
    design_type = GetDesign()->type;

  return (design_type & Ship::DROPSHIPS) ? true : false;
}

bool MissionElement::IsCarrier() const
{
  const ShipDesign* design = GetDesign();
  if (design && design->flight_decks.size() > 0)
    return true;

  return false;
}

bool MissionElement::IsSquadron() const
{
  if (carrier.length() > 0)
    return true;

  return false;
}

Point MissionElement::Location() const
{
  auto pThis = (MissionElement*)this;
  return pThis->rloc.Location();
}

void MissionElement::SetLocation(const Point& l)
{
  rloc.SetBaseLocation(l);
  rloc.SetReferenceLoc(nullptr);
  rloc.SetDistance(0);
}

void MissionElement::SetRLoc(const RLoc& r) { rloc = r; }

void MissionElement::AddNavPoint(Instruction* pt, Instruction* afterPoint)
{
  if (pt && !navlist.contains(pt))
  {
    if (afterPoint)
    {
      int index = navlist.index(afterPoint);

      if (index > -1)
        navlist.insert(pt, index + 1);
      else
        navlist.append(pt);
    }

    else
      navlist.append(pt);
  }
}

void MissionElement::DelNavPoint(Instruction* pt)
{
  if (pt)
    delete navlist.remove(pt);
}

void MissionElement::ClearFlightPlan() { navlist.destroy(); }

int MissionElement::GetNavIndex(const Instruction* n)
{
  int index = 0;

  if (navlist.size() > 0)
  {
    ListIter<Instruction> navpt = navlist;
    while (++navpt)
    {
      index++;
      if (navpt.value() == n)
        return index;
    }
  }

  return 0;
}

MissionLoad::MissionLoad(int s, std::string_view n)
  : m_ship(s)
{
  for (int i = 0; i < 16; i++)
    m_load[i] = -1; // default: no weapon mounted

  if (!n.empty())
    m_name = n;
}

std::string MissionLoad::GetName() const { return m_name; }

int MissionLoad::GetStation(int index)
{
  if (index >= 0 && index < 16)
    return m_load[index];

  return 0;
}

void MissionLoad::SetStation(int index, int selection)
{
  if (index >= 0 && index < 16)
    m_load[index] = selection;
}

MissionShip::MissionShip()
  : skin(nullptr),
  loc(-1e9, -1e9, -1e9),
  respawns(0),
  heading(0),
  integrity(100),
  decoys(-10),
  probes(-10)
{
  for (int i = 0; i < 16; i++)
    ammo[i] = -10;

  for (int i = 0; i < 4; i++)
    fuel[i] = -10;
}

void MissionShip::SetAmmo(const int* a)
{
  if (a)
  {
    for (int i = 0; i < 16; i++)
      ammo[i] = a[i];
  }
}

void MissionShip::SetFuel(const int* f)
{
  if (f)
  {
    for (int i = 0; i < 4; i++)
      fuel[i] = f[i];
  }
}

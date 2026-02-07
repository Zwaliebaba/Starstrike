#include "pch.h"
#include "CampaignMissionStarship.h"
#include "Callsign.h"
#include "Campaign.h"
#include "CampaignMissionRequest.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "CombatZone.h"
#include "Combatant.h"
#include "Instruction.h"
#include "Intel.h"
#include "Mission.h"
#include "MissionTemplate.h"
#include "Player.h"
#include "Random.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "StarSystem.h"
#include "Starshatter.h"

static int pkg_id = 1000;
extern int dump_missions;

CampaignMissionStarship::CampaignMissionStarship(Campaign* c)
  : campaign(c),
    player_unit(nullptr),
    player_group(nullptr),
    strike_group(nullptr),
    strike_target(nullptr),
    mission(nullptr),
    m_player(nullptr),
    ward(nullptr),
    prime_target(nullptr),
    escort(nullptr),
    ownside(0),
    enemy(-1),
    mission_type(0)
{
  if (!campaign || !campaign->GetPlayerGroup())
  {
    DebugTrace("ERROR - CMS campaign=0x{:08x} player_group=0x{:08x}\n", (size_t)campaign,
               (intptr_t)campaign ? (size_t)campaign->GetPlayerGroup() : 0);
    return;
  }

  player_group = campaign->GetPlayerGroup();
  player_unit = campaign->GetPlayerUnit();
}

CampaignMissionStarship::~CampaignMissionStarship() {}

void CampaignMissionStarship::CreateMission(CampaignMissionRequest* req)
{
  if (!campaign || !req)
    return;

  DebugTrace("\n-----------------------------------------------\n");
  if (req->Script().length())
  {
    DebugTrace("CMS CreateMission() request: {} '{}'\n", Mission::RoleName(req->Type()),
               (req->Script()));
  }

  else
  {
    DebugTrace("CMS CreateMission() request: {} {}\n", Mission::RoleName(req->Type()),
               req->GetObjective() ? req->GetObjective()->Name().data() : "(no target)");
  }

  request = req;

  if (!player_group)
    return;

  if (request->GetPrimaryGroup() != player_group)
  {
    player_group = request->GetPrimaryGroup();
    player_unit = nullptr;
  }

  ownside = player_group->GetIFF();
  mission_info = nullptr;

  for (int i = 0; i < campaign->GetCombatants().size(); i++)
  {
    int iff = campaign->GetCombatants().at(i)->GetIFF();
    if (iff > 0 && iff != ownside)
    {
      enemy = iff;
      break;
    }
  }

  static int id_key = 1;
  GenerateMission(id_key++);
  DefineMissionObjectives();

  MissionInfo* info = DescribeMission();

  if (info)
  {
    campaign->GetMissionList().append(info);

    DebugTrace("CMS Created %03d '{}' {}\n\n", info->m_id, info->m_name, Mission::RoleName(mission->Type()));

    if (dump_missions)
    {
      std::string script = mission->Serialize();
      char fname[32];

      sprintf_s(fname, "msn%03d.def", info->m_id);
      FILE* f;
      fopen_s(&f, fname, "w");
      if (f)
      {
        fprintf(f, "%s\n", script.data());
        fclose(f);
      }
    }
  }

  else
    DebugTrace("CMS failed to create mission.\n");
}

Mission* CampaignMissionStarship::GenerateMission(int id)
{
  bool found = false;

  SelectType();

  if (request && request->Script().length())
  {
    auto mt = NEW MissionTemplate(id, request->Script(), campaign->Path());
    if (mt)
      mt->SetPlayerSquadron(player_group);
    mission = mt;
    found = true;
  }

  else
  {
    mission_info = campaign->FindMissionTemplate(mission_type, player_group);

    found = mission_info != nullptr;

    if (found)
    {
      auto mt = NEW MissionTemplate(id, mission_info->script, campaign->Path());
      if (mt)
        mt->SetPlayerSquadron(player_group);
      mission = mt;
    }
    else
    {
      mission = NEW Mission(id);
      if (mission)
        mission->SetType(mission_type);
    }
  }

  if (!mission || !player_group)
  {
    Exit();
    return nullptr;
  }

  char name[64];
  sprintf_s(name, "Starship Mission %d", id);

  mission->SetName(name);
  mission->SetTeam(player_group->GetIFF());
  mission->SetStart(request->StartTime());

  SelectRegion();
  GenerateStandardElements();

  if (!found)
  {
    GenerateMissionElements();
    mission->SetOK(true);
    mission->Validate();
  }

  else
  {
    CreatePlayer();
    mission->Load();

    if (mission->IsOK())
    {
      m_player = mission->GetPlayer();
      prime_target = mission->GetTarget();
      ward = mission->GetWard();
    }

    // if there was a problem, scrap the mission
    // and start over:
    else
    {
      delete mission;

      mission = NEW Mission(id);

      if (!mission)
      {
        Exit();
        return nullptr;
      }

      mission->SetType(mission_type);
      mission->SetName(name);
      mission->SetTeam(player_group->GetIFF());
      mission->SetStart(request->StartTime());

      SelectRegion();
      GenerateStandardElements();
      GenerateMissionElements();

      mission->SetOK(true);
      mission->Validate();
    }
  }

  return mission;
}

void CampaignMissionStarship::SelectType()
{
  if (request)
    mission_type = request->Type();

  else
    mission_type = Mission::PATROL;

  if (player_unit && player_unit->GetShipClass() == Ship::CARRIER)
    mission_type = Mission::FLIGHT_OPS;
}

void CampaignMissionStarship::SelectRegion()
{
  if (!player_group)
  {
    DebugTrace("WARNING: CMS - no player group in SelectRegion\n");
    return;
  }

  CombatZone* zone = player_group->GetAssignedZone();

  if (!zone)
    zone = player_group->GetCurrentZone();

  if (zone)
  {
    mission->SetStarSystem(campaign->GetSystem(zone->System()));

    if (zone->HasRegion(player_group->GetRegion()))
      mission->SetRegion(player_group->GetRegion());

    else
      mission->SetRegion(zone->GetRegions().at(0));
  }

  else
  {
    DebugTrace("WARNING: CMS - No zone for '{}'\n", player_group->Name().data());

    StarSystem* s = campaign->GetSystemList()[0];

    mission->SetStarSystem(s);
    mission->SetRegion(s->Regions()[0]->Name());
  }
}

void CampaignMissionStarship::GenerateStandardElements()
{
  ListIter<CombatZone> z = campaign->GetZones();
  while (++z)
  {
    ListIter<ZoneForce> iter = z->GetForces();
    while (++iter)
    {
      ZoneForce* force = iter.value();
      ListIter<CombatGroup> group = force->GetGroups();

      while (++group)
      {
        CombatGroup* g = group.value();

        switch (g->Type())
        {
          case CombatGroup::INTERCEPT_SQUADRON:
          case CombatGroup::FIGHTER_SQUADRON:
          case CombatGroup::ATTACK_SQUADRON:
          case CombatGroup::LCA_SQUADRON:
            CreateSquadron(g);
            break;

          case CombatGroup::DESTROYER_SQUADRON:
          case CombatGroup::BATTLE_GROUP:
          case CombatGroup::CARRIER_GROUP:
            CreateElements(g);
            break;

          case CombatGroup::MINEFIELD:
          case CombatGroup::BATTERY:
          case CombatGroup::MISSILE:
          case CombatGroup::STATION:
          case CombatGroup::STARBASE:
          case CombatGroup::SUPPORT:
          case CombatGroup::COURIER:
          case CombatGroup::MEDICAL:
          case CombatGroup::SUPPLY:
          case CombatGroup::REPAIR:
            CreateElements(g);
            break;

          case CombatGroup::CIVILIAN:
          case CombatGroup::WAR_PRODUCTION:
          case CombatGroup::FACTORY:
          case CombatGroup::REFINERY:
          case CombatGroup::RESOURCE:
          case CombatGroup::INFRASTRUCTURE:
          case CombatGroup::TRANSPORT:
          case CombatGroup::NETWORK:
          case CombatGroup::HABITAT:
          case CombatGroup::STORAGE:
          case CombatGroup::NON_COM:
            CreateElements(g);
            break;
        }
      }
    }
  }
}

void CampaignMissionStarship::GenerateMissionElements()
{
  CreatePlayer();
  CreateWards();
  CreateTargets();

  if (ward && m_player)
  {
    auto obj = NEW Instruction(Instruction::ESCORT, ward->Name());

    if (obj)
    {
      switch (mission->Type())
      {
        case Mission::ESCORT_FREIGHT:
          obj->SetTargetDesc(std::string("the star freighter ") + ward->Name());
          break;

        case Mission::ESCORT_SHUTTLE:
          obj->SetTargetDesc(std::string("the shuttle ") + ward->Name());
          break;

        case Mission::ESCORT_STRIKE:
          obj->SetTargetDesc(std::string("the ") + ward->Name() + std::string(" strike package"));
          break;

        default:
          if (ward->GetCombatGroup())
            obj->SetTargetDesc(std::string("the ") + ward->GetCombatGroup()->GetDescription());
          else
            obj->SetTargetDesc(std::string("the ") + ward->Name());
          break;
      }

      m_player->AddObjective(obj);
    }
  }
}

void CampaignMissionStarship::CreatePlayer()
{
  // prepare elements for the player's group
  MissionElement* elem = nullptr;

  if (player_group)
  {
    ListIter<MissionElement> iter = mission->GetElements();
    while (++iter)
    {
      MissionElement* e = iter.value();
      if (e->GetCombatGroup() == player_group)
      {
        player_group_elements.append(e);

        // match the player to the requested unit, if possible:
        if ((!player_unit && !elem) || (player_unit == e->GetCombatUnit()))
          elem = e;
      }
    }
  }

  if (elem)
  {
    elem->SetPlayer(1);
    elem->SetCommandAI(0);
    m_player = elem;
  }
  else if (player_group)
  {
    DebugTrace("CMS GenerateMissionElements() could not find player element '{}'\n", player_group->Name().data());
  }
  else
    DebugTrace("CMS GenerateMissionElements() could not find player element (no player group)\n");
}

void CampaignMissionStarship::CreateElements(CombatGroup* g)
{
  MissionElement* elem = nullptr;
  List<CombatUnit>& units = g->GetUnits();

  CombatUnit* cmdr = nullptr;

  for (int i = 0; i < units.size(); i++)
  {
    elem = CreateSingleElement(g, units[i]);

    if (elem)
    {
      if (!cmdr)
      {
        cmdr = units[i];

        if (player_group && player_group->GetIFF() == g->GetIFF())
        {
          // the grand admiral is all powerful!
          Player* player = Player::GetCurrentPlayer();
          if (player && player->Rank() >= 10)
            elem->SetCommander(player_group->Name().c_str());
        }
      }
      else
      {
        elem->SetCommander(cmdr->Name());

        if (g->Type() == CombatGroup::CARRIER_GROUP && elem->MissionRole() == Mission::ESCORT)
        {
          auto obj = NEW Instruction(Instruction::ESCORT, cmdr->Name());
          if (obj)
          {
            obj->SetTargetDesc(std::string("the ") + g->GetDescription());
            elem->AddObjective(obj);
          }
        }
      }

      mission->AddElement(elem);
    }
  }
}

MissionElement* CampaignMissionStarship::CreateSingleElement(CombatGroup* g, CombatUnit* u)
{
  if (!g || g->IsReserve())
    return nullptr;
  if (!u || u->LiveCount() < 1)
    return nullptr;
  if (!mission->GetStarSystem())
    return nullptr;

  // no ground units in starship missions:
  StarSystem* system = mission->GetStarSystem();
  OrbitalRegion* rgn = system->FindRegion(u->GetRegion());

  if (!rgn || rgn->Type() == Orbital::TERRAIN)
    return nullptr;

  // make sure this unit isn't already in the mission:
  ListIter<MissionElement> e_iter = mission->GetElements();
  while (++e_iter)
  {
    MissionElement* elem = e_iter.value();

    if (elem->GetCombatUnit() == u)
      return nullptr;
  }

  auto elem = NEW MissionElement;
  if (!elem)
  {
    Exit();
    return nullptr;
  }

  if (u->Name().length())
    elem->SetName(u->Name());
  else
    elem->SetName(u->DesignName());

  elem->SetElementID(pkg_id++);

  elem->SetDesign(u->GetDesign());
  elem->SetCount(u->LiveCount());
  elem->SetIFF(u->GetIFF());
  elem->SetIntelLevel(g->IntelLevel());
  elem->SetRegion(u->GetRegion());
  elem->SetHeading(u->GetHeading());

  int unit_index = g->GetUnits().index(u);
  Point base_loc = u->Location();
  bool exact = u->IsStatic(); // exact unit-level placement

  if (base_loc.length() < 1)
  {
    base_loc = g->Location();
    exact = false;
  }

  if (unit_index < 0 || unit_index > 0 && !exact)
  {
    Point loc = RandomDirection();

    if (!u->IsStatic())
    {
      while (fabs(loc.y) > fabs(loc.x))
        loc = RandomDirection();

      loc *= 10e3 + 9e3 * unit_index;
    }
    else
      loc *= 2e3 + 2e3 * unit_index;

    elem->SetLocation(base_loc + loc);
  }
  else
    elem->SetLocation(base_loc);

  if (g->Type() == CombatGroup::CARRIER_GROUP)
  {
    if (u->Type() == Ship::CARRIER)
      elem->SetMissionRole(Mission::FLIGHT_OPS);
    else
      elem->SetMissionRole(Mission::ESCORT);
  }
  else if (u->Type() == Ship::STATION || u->Type() == Ship::FARCASTER)
  {
    elem->SetMissionRole(Mission::OTHER);

    // link farcaster to other terminus:
    if (u->Type() == Ship::FARCASTER)
    {
      std::string name = u->Name();
      int dash = -1;

      for (int i = 0; i < name.length(); i++)
      {
        if (name[i] == '-')
          dash = i;
      }

      std::string src = name.substr(0, dash);
      std::string dst = name.substr(dash + 1, name.length() - (dash + 1));

      auto obj = NEW Instruction(Instruction::VECTOR, dst + "-" + src);
      if (obj)
        elem->AddObjective(obj);
    }
  }
  else if ((u->Type() & Ship::STARSHIPS) != 0)
    elem->SetMissionRole(Mission::FLEET);

  elem->SetCombatGroup(g);
  elem->SetCombatUnit(u);

  return elem;
}

CombatUnit* CampaignMissionStarship::FindCarrier(CombatGroup* g)
{
  CombatGroup* carrier = g->FindCarrier();

  if (carrier && carrier->GetUnits().size())
  {
    MissionElement* carrier_elem = mission->FindElement(carrier->Name().c_str());

    if (carrier_elem)
      return carrier->GetUnits().at(0);
  }

  return nullptr;
}

void CampaignMissionStarship::CreateSquadron(CombatGroup* g)
{
  if (!g || g->IsReserve())
    return;

  CombatUnit* fighter = g->GetUnits().at(0);
  CombatUnit* carrier = FindCarrier(g);

  if (!fighter || !carrier)
    return;

  int live_count = fighter->LiveCount();
  int maint_count = (live_count > 4) ? live_count / 2 : 0;

  auto elem = NEW MissionElement;

  if (!elem)
  {
    Exit();
    return;
  }

  elem->SetName(g->Name());
  elem->SetElementID(pkg_id++);

  elem->SetDesign(fighter->GetDesign());
  elem->SetCount(fighter->Count());
  elem->SetDeadCount(fighter->DeadCount());
  elem->SetMaintCount(maint_count);
  elem->SetIFF(fighter->GetIFF());
  elem->SetIntelLevel(g->IntelLevel());
  elem->SetRegion(fighter->GetRegion());

  elem->SetCarrier(carrier->Name());
  elem->SetCommander(carrier->Name());
  elem->SetLocation(carrier->Location() + RandomPoint());

  elem->SetCombatGroup(g);
  elem->SetCombatUnit(fighter);

  mission->AddElement(elem);
}

void CampaignMissionStarship::CreateWards()
{
  switch (mission->Type())
  {
    case Mission::ESCORT_FREIGHT:
      CreateWardFreight();
      break;
    default:
      break;
  }
}

void CampaignMissionStarship::CreateWardFreight()
{
  if (!mission || !mission->GetStarSystem() || !player_group)
    return;

  CombatGroup* freight = nullptr;

  if (request)
    freight = request->GetObjective();

  if (!freight)
    freight = campaign->FindGroup(ownside, CombatGroup::FREIGHT);

  if (!freight || freight->CalcValue() < 1)
    return;

  CombatUnit* unit = freight->GetNextUnit();
  if (!unit)
    return;

  MissionElement* elem = CreateSingleElement(freight, unit);
  if (!elem)
    return;

  elem->SetMissionRole(Mission::CARGO);
  elem->SetIntelLevel(Intel::KNOWN);
  elem->SetRegion(player_group->GetRegion());

  ward = elem;
  mission->AddElement(elem);

  StarSystem* system = mission->GetStarSystem();
  OrbitalRegion* rgn1 = system->FindRegion(elem->Region());
  Point delta = rgn1->Location() - rgn1->Primary()->Location();
  Point navpt_loc = elem->Location();
  Instruction* n = nullptr;

  delta.Normalize();
  delta *= 200.0e3;

  navpt_loc += delta;

  n = NEW Instruction(elem->Region(), navpt_loc, Instruction::VECTOR);
  if (n)
  {
    n->SetSpeed(500);
    elem->AddNavPoint(n);
  }

  std::string rgn2 = elem->Region();
  List<CombatZone>& zones = campaign->GetZones();
  if (zones[zones.size() - 1]->HasRegion(rgn2))
    rgn2 = zones[0]->GetRegions()[0];
  else
    rgn2 = zones[zones.size() - 1]->GetRegions()[0];

  n = NEW Instruction(rgn2, Point(0, 0, 0), Instruction::VECTOR);
  if (n)
  {
    n->SetSpeed(750);
    elem->AddNavPoint(n);
  }
}

void CampaignMissionStarship::CreateEscorts() {}

void CampaignMissionStarship::CreateTargets()
{
  if (player_group && player_group->Type() == CombatGroup::CARRIER_GROUP)
    CreateTargetsCarrier();

  else
  {
    switch (mission->Type())
    {
      default: case Mission::PATROL:
        CreateTargetsPatrol();
        break;
      case Mission::ASSAULT:
      case Mission::STRIKE:
        CreateTargetsAssault();
        break;
      case Mission::ESCORT_FREIGHT:
        CreateTargetsFreightEscort();
        break;
    }
  }
}

void CampaignMissionStarship::CreateTargetsAssault()
{
  if (!m_player)
    return;

  CombatGroup* assigned = nullptr;

  if (request)
    assigned = request->GetObjective();

  if (assigned)
  {
    CreateElements(assigned);

    ListIter<MissionElement> e_iter = mission->GetElements();
    while (++e_iter)
    {
      MissionElement* elem = e_iter.value();

      if (elem->GetCombatGroup() == assigned)
      {
        if (!prime_target)
        {
          prime_target = elem;

          MissionElement* player_lead = player_group_elements[0];

          if (!player_lead)
            return;

          auto obj = NEW Instruction(Instruction::ASSAULT, prime_target->Name());
          if (obj)
          {
            obj->SetTargetDesc(std::string("preplanned target '") + prime_target->Name() + "'");
            player_lead->AddObjective(obj);
          }

          // create flight plan:
          RLoc rloc;
          RLoc* ref = nullptr;
          Vec3 dummy(0, 0, 0);
          Instruction* instr = nullptr;
          Point loc = player_lead->Location();
          Point tgt = prime_target->Location();
          Point mid;

          mid = loc + (prime_target->Location() - loc) * 0.35;

          rloc.SetReferenceLoc(nullptr);
          rloc.SetBaseLocation(mid);
          rloc.SetDistance(50e3);
          rloc.SetDistanceVar(5e3);
          rloc.SetAzimuth(90 * DEGREES);
          rloc.SetAzimuthVar(45 * DEGREES);

          instr = NEW Instruction(prime_target->Region(), dummy, Instruction::VECTOR);

          if (!instr)
            return;

          instr->SetSpeed(750);
          instr->GetRLoc() = rloc;

          ref = &instr->GetRLoc();

          player_lead->AddNavPoint(instr);

          for (int i = 1; i < player_group_elements.size(); i++)
          {
            MissionElement* pge = player_group_elements[i];
            RLoc rloc2;

            rloc2.SetReferenceLoc(ref);
            rloc2.SetDistance(50e3);
            rloc2.SetDistanceVar(5e3);

            instr = NEW Instruction(prime_target->Region(), dummy, Instruction::VECTOR);

            if (!instr)
              return;

            instr->SetSpeed(750);
            instr->GetRLoc() = rloc2;

            pge->AddNavPoint(instr);
          }

          double extra = 10e3;

          if (prime_target && prime_target->GetDesign())
          {
            switch (prime_target->GetDesign()->type)
            {
              default:
                extra = 20e3;
                break;
              case Ship::FRIGATE:
                extra = 25e3;
                break;
              case Ship::DESTROYER:
                extra = 30e3;
                break;
              case Ship::CRUISER:
                extra = 50e3;
                break;
              case Ship::BATTLESHIP:
                extra = 70e3;
                break;
              case Ship::DREADNAUGHT:
                extra = 80e3;
                break;
              case Ship::SWACS:
                extra = 30e3;
                break;
              case Ship::CARRIER:
                extra = 90e3;
                break;
            }
          }

          rloc.SetReferenceLoc(nullptr);
          rloc.SetBaseLocation(tgt);
          rloc.SetDistance(100e3 + extra);
          rloc.SetDistanceVar(15e3);
          rloc.SetAzimuth(90 * DEGREES);
          rloc.SetAzimuthVar(45 * DEGREES);

          instr = NEW Instruction(prime_target->Region(), dummy, Instruction::ASSAULT);

          if (!instr)
            return;

          instr->SetSpeed(500);
          instr->GetRLoc() = rloc;
          instr->SetTarget(prime_target->Name());

          ref = &instr->GetRLoc();

          player_lead->AddNavPoint(instr);

          for (int i = 1; i < player_group_elements.size(); i++)
          {
            MissionElement* pge = player_group_elements[i];
            RLoc rloc2;

            rloc2.SetReferenceLoc(ref);
            rloc2.SetDistance(50e3);
            rloc2.SetDistanceVar(5e3);

            instr = NEW Instruction(prime_target->Region(), dummy, Instruction::ASSAULT);

            if (!instr)
              return;

            instr->SetSpeed(500);
            instr->GetRLoc() = rloc2;
            instr->SetTarget(prime_target->Name());

            pge->AddNavPoint(instr);
          }
        }
      }
    }
  }
}

void CampaignMissionStarship::CreateTargetsCarrier()
{
  if (!player_group || !m_player)
    return;

  std::string region = player_group->GetRegion();
  Point base_loc = m_player->Location();
  Point patrol_loc = base_loc + RandomDirection() * Random(75e3, 150e3);
  Point loc2 = patrol_loc + RandomDirection() * Random(50e3, 100e3);

  int ntargets = 2 + (RandomChance() ? 1 : 0);
  int ntries = 8;

  while (ntargets > 0 && ntries > 0)
  {
    Point target_loc = RandomChance() ? patrol_loc : loc2;
    int t = CreateRandomTarget(region, target_loc);
    ntargets -= t;
    if (t < 1)
      ntries--;
  }
}

void CampaignMissionStarship::CreateTargetsPatrol()
{
  if (!player_group || !m_player)
    return;

  std::string region = player_group->GetRegion();
  Point base_loc = m_player->Location();
  Point patrol_loc = base_loc + RandomDirection() * Random(170e3, 250e3);

  auto n = NEW Instruction(region, patrol_loc, Instruction::PATROL);
  m_player->AddNavPoint(n);

  for (int i = 1; i < player_group_elements.size(); i++)
  {
    MissionElement* elem = player_group_elements[i];

    n = NEW Instruction(region, patrol_loc + RandomDirection() * Random(20e3, 40e3), Instruction::PATROL);
    if (n)
      elem->AddNavPoint(n);
  }

  Point loc2 = patrol_loc + RandomDirection() * Random(150e3, 200e3);

  n = NEW Instruction(region, loc2, Instruction::PATROL);
  if (n)
    m_player->AddNavPoint(n);

  for (int i = 1; i < player_group_elements.size(); i++)
  {
    MissionElement* elem = player_group_elements[i];

    n = NEW Instruction(region, loc2 + RandomDirection() * Random(20e3, 40e3), Instruction::PATROL);

    if (n)
      elem->AddNavPoint(n);
  }

  int ntargets = 2 + (RandomChance() ? 1 : 0);
  int ntries = 8;

  while (ntargets > 0 && ntries > 0)
  {
    Point target_loc = RandomChance() ? patrol_loc : loc2;
    int t = CreateRandomTarget(region, target_loc);
    ntargets -= t;
    if (t < 1)
      ntries--;
  }

  auto obj = NEW Instruction(*n);
  if (obj)
  {
    obj->SetTargetDesc("inbound enemy units");
    m_player->AddObjective(obj);
  }
}

void CampaignMissionStarship::CreateTargetsFreightEscort()
{
  if (!ward)
  {
    CreateTargetsPatrol();
    return;
  }

  CombatGroup* s = FindSquadron(enemy, CombatGroup::ATTACK_SQUADRON);
  CombatGroup* s2 = FindSquadron(enemy, CombatGroup::FIGHTER_SQUADRON);

  if (!s || !s2)
    return;

  MissionElement* elem = CreateFighterPackage(s, 2, Mission::ASSAULT);
  if (elem)
  {
    elem->SetIntelLevel(Intel::KNOWN);

    elem->SetLocation(ward->Location() + RandomPoint() * 5.0f);

    auto obj = NEW Instruction(Instruction::ASSAULT, ward->Name());
    if (obj)
      elem->AddObjective(obj);
    mission->AddElement(elem);

    MissionElement* e2 = CreateFighterPackage(s2, 2, Mission::ESCORT);
    if (e2)
    {
      e2->SetIntelLevel(Intel::KNOWN);
      e2->SetLocation(elem->Location() + RandomPoint() * 0.25);

      auto obj2 = NEW Instruction(Instruction::ESCORT, elem->Name());
      if (obj2)
        e2->AddObjective(obj2);
      mission->AddElement(e2);
    }
  }

  auto obj3 = NEW Instruction(mission->GetRegion(), Point(0, 0, 0), Instruction::PATROL);
  if (m_player && obj3)
  {
    obj3->SetTargetDesc("enemy patrols");
    m_player->AddObjective(obj3);
  }
}

int CampaignMissionStarship::CreateRandomTarget(std::string_view rgn, Point base_loc)
{
  int ntargets = 0;
  int ttype = RandomIndex();

  if (player_group && player_group->Type() == CombatGroup::CARRIER_GROUP)
  {
    switch (ttype)
    {
      case 0:
      case 1:
      case 2:
      case 3:
        ttype = 0;
        break;
      case 4:
      case 5:
        ttype = 1;
        break;
      case 6:
      case 7:
        ttype = 2;
        break;
      case 8:
      case 9:
        ttype = 3;
        break;
      case 10:
      case 11:
        ttype = 4;
        break;
      case 12:
      case 13:
      case 14:
      case 15:
        ttype = 5;
        break;
    }
  }
  else
  {
    switch (ttype)
    {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
        ttype = 0;
        break;
      case 6:
      case 7:
      case 8:
        ttype = 1;
        break;
      case 9:
      case 10:
        ttype = 4;
        break;
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
        ttype = 5;
        break;
    }
  }

  switch (ttype)
  {
    case 0:
    {
      CombatGroup* s = nullptr;

      s = FindSquadron(enemy, CombatGroup::DESTROYER_SQUADRON);

      if (s)
      {
        for (int i = 0; i < 2; i++)
        {
          CombatUnit* u = s->GetRandomUnit();
          MissionElement* elem = CreateSingleElement(s, u);
          if (elem)
          {
            elem->SetIntelLevel(Intel::KNOWN);
            elem->SetRegion(rgn);
            elem->SetLocation(base_loc + RandomPoint() * 1.5);
            elem->SetMissionRole(Mission::FLEET);
            mission->AddElement(elem);
            ntargets++;
          }
        }
      }
    }
    break;

    case 1:
    {
      CombatGroup* s = FindSquadron(enemy, CombatGroup::LCA_SQUADRON);

      if (s)
      {
        MissionElement* elem = CreateFighterPackage(s, 2, Mission::CARGO);
        if (elem)
        {
          elem->SetIntelLevel(Intel::KNOWN);
          elem->SetRegion(rgn);
          elem->SetLocation(base_loc + RandomPoint() * 2.0f);
          mission->AddElement(elem);
          ntargets++;

          CombatGroup* s2 = FindSquadron(enemy, CombatGroup::FIGHTER_SQUADRON);

          if (s2)
          {
            MissionElement* e2 = CreateFighterPackage(s2, 2, Mission::ESCORT);
            if (e2)
            {
              e2->SetIntelLevel(Intel::KNOWN);
              e2->SetRegion(rgn);
              e2->SetLocation(elem->Location() + RandomPoint() * 0.5);

              auto obj = NEW Instruction(Instruction::ESCORT, elem->Name());
              if (obj)
                e2->AddObjective(obj);

              mission->AddElement(e2);
            }
          }
        }
      }
    }
    break;

    case 2:
    {
      CombatGroup* s = FindSquadron(enemy, CombatGroup::INTERCEPT_SQUADRON);

      if (s)
      {
        MissionElement* elem = CreateFighterPackage(s, 4, Mission::PATROL);
        if (elem)
        {
          elem->SetIntelLevel(Intel::SECRET);
          elem->SetRegion(rgn);
          elem->SetLocation(base_loc);
          mission->AddElement(elem);
          ntargets++;
        }
      }
    }
    break;

    case 3:
    {
      CombatGroup* s = FindSquadron(enemy, CombatGroup::FIGHTER_SQUADRON);

      if (s)
      {
        MissionElement* elem = CreateFighterPackage(s, 3, Mission::ASSAULT);
        if (elem)
        {
          elem->SetIntelLevel(Intel::KNOWN);
          elem->Loadouts().destroy();
          elem->Loadouts().append(NEW MissionLoad(-1, "Ship Strike"));
          elem->SetRegion(rgn);
          elem->SetLocation(base_loc + RandomPoint());
          mission->AddElement(elem);

          if (m_player)
          {
            auto n = NEW Instruction(m_player->Region(), m_player->Location() + RandomPoint(), Instruction::ASSAULT);
            n->SetTarget(m_player->Name());
            elem->AddNavPoint(n);
          }

          ntargets++;
        }
      }
    }
    break;

    case 4:
    {
      CombatGroup* s = FindSquadron(enemy, CombatGroup::ATTACK_SQUADRON);

      if (s)
      {
        MissionElement* elem = CreateFighterPackage(s, 2, Mission::ASSAULT);
        if (elem)
        {
          elem->SetIntelLevel(Intel::KNOWN);
          elem->Loadouts().destroy();
          elem->Loadouts().append(NEW MissionLoad(-1, "Hvy Ship Strike"));
          elem->SetRegion(rgn);
          elem->SetLocation(base_loc + RandomPoint() * 1.3);
          mission->AddElement(elem);

          if (m_player)
          {
            auto n = NEW Instruction(m_player->Region(), m_player->Location() + RandomPoint(), Instruction::ASSAULT);
            n->SetTarget(m_player->Name());
            elem->AddNavPoint(n);
          }

          ntargets++;
        }
      }
    }
    break;

    default:
    {
      CombatGroup* s = nullptr;

      s = FindSquadron(enemy, CombatGroup::FREIGHT);

      if (s)
      {
        CombatUnit* u = s->GetRandomUnit();
        MissionElement* elem = CreateSingleElement(s, u);
        if (elem)
        {
          elem->SetIntelLevel(Intel::KNOWN);
          elem->SetRegion(rgn);
          elem->SetLocation(base_loc + RandomPoint() * 2.0f);
          elem->SetMissionRole(Mission::CARGO);
          mission->AddElement(elem);
          ntargets++;

          CombatGroup* s2 = FindSquadron(enemy, CombatGroup::INTERCEPT_SQUADRON);

          if (s2)
          {
            MissionElement* e2 = CreateFighterPackage(s2, 2, Mission::ESCORT);
            if (e2)
            {
              e2->SetIntelLevel(Intel::KNOWN);
              e2->SetRegion(rgn);
              e2->SetLocation(elem->Location() + RandomPoint() * 0.5);

              auto obj = NEW Instruction(Instruction::ESCORT, elem->Name());
              if (obj)
                e2->AddObjective(obj);
              mission->AddElement(e2);
              ntargets++;
            }
          }
        }
      }
    }
    break;
  }

  return ntargets;
}

MissionElement* CampaignMissionStarship::CreateFighterPackage(CombatGroup* squadron, int count, int role)
{
  if (!squadron || squadron->IsReserve())
    return nullptr;

  CombatUnit* fighter = squadron->GetUnits().at(0);
  CombatUnit* carrier = FindCarrier(squadron);

  if (!fighter)
    return nullptr;

  int avail = fighter->LiveCount();
  int actual = count;

  if (avail < actual)
    actual = avail;

  if (avail < 1)
  {
    DebugTrace("CMS - Insufficient fighters in squadron '{}' - %d required, %d available\n", squadron->Name().data(),
               count, avail);
    return nullptr;
  }

  auto elem = NEW MissionElement;

  if (!elem)
  {
    Exit();
    return nullptr;
  }

  elem->SetName(Callsign::GetCallsign(fighter->GetIFF()));
  elem->SetElementID(pkg_id++);

  if (carrier)
  {
    elem->SetCommander(carrier->Name());
    elem->SetHeading(carrier->GetHeading());
  }
  else
    elem->SetHeading(fighter->GetHeading());

  elem->SetDesign(fighter->GetDesign());
  elem->SetCount(actual);
  elem->SetIFF(fighter->GetIFF());
  elem->SetIntelLevel(squadron->IntelLevel());
  elem->SetRegion(fighter->GetRegion());
  elem->SetSquadron(fighter->Name());
  elem->SetMissionRole(role);
  elem->Loadouts().append(NEW MissionLoad(-1, "ACM Medium Range"));

  if (carrier)
    elem->SetLocation(carrier->Location() + RandomPoint() * 0.3);
  else
    elem->SetLocation(fighter->Location() + RandomPoint());

  elem->SetCombatGroup(squadron);
  elem->SetCombatUnit(fighter);

  return elem;
}

CombatGroup* CampaignMissionStarship::FindSquadron(int iff, int type)
{
  if (!player_group)
    return nullptr;

  CombatGroup* result = nullptr;
  CombatZone* zone = player_group->GetAssignedZone();
  if (!zone)
    zone = player_group->GetCurrentZone();

  if (!zone)
  {
    DebugTrace("CMS Warning: no zone for {}\n", player_group->Name().data());
    return result;
  }

  ZoneForce* force = zone->FindForce(iff);

  if (force)
  {
    List<CombatGroup> groups;
    ListIter<CombatGroup> group = force->GetGroups();
    while (++group)
    {
      CombatGroup* g = group.value();

      if (g->Type() == type && g->CountUnits() > 0)
      {
        result = g;
        groups.append(g);
      }
    }

    if (groups.size() > 1)
    {
      int index = static_cast<int>(Random(0, groups.size()));
      if (index >= groups.size())
        index = groups.size() - 1;
      result = groups[index];
    }
  }

  return result;
}

void CampaignMissionStarship::DefineMissionObjectives()
{
  if (!mission || !m_player)
    return;

  if (prime_target)
    mission->SetTarget(prime_target);
  if (ward)
    mission->SetWard(ward);

  std::string objectives;

  if (m_player->Objectives().size() > 0)
  {
    for (int i = 0; i < m_player->Objectives().size(); i++)
    {
      Instruction* obj = m_player->Objectives().at(i);
      objectives += "* ";
      objectives += obj->GetDescription();
      objectives += ".\n";
    }
  }
  else
  {
    objectives += "* Perform standard fleet operations in the ";
    objectives += mission->GetRegion();
    objectives += " sector.\n";
  }

  mission->SetObjective(objectives);
}

MissionInfo* CampaignMissionStarship::DescribeMission()
{
  if (!mission || !m_player)
    return nullptr;

  char name[256];
  char player_info[256];

  if (mission_info && !mission_info->m_name.empty())
    sprintf_s(name, "MSN-%03d %s", mission->Identity(), mission_info->m_name.c_str());
  else if (ward)
    sprintf_s(name, "MSN-%03d %s %s", mission->Identity(), Game::GetText(mission->TypeName()).c_str(),
              ward->Name().c_str());
  else if (prime_target)
  {
    sprintf_s(name, "MSN-%03d %s %s %s", mission->Identity(), Game::GetText(mission->TypeName()).c_str(),
              Ship::ClassName(prime_target->GetDesign()->type).data(), prime_target->Name().c_str());
  }
  else
    sprintf_s(name, "MSN-%03d %s", mission->Identity(), Game::GetText(mission->TypeName()).c_str());

  if (m_player)
    strcpy_s(player_info, m_player->GetCombatGroup()->GetDescription());

  auto info = NEW MissionInfo;

  if (info)
  {
    info->m_id = mission->Identity();
    info->mission = mission;
    info->m_name = name;
    info->type = mission->Type();
    info->player_info = player_info;
    info->description = mission->Objective();
    info->start = mission->Start();

    if (mission->GetStarSystem())
      info->system = mission->GetStarSystem()->Name();
    info->region = mission->GetRegion();
  }

  mission->SetName(name);

  return info;
}

void CampaignMissionStarship::Exit()
{
  Starshatter* stars = Starshatter::GetInstance();
  if (stars)
    stars->SetGameMode(Starshatter::MENU_MODE);
}

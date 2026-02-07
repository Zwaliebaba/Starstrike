#include "pch.h"
#include "CombatZone.h"
#include "CombatGroup.h"
#include "DataLoader.h"
#include "ParseUtil.h"
#include "Reader.h"

CombatZone::CombatZone() {}

CombatZone::~CombatZone()
{
  forces.destroy();
}

void CombatZone::Clear() { forces.destroy(); }

void CombatZone::AddGroup(CombatGroup* group)
{
  if (group)
  {
    int iff = group->GetIFF();
    ZoneForce* f = FindForce(iff);
    f->AddGroup(group);
    group->SetCurrentZone(this);
  }
}

void CombatZone::RemoveGroup(CombatGroup* group)
{
  if (group)
  {
    int iff = group->GetIFF();
    ZoneForce* f = FindForce(iff);
    f->RemoveGroup(group);
    group->SetCurrentZone(nullptr);
  }
}

bool CombatZone::HasGroup(CombatGroup* group)
{
  if (group)
  {
    int iff = group->GetIFF();
    ZoneForce* f = FindForce(iff);
    return f->HasGroup(group);
  }

  return false;
}

void CombatZone::AddRegion(std::string_view rgn)
{
  if (!rgn.empty())
  {
    regions.emplace_back(rgn);

    if (name.empty())
      name = rgn;
  }
}

bool CombatZone::HasRegion(std::string_view rgn)
{
  if (!rgn.empty() && regions.size())
  {
    return std::ranges::find(regions, rgn) != regions.end();
  }

  return false;
}

ZoneForce* CombatZone::FindForce(int iff)
{
  ListIter<ZoneForce> f = forces;
  while (++f)
  {
    if (f->GetIFF() == iff)
      return f.value();
  }

  return MakeForce(iff);
}

ZoneForce* CombatZone::MakeForce(int iff)
{
  auto f = NEW ZoneForce(iff);
  forces.append(f);
  return f;
}

static List<CombatZone> zonelist;

List<CombatZone>& CombatZone::Load(const char* filename)
{
  zonelist.clear();

  DataLoader* loader = DataLoader::GetLoader();
  BYTE* block = nullptr;

  loader->LoadBuffer(filename, block, true);
  Parser parser(NEW BlockReader((const char*)block));

  Term* term = parser.ParseTerm();

  if (!term)
    return zonelist;
  TermText* file_type = term->isText();
  if (!file_type || file_type->value() != "ZONES")
    return zonelist;

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
        if (def->name()->value() == "zone")
        {
          if (!def->term() || !def->term()->isStruct())
            DebugTrace("WARNING: zone struct missing in '%s%s'\n", loader->GetDataPath(), filename);
          else
          {
            TermStruct* val = def->term()->isStruct();

            auto zone = NEW CombatZone();
            std::string rgn;

            for (int i = 0; i < val->elements()->size(); i++)
            {
              if (TermDef* pdef = val->elements()->at(i)->isDef())
              {
                if (pdef->name()->value() == "region")
                {
                  GetDefText(rgn, pdef, filename);
                  zone->AddRegion(rgn);
                }
                else if (pdef->name()->value() == "system")
                {
                  GetDefText(rgn, pdef, filename);
                  zone->system = rgn;
                }
              }
            }

            zonelist.append(zone);
          }
        }
      }
    }
  }
  while (term);

  loader->ReleaseBuffer(block);

  return zonelist;
}

ZoneForce::ZoneForce(int i)
{
  iff = i;

  for (int n = 0; n < 8; n++)
    need[n] = 0;
}

void ZoneForce::AddGroup(CombatGroup* group)
{
  if (group)
    groups.append(group);
}

void ZoneForce::RemoveGroup(CombatGroup* group)
{
  if (group)
    groups.remove(group);
}

bool ZoneForce::HasGroup(CombatGroup* group)
{
  if (group)
    return groups.contains(group);

  return false;
}

int ZoneForce::GetNeed(int group_type) const
{
  switch (group_type)
  {
    case CombatGroup::CARRIER_GROUP:
      return need[0];
    case CombatGroup::BATTLE_GROUP:
      return need[1];
    case CombatGroup::DESTROYER_SQUADRON:
      return need[2];
    case CombatGroup::ATTACK_SQUADRON:
      return need[3];
    case CombatGroup::FIGHTER_SQUADRON:
      return need[4];
    case CombatGroup::INTERCEPT_SQUADRON:
      return need[5];
  }

  return 0;
}

void ZoneForce::SetNeed(int group_type, int needed)
{
  switch (group_type)
  {
    case CombatGroup::CARRIER_GROUP:
      need[0] = needed;
      break;
    case CombatGroup::BATTLE_GROUP:
      need[1] = needed;
      break;
    case CombatGroup::DESTROYER_SQUADRON:
      need[2] = needed;
      break;
    case CombatGroup::ATTACK_SQUADRON:
      need[3] = needed;
      break;
    case CombatGroup::FIGHTER_SQUADRON:
      need[4] = needed;
      break;
    case CombatGroup::INTERCEPT_SQUADRON:
      need[5] = needed;
      break;
  }
}

void ZoneForce::AddNeed(int group_type, int needed)
{
  switch (group_type)
  {
    case CombatGroup::CARRIER_GROUP:
      need[0] += needed;
      break;
    case CombatGroup::BATTLE_GROUP:
      need[1] += needed;
      break;
    case CombatGroup::DESTROYER_SQUADRON:
      need[2] += needed;
      break;
    case CombatGroup::ATTACK_SQUADRON:
      need[3] += needed;
      break;
    case CombatGroup::FIGHTER_SQUADRON:
      need[4] += needed;
      break;
    case CombatGroup::INTERCEPT_SQUADRON:
      need[5] += needed;
      break;
  }
}

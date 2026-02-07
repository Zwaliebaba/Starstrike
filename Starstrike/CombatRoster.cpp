#include "pch.h"
#include "CombatRoster.h"
#include "CombatGroup.h"
#include "DataLoader.h"


static CombatRoster* roster = nullptr;

CombatRoster::CombatRoster()
{
  DataLoader* loader = DataLoader::GetLoader();
  loader->SetDataPath("Campaigns\\");

  std::vector<std::string> files;
  loader->ListFiles("*.def", files);

  for (int i = 0; i < files.size(); i++)
  {
    std::string filename = files[i];

    if (!filename.contains("/") && !filename.contains("\\"))
    {
      loader->SetDataPath("Campaigns\\");
      CombatGroup* g = CombatGroup::LoadOrderOfBattle(filename, -1, nullptr);
      forces.append(g);
    }
  }
}

CombatRoster::~CombatRoster() { forces.destroy(); }

CombatGroup* CombatRoster::GetForce(std::string_view name)
{
  ListIter<CombatGroup> iter = forces;
  while (++iter)
  {
    CombatGroup* f = iter.value();

    if (f->Name() == name)
      return f;
  }

  return nullptr;
}

void CombatRoster::Initialize() { roster = NEW CombatRoster(); }

void CombatRoster::Close()
{
  delete roster;
  roster = nullptr;
}

CombatRoster* CombatRoster::GetInstance() { return roster; }

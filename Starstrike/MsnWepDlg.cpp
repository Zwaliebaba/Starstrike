#include "pch.h"
#include "MsnWepDlg.h"
#include "Button.h"
#include "Campaign.h"
#include "Game.h"
#include "HardPoint.h"
#include "Keyboard.h"
#include "ListBox.h"
#include "Mission.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "WeaponDesign.h"

DEF_MAP_CLIENT(MsnWepDlg, OnCommit);
DEF_MAP_CLIENT(MsnWepDlg, OnCancel);
DEF_MAP_CLIENT(MsnWepDlg, OnTabButton);
DEF_MAP_CLIENT(MsnWepDlg, OnMount);
DEF_MAP_CLIENT(MsnWepDlg, OnLoadout);

MsnWepDlg::MsnWepDlg(Screen* s, FormDef& def, PlanScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    MsnDlg(mgr),
    player_desc(nullptr),
    beauty(nullptr),
    elem(nullptr),
    first_station(0)
{
  campaign = Campaign::GetCampaign();

  if (campaign)
    mission = campaign->GetMission();

  ZeroMemory(designs, sizeof(designs));
  ZeroMemory(mounts, sizeof(mounts));
  Init(def);
}

MsnWepDlg::~MsnWepDlg() {}

void MsnWepDlg::RegisterControls()
{
  lbl_element = FindControl(601);
  lbl_type = FindControl(602);
  lbl_weight = FindControl(603);
  loadout_list = static_cast<ListBox*>(FindControl(604));
  beauty = static_cast<ImageBox*>(FindControl(300));
  player_desc = FindControl(301);

  if (loadout_list)
    REGISTER_CLIENT(EID_SELECT, loadout_list, MsnWepDlg, OnLoadout);

  for (int i = 0; i < 8; i++)
  {
    lbl_desc[i] = FindControl(500 + i * 10);
    lbl_station[i] = FindControl(401 + i);

    for (int n = 0; n < 8; n++)
    {
      btn_load[i][n] = static_cast<Button*>(FindControl(500 + i * 10 + n + 1));

      if (btn_load[i][n])
      {
        if (i == 0)
        {
          if (n == 0)
            btn_load[i][n]->GetPicture(led_off);
          else if (n == 1)
            btn_load[i][n]->GetPicture(led_on);
        }

        btn_load[i][n]->SetPicture(led_off);
        btn_load[i][n]->SetPictureLocation(4); // centered
        REGISTER_CLIENT(EID_CLICK, btn_load[i][n], MsnWepDlg, OnMount);
      }
    }
  }

  RegisterMsnControls(this);

  if (commit)
    REGISTER_CLIENT(EID_CLICK, commit, MsnWepDlg, OnCommit);

  if (cancel)
    REGISTER_CLIENT(EID_CLICK, cancel, MsnWepDlg, OnCancel);

  if (sit_button)
    REGISTER_CLIENT(EID_CLICK, sit_button, MsnWepDlg, OnTabButton);

  if (pkg_button)
    REGISTER_CLIENT(EID_CLICK, pkg_button, MsnWepDlg, OnTabButton);

  if (nav_button)
    REGISTER_CLIENT(EID_CLICK, nav_button, MsnWepDlg, OnTabButton);

  if (wep_button)
    REGISTER_CLIENT(EID_CLICK, wep_button, MsnWepDlg, OnTabButton);
}

void MsnWepDlg::Show()
{
  FormWindow::Show();
  ShowMsnDlg();

  if (mission)
  {
    for (int i = 0; i < mission->GetElements().size(); i++)
    {
      MissionElement* e = mission->GetElements().at(i);
      if (e->Player())
      {
        elem = e;
        break;
      }
    }
  }

  if (elem)
    SetupControls();
}

void MsnWepDlg::SetupControls()
{
  auto design = (ShipDesign*)elem->GetDesign();

  if (lbl_element)
    lbl_element->SetText(elem->Name());

  if (lbl_type)
    lbl_type->SetText(design->m_name);

  BuildLists();

  for (int i = 0; i < 8; i++)
  {
    if (!lbl_desc[i])
      continue;

    if (designs[i])
    {
      lbl_desc[i]->Show();
      lbl_desc[i]->SetText(designs[i]->group + " " + designs[i]->name);

      for (int n = 0; n < 8; n++)
      {
        if (mounts[i][n])
        {
          btn_load[i][n]->Show();
          btn_load[i][n]->SetPicture((loads[n] == i) ? led_on : led_off);
        }
        else
          btn_load[i][n]->Hide();
      }
    }
    else
    {
      lbl_desc[i]->Hide();

      for (int n = 0; n < 8; n++)
        btn_load[i][n]->Hide();
    }
  }

  double loaded_mass = 0;
  char weight[32];

  if (loadout_list)
  {
    loadout_list->ClearItems();

    if (design)
    {
      ListIter<ShipLoad> sl = static_cast<List<ShipLoad>&>(design->loadouts);
      while (++sl)
      {
        ShipLoad* load = sl.value();
        int item = loadout_list->AddItem(load->name) - 1;

        sprintf_s(weight, "%d kg", static_cast<int>((design->mass + load->mass) * 1000));
        loadout_list->SetItemText(item, 1, weight);
        loadout_list->SetItemData(item, 1, static_cast<DWORD>(load->mass * 1000));

        if (elem->Loadouts().size() > 0 && elem->Loadouts().at(0)->GetName() == load->name)
        {
          loadout_list->SetSelected(item, true);
          loaded_mass = design->mass + load->mass;
        }
      }
    }
  }

  if (lbl_weight && design)
  {
    if (loaded_mass < 1)
      loaded_mass = design->mass;

    sprintf_s(weight, "%d kg", static_cast<int>(loaded_mass * 1000));
    lbl_weight->SetText(weight);
  }

  if (beauty && design)
    beauty->SetPicture(design->beauty);

  if (player_desc && design)
  {
    char txt[256];

    if (design->type <= Ship::ATTACK)
      sprintf_s(txt, "%s %s", design->m_abrv.c_str(), design->display_name.c_str());
    else
      sprintf_s(txt, "%s %s", design->m_abrv.c_str(), elem->Name().c_str());

    player_desc->SetText(txt);
  }
}

void MsnWepDlg::BuildLists()
{
  ZeroMemory(designs, sizeof(designs));
  ZeroMemory(mounts, sizeof(mounts));

  if (elem)
  {
    auto d = (ShipDesign*)elem->GetDesign();
    int nstations = d->hard_points.size();

    first_station = (8 - nstations) / 2;

    int index = 0;
    int station = first_station;

    for (int s = 0; s < 8; s++)
    {
      if (lbl_station[s])
        lbl_station[s]->SetText("");
    }

    ListIter<HardPoint> iter = d->hard_points;
    while (++iter)
    {
      HardPoint* hp = iter.value();

      if (lbl_station[station])
        lbl_station[station]->SetText(hp->GetAbbreviation());

      for (int n = 0; n < HardPoint::MAX_DESIGNS; n++)
      {
        WeaponDesign* wep_dsn = hp->GetWeaponDesign(n);

        if (wep_dsn)
        {
          bool found = false;

          for (int i = 0; i < 8 && !found; i++)
          {
            if (designs[i] == wep_dsn)
            {
              found = true;
              mounts[i][station] = true;
            }
          }

          if (!found)
          {
            mounts[index][station] = true;
            designs[index++] = wep_dsn;
          }
        }
      }

      station++;
    }

    if (elem->Loadouts().size())
    {
      MissionLoad* msn_load = elem->Loadouts().at(0);

      for (int i = 0; i < 8; i++)
        loads[i] = -1;

      // map loadout:
      int* loadout = nullptr;
      if (msn_load->GetName().length())
      {
        ListIter<ShipLoad> sl = ((ShipDesign*)elem->GetDesign())->loadouts;
        while (++sl)
        {
          if (sl->name == msn_load->GetName())
            loadout = sl->load;
        }
      }
      else
        loadout = msn_load->GetStations();

      for (int i = 0; i < nstations; i++)
        loads[i + first_station] = loadout[i];
    }
  }
}

void MsnWepDlg::ExecFrame()
{
  if (Keyboard::KeyDown(VK_RETURN))
    OnCommit(nullptr);
}

int MsnWepDlg::LoadToPointIndex(int n)
{
  int nn = n + first_station;

  if (!elem || nn < 0 || nn >= 8 || loads[nn] == -1)
    return -1;

  int index = -1;
  WeaponDesign* wep_design = designs[loads[nn]];
  auto design = (ShipDesign*)elem->GetDesign();
  HardPoint* hard_point = design->hard_points[n];

  for (int i = 0; i < 8 && index < 0; i++)
  {
    if (hard_point->GetWeaponDesign(i) == wep_design)
      index = i;
  }

  return index;
}

int MsnWepDlg::PointIndexToLoad(int n, int index)
{
  int nn = n + first_station;

  if (!elem || nn < 0 || nn >= 8)
    return -1;

  int result = -1;
  auto design = (ShipDesign*)elem->GetDesign();
  HardPoint* hard_point = design->hard_points[n];
  WeaponDesign* wep_design = hard_point->GetWeaponDesign(index);

  for (int i = 0; i < 8 && result < 0; i++)
  {
    if (designs[i] == wep_design)
      result = i;
  }

  return result;
}

void MsnWepDlg::OnMount(AWEvent* event)
{
  int station = -1;
  int item = -1;

  for (int i = 0; i < 8 && item < 0; i++)
  {
    for (int n = 0; n < 8 && station < 0; n++)
    {
      if (btn_load[i][n] == event->window)
      {
        station = n;
        item = i;
      }
    }
  }

  if (item >= 0 && station >= 0)
  {
    if (loads[station] == item)
      item = -1;

    loads[station] = item;

    for (int n = 0; n < 8; n++)
      btn_load[n][station]->SetPicture(n == item ? led_on : led_off);

    if (elem)
    {
      int nstations = elem->GetDesign()->hard_points.size();

      if (elem->Loadouts().size() < 1)
      {
        auto l = NEW MissionLoad;
        elem->Loadouts().append(l);

        for (int n = 0; n < nstations; n++)
          l->SetStation(n, LoadToPointIndex(n));
      }
      else
      {
        ListIter<MissionLoad> l = elem->Loadouts();
        while (++l)
        {
          // if the player customizes the loadout,
          // tell the sim loader not to use a named
          // loadout from the ship design:
          l->SetName("");

          for (int n = 0; n < nstations; n++)
            l->SetStation(n, LoadToPointIndex(n));
        }
      }
    }
  }

  if (loadout_list)
    loadout_list->ClearSelection();

  if (lbl_weight && elem)
  {
    auto d = (ShipDesign*)elem->GetDesign();
    int nstations = d->hard_points.size();
    double mass = d->mass;

    for (int n = 0; n < nstations; n++)
    {
      int item = loads[n + first_station];
      mass += d->hard_points[n]->GetCarryMass(item);
    }

    char weight[32];
    sprintf_s(weight, "%d kg", static_cast<int>(mass * 1000));
    lbl_weight->SetText(weight);
  }
}

void MsnWepDlg::OnLoadout(AWEvent* event)
{
  if (!elem)
    return;

  auto design = (ShipDesign*)elem->GetDesign();
  ShipLoad* shipload = nullptr;

  if (loadout_list && design)
  {
    int index = loadout_list->GetListIndex();
    std::string loadname = loadout_list->GetItemText(index);

    ListIter<ShipLoad> sl = static_cast<List<ShipLoad>&>(design->loadouts);
    while (++sl)
    {
      if (sl->name == loadname)
        shipload = sl.value();
    }

    if (!shipload)
      return;

    if (lbl_weight)
    {
      char weight[32];
      sprintf_s(weight, "%d kg", static_cast<int>((design->mass + shipload->mass) * 1000));
      lbl_weight->SetText(weight);
    }

    if (elem->Loadouts().size() < 1)
    {
      auto l = NEW MissionLoad(-1, shipload->name);
      elem->Loadouts().append(l);
    }
    else
    {
      ListIter<MissionLoad> l = elem->Loadouts();
      while (++l)
      {
        // if the player chooses a std loadout,
        // tell the sim loader to use a named
        // loadout from the ship design:
        l->SetName(shipload->name);
      }
    }

    int nstations = design->hard_points.size();
    int* loadout = shipload->load;

    for (int i = 0; i < 8; i++)
      loads[i] = -1;

    for (int i = 0; i < nstations; i++)
      loads[i + first_station] = PointIndexToLoad(i, loadout[i]);

    for (int i = 0; i < 8; i++)
    {
      for (int n = 0; n < 8; n++)
        btn_load[i][n]->SetPicture(i == loads[n] ? led_on : led_off);
    }
  }
}

void MsnWepDlg::OnCommit(AWEvent* event) { MsnDlg::OnCommit(event); }

void MsnWepDlg::OnCancel(AWEvent* event) { MsnDlg::OnCancel(event); }

void MsnWepDlg::OnTabButton(AWEvent* event) { MsnDlg::OnTabButton(event); }

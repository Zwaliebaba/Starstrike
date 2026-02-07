#include "pch.h"
#include "FltDlg.h"
#include "Button.h"
#include "CombatGroup.h"
#include "ComboBox.h"
#include "Element.h"
#include "FlightDeck.h"
#include "FlightPlanner.h"
#include "FormatUtil.h"
#include "Game.h"
#include "GameScreen.h"
#include "Hangar.h"
#include "Instruction.h"
#include "Intel.h"
#include "ListBox.h"
#include "Mission.h"
#include "NetUtil.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Sim.h"

DEF_MAP_CLIENT(FltDlg, OnFilter);
DEF_MAP_CLIENT(FltDlg, OnPackage);
DEF_MAP_CLIENT(FltDlg, OnAlert);
DEF_MAP_CLIENT(FltDlg, OnLaunch);
DEF_MAP_CLIENT(FltDlg, OnStandDown);
DEF_MAP_CLIENT(FltDlg, OnRecall);
DEF_MAP_CLIENT(FltDlg, OnClose);
DEF_MAP_CLIENT(FltDlg, OnMissionType);

static const ShipDesign* design = nullptr;

FltDlg::FltDlg(Screen* s, FormDef& def, GameScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    filter_list(nullptr),
    hangar_list(nullptr),
    package_btn(nullptr),
    alert_btn(nullptr),
    launch_btn(nullptr),
    stand_btn(nullptr),
    recall_btn(nullptr),
    mission_type(-1),
    ship(nullptr),
    flight_planner(nullptr),
    patrol_pattern(0) { Init(def); }

FltDlg::~FltDlg() { delete flight_planner; }

void FltDlg::RegisterControls()
{
  filter_list = static_cast<ComboBox*>(FindControl(101));
  hangar_list = static_cast<ListBox*>(FindControl(102));
  package_btn = static_cast<Button*>(FindControl(110));
  alert_btn = static_cast<Button*>(FindControl(111));
  launch_btn = static_cast<Button*>(FindControl(112));
  stand_btn = static_cast<Button*>(FindControl(113));
  recall_btn = static_cast<Button*>(FindControl(114));
  close_btn = static_cast<Button*>(FindControl(1));

  if (filter_list)
    REGISTER_CLIENT(EID_SELECT, filter_list, FltDlg, OnFilter);

  if (package_btn)
    REGISTER_CLIENT(EID_CLICK, package_btn, FltDlg, OnPackage);

  if (alert_btn)
    REGISTER_CLIENT(EID_CLICK, alert_btn, FltDlg, OnAlert);

  if (launch_btn)
    REGISTER_CLIENT(EID_CLICK, launch_btn, FltDlg, OnLaunch);

  if (stand_btn)
    REGISTER_CLIENT(EID_CLICK, stand_btn, FltDlg, OnStandDown);

  if (recall_btn)
    REGISTER_CLIENT(EID_CLICK, recall_btn, FltDlg, OnRecall);

  if (close_btn)
    REGISTER_CLIENT(EID_CLICK, close_btn, FltDlg, OnClose);

  for (int i = 0; i < 6; i++)
  {
    mission_btn[i] = static_cast<Button*>(FindControl(210 + i));
    if (mission_btn[i])
      REGISTER_CLIENT(EID_CLICK, mission_btn[i], FltDlg, OnMissionType);
  }

  objective_list = static_cast<ListBox*>(FindControl(221));
  loadout_list = static_cast<ListBox*>(FindControl(222));
}

void FltDlg::SetShip(Ship* s)
{
  if (ship != s)
  {
    ship = s;

    delete flight_planner;
    flight_planner = nullptr;

    if (filter_list)
    {
      filter_list->ClearItems();

      if (ship)
      {
        int nsquadrons = 0;
        int nslots = 0;
        Hangar* hangar = ship->GetHangar();

        if (hangar)
        {
          nsquadrons = hangar->NumSquadrons();

          for (int i = 0; i < nsquadrons; i++)
          {
            char filter[64];
            sprintf_s(filter, "%s %s", hangar->SquadronDesign(i)->m_abrv.c_str(), hangar->SquadronName(i).data());

            filter_list->AddItem(filter);
          }

          filter_list->AddItem(Game::GetText("FltDlg.PENDING"));
          filter_list->AddItem(Game::GetText("FltDlg.ACTIVE"));
        }

        flight_planner = NEW FlightPlanner(ship);
      }

      OnFilter(nullptr);
    }
  }
}

void FltDlg::ExecFrame()
{
  if (!ship || !ship->GetHangar())
    manager->HideFltDlg();
  else
  {
    UpdateSelection();
    UpdateObjective();
  }
}

void FltDlg::UpdateSelection()
{
  if (!filter_list || !hangar_list || !ship)
    return;

  design = nullptr;

  bool package = false;
  bool alert = false;
  bool launch = false;
  bool stand = false;
  bool recall = false;

  Hangar* hangar = ship->GetHangar();
  int seln = filter_list->GetSelectedIndex();
  char txt[32];
  int item;

  // selected squadron:
  if (seln < hangar->NumSquadrons())
  {
    int nslots = hangar->SquadronSize(seln);

    for (item = 0; item < hangar_list->NumItems(); item++)
    {
      int i = hangar_list->GetItemData(item);
      const HangarSlot* s = hangar->GetSlot(seln, i);

      if (hangar->GetState(s) == Hangar::UNAVAIL)
        hangar_list->SetItemColor(item, Color::DarkGray);
      else if (hangar->GetState(s) == Hangar::MAINT)
        hangar_list->SetItemColor(item, Color::Gray);
      else
        hangar_list->SetItemColor(item, Color::White);

      if (hangar->GetState(s) > Hangar::STORAGE)
      {
        if (hangar->GetShip(s))
          hangar_list->SetItemText(item, 1, hangar->GetShip(s)->Name());
        else if (hangar->GetPackageElement(s))
          hangar_list->SetItemText(item, 1, hangar->GetPackageElement(s)->Name());
        else
          hangar_list->SetItemText(item, 1, hangar->SquadronName(seln));

        if (hangar->GetPackageElement(s))
          hangar_list->SetItemText(item, 3, Mission::RoleName(hangar->GetPackageElement(s)->Type()));
        else
          hangar_list->SetItemText(item, 3, "--");
      }
      else
      {
        hangar_list->SetItemText(item, 1, "--");
        hangar_list->SetItemText(item, 3, "--");
      }

      hangar_list->SetItemText(item, 2, hangar->StatusName(s));

      if (hangar->GetState(s) >= Hangar::ACTIVE)
      {
        FormatTime(txt, hangar->GetShip(s)->MissionClock() / 1000);
        hangar_list->SetItemText(item, 4, txt);
      }

      else if (hangar->GetState(s) == Hangar::MAINT || hangar->GetState(s) > Hangar::STORAGE)
      {
        FormatTime(txt, hangar->TimeRemaining(s));
        hangar_list->SetItemText(item, 4, txt);
      }

      else
        hangar_list->SetItemText(item, 4, "");

      if (hangar_list->IsSelected(item))
      {
        if (!design)
          design = hangar->GetDesign(s);

        switch (hangar->GetState(s))
        {
          case Hangar::STORAGE:
            alert = true;
            break;
          case Hangar::ALERT:
            launch = true;
            stand = true;
            break;
          case Hangar::QUEUED:
            stand = true;
            break;
          case Hangar::ACTIVE:
            recall = true;
            break;
        }
      }
    }
  }

  // selected pending filter:
  else if (seln == hangar->NumSquadrons())
  {
    for (item = 0; item < hangar_list->NumItems(); item++)
    {
      int f = hangar_list->GetItemData(item, 1);
      int i = hangar_list->GetItemData(item, 2);

      int squadron = -1;
      int slot = -1;
      const HangarSlot* s = nullptr;

      FlightDeck* deck = ship->GetFlightDeck(f);

      if (deck->IsLaunchDeck())
      {
        int state = deck->State(i);
        int seq = deck->Sequence(i);
        double time = deck->TimeRemaining(i);
        Ship* deckship = deck->GetShip(i);

        if (deckship)
        {
          hangar_list->SetItemText(item, 1, deckship->Name());

          for (int a = 0; !s && a < hangar->NumSquadrons(); a++)
          {
            for (int b = 0; !s && b < hangar->SquadronSize(a); b++)
            {
              const HangarSlot* test = hangar->GetSlot(a, b);
              if (hangar->GetShip(test) == deckship)
              {
                s = test;
                squadron = a;
                slot = b;
              }
            }
          }

          if (s)
          {
            hangar_list->SetItemText(item, 2, hangar->StatusName(s));
            if (hangar->GetPackageElement(s))
              hangar_list->SetItemText(item, 3, Mission::RoleName(hangar->GetPackageElement(s)->Type()));
          }
        }
        else
        {
          hangar_list->SetItemText(item, 1, "--");
          hangar_list->SetItemText(item, 2, Game::GetText("FltDlg.Open"));
        }

        FormatTime(txt, time);
        hangar_list->SetItemText(item, 4, txt);
        hangar_list->SetItemData(item, 3, squadron);
        hangar_list->SetItemData(item, 4, slot);

        if (hangar_list->IsSelected(item) && s)
        {
          if (!design)
            design = hangar->GetDesign(s);

          switch (hangar->GetState(s))
          {
            case Hangar::ALERT:
              launch = true;
              stand = true;
              break;
            case Hangar::QUEUED:
              stand = true;
              break;
          }
        }
      }
    }
  }

  // selected active filter:
  else if (seln == hangar->NumSquadrons() + 1)
  {
    int last_index = -1;

    for (item = 0; item < hangar_list->NumItems(); item++)
    {
      int squadron = hangar_list->GetItemData(item, 1);
      int slot = hangar_list->GetItemData(item, 2);

      int nslots = hangar->SquadronSize(squadron);

      if (slot >= 0 && slot < nslots)
      {
        const HangarSlot* s = hangar->GetSlot(squadron, slot);

        if (hangar->GetState(s) > Hangar::STORAGE)
        {
          if (hangar->GetShip(s))
            hangar_list->SetItemText(item, 1, hangar->GetShip(s)->Name());
          else if (hangar->GetPackageElement(s))
            hangar_list->SetItemText(item, 1, hangar->GetPackageElement(s)->Name());
          else
            hangar_list->SetItemText(item, 1, hangar->SquadronName(squadron));

          if (hangar->GetPackageElement(s))
            hangar_list->SetItemText(item, 3, Mission::RoleName(hangar->GetPackageElement(s)->Type()));
          else
            hangar_list->SetItemText(item, 3, "--");

          hangar_list->SetItemText(item, 2, hangar->StatusName(s));

          FormatTime(txt, hangar->GetShip(s)->MissionClock() / 1000);
          hangar_list->SetItemText(item, 4, txt);

          if (last_index < static_cast<int>(hangar_list->GetItemData(item)))
            last_index = static_cast<int>(hangar_list->GetItemData(item));
        }
        else
        {
          hangar_list->RemoveItem(item);
          item--;
        }
      }
      else
      {
        hangar_list->RemoveItem(item);
        item--;
      }
    }

    for (int i = 0; i < hangar->NumSquadrons(); i++)
    {
      int nslots = hangar->SquadronSize(i);

      for (int j = 0; j < nslots; j++)
      {
        const HangarSlot* s = hangar->GetSlot(i, j);

        if (hangar->GetState(s) >= Hangar::ACTIVE)
        {
          bool found = false;

          for (int n = 0; !found && n < hangar_list->NumItems(); n++)
          {
            if (hangar_list->GetItemData(n, 1) == static_cast<DWORD>(i) && hangar_list->GetItemData(n, 2) == static_cast
              <DWORD>(j))
              found = true;
          }

          if (!found)
          {
            last_index++;

            char txt[32];
            sprintf_s(txt, "%2d    ", last_index + 1);
            hangar_list->AddItemWithData(txt, last_index); // use data for sort

            if (hangar->GetShip(s))
              hangar_list->SetItemText(item, 1, hangar->GetShip(s)->Name());

            hangar_list->SetItemText(item, 2, hangar->StatusName(s));

            if (hangar->GetPackageElement(s))
              hangar_list->SetItemText(item, 3, Mission::RoleName(hangar->GetPackageElement(s)->Type()));

            FormatTime(txt, hangar->GetShip(s)->MissionClock() / 1000);
            hangar_list->SetItemText(item, 4, txt);

            hangar_list->SetItemData(item, 1, i);
            hangar_list->SetItemData(item, 2, j);

            item++;
          }
        }
      }
    }

    if (hangar_list->GetSelCount() > 0)
      recall = true;
  }

  if (package_btn)
  {
    bool pkg_ok = alert && mission_type > -1;

    if (pkg_ok && mission_type > 0)
    {
      pkg_ok = objective_list && objective_list->GetSelCount() > 0;

      if (pkg_ok)
      {
        int obj_index = objective_list->GetSelection();
        DWORD obj_data = objective_list->GetItemData(obj_index, 2);

        if (obj_data > 1e9)
          pkg_ok = false;
      }
    }

    package_btn->SetEnabled(pkg_ok);
  }

  if (alert_btn)
  {
    alert_btn->SetEnabled(alert);

    for (int i = 0; i < 6; i++)
    {
      if (mission_btn[i])
        mission_btn[i]->SetEnabled(alert);
    }
  }

  if (launch_btn)
    launch_btn->SetEnabled(launch);

  if (stand_btn)
    stand_btn->SetEnabled(stand);

  if (recall_btn)
    recall_btn->SetEnabled(recall);
}

void FltDlg::UpdateObjective()
{
  if (!objective_list || mission_type < 0 || !ship)
    return;

  Sim* sim = Sim::GetSim();
  char txt[32];

  for (int item = 0; item < objective_list->NumItems(); item++)
  {
    auto obj_name = objective_list->GetItemText(item);

    Element* elem = sim->FindElement(obj_name);

    // if element has expired, remove it from the objective list
    if (!elem || !elem->IsActive() || elem->IsFinished())
    {
      objective_list->RemoveItem(item);
      item--;
    }

    // otherwise, the element is still active, so update range/region
    else
    {
      Ship* s = elem->GetShip(1);
      double r = 0;
      bool con = false;

      if (s)
      {
        Point s_loc = s->Location() + s->GetRegion()->Location();
        Point h_loc = ship->Location() + ship->GetRegion()->Location();

        r = (s_loc - h_loc).length();

        con = ship->FindContact(s) != nullptr;

        if (con)
          FormatNumber(txt, r);
        else
        {
          strcpy_s(txt, Game::GetText("FltDlg.Unknown").data());
          r = 2e9;
        }
      }

      objective_list->SetItemText(item, 1, s->GetRegion()->Name());

      objective_list->SetItemText(item, 2, txt);
      objective_list->SetItemData(item, 2, static_cast<DWORD>(r));
    }
  }
}

void FltDlg::Show()
{
  if (shown)
    return;

  FormWindow::Show();
  UpdateSelection();
  UpdateObjective();
}

void FltDlg::Hide() { FormWindow::Hide(); }

void FltDlg::OnFilter(AWEvent* event)
{
  if (!filter_list || !hangar_list)
    return;

  int seln = filter_list->GetSelectedIndex();

  hangar_list->ClearItems();

  if (!ship)
    return;

  Hangar* hangar = ship->GetHangar();

  // selected squadron:
  if (seln < hangar->NumSquadrons())
  {
    int nslots = hangar->SquadronSize(seln);

    for (int i = 0; i < nslots; i++)
    {
      char txt[32];
      sprintf_s(txt, " %2d    ", i + 1);

      const HangarSlot* s = hangar->GetSlot(seln, i);
      hangar_list->AddItemWithData(txt, i);

      hangar_list->SetItemText(i, 1, "--");
      hangar_list->SetItemText(i, 2, hangar->StatusName(s));

      FormatTime(txt, hangar->TimeRemaining(s));
      hangar_list->SetItemText(i, 4, txt);
    }
  }

  // selected pending filter:
  else if (seln == hangar->NumSquadrons())
  {
    int item = 0;

    for (int f = 0; f < ship->NumFlightDecks(); f++)
    {
      FlightDeck* deck = ship->GetFlightDeck(f);

      if (deck->IsLaunchDeck())
      {
        for (int i = 0; i < deck->NumSlots(); i++)
        {
          int state = deck->State(i);
          int seq = deck->Sequence(i);
          double time = deck->TimeRemaining(i);
          Ship* deckship = deck->GetShip(i);

          int squadron = -1;
          int slot = -1;

          char txt[32];
          sprintf_s(txt, "%d-%d    ", f + 1, i + 1);

          hangar_list->AddItemWithData(txt, item); // use data for sort

          if (deckship)
          {
            hangar_list->SetItemText(item, 1, deckship->Name());

            const HangarSlot* s = nullptr;

            for (int a = 0; !s && a < hangar->NumSquadrons(); a++)
            {
              for (int b = 0; !s && b < hangar->SquadronSize(a); b++)
              {
                const HangarSlot* test = hangar->GetSlot(a, b);
                if (hangar->GetShip(test) == deckship)
                {
                  s = test;
                  squadron = a;
                  slot = b;
                }
              }
            }

            if (s)
            {
              hangar_list->SetItemText(item, 2, hangar->StatusName(s));
              if (hangar->GetPackageElement(s))
                hangar_list->SetItemText(item, 3, Mission::RoleName(hangar->GetPackageElement(s)->Type()));
            }
          }
          else
          {
            hangar_list->SetItemText(item, 1, "--");
            hangar_list->SetItemText(item, 2, Game::GetText("FltDlg.Open"));
          }

          FormatTime(txt, time);
          hangar_list->SetItemText(item, 4, txt);

          hangar_list->SetItemData(item, 1, f);
          hangar_list->SetItemData(item, 2, i);
          hangar_list->SetItemData(item, 3, squadron);
          hangar_list->SetItemData(item, 4, slot);

          item++;
        }
      }
    }
  }

  // selected active filter:
  else if (seln == hangar->NumSquadrons() + 1)
  {
    int item = 0;

    for (int i = 0; i < hangar->NumSquadrons(); i++)
    {
      int nslots = hangar->SquadronSize(i);

      for (int j = 0; j < nslots; j++)
      {
        const HangarSlot* s = hangar->GetSlot(i, j);

        if (hangar->GetState(s) >= Hangar::ACTIVE)
        {
          char txt[32];
          sprintf_s(txt, " %2d    ", item + 1);

          hangar_list->AddItemWithData(txt, item); // use data for sort

          if (hangar->GetShip(s))
            hangar_list->SetItemText(item, 1, hangar->GetShip(s)->Name());

          hangar_list->SetItemText(item, 2, hangar->StatusName(s));

          if (hangar->GetPackageElement(s))
            hangar_list->SetItemText(item, 3, Mission::RoleName(hangar->GetPackageElement(s)->Type()));

          FormatTime(txt, hangar->TimeRemaining(s));
          hangar_list->SetItemText(item, 4, txt);

          hangar_list->SetItemData(item, 1, i);
          hangar_list->SetItemData(item, 2, j);

          item++;
        }
      }
    }
  }
}

void FltDlg::OnPackage(AWEvent* event)
{
  if (!filter_list || !hangar_list || !ship)
    return;

  int code = Mission::PATROL;

  switch (mission_type)
  {
    case 0:
      code = Mission::PATROL;
      break;
    case 1:
      code = Mission::INTERCEPT;
      break;
    case 2:
      code = Mission::ASSAULT;
      break;
    case 3:
      code = Mission::STRIKE;
      break;
    case 4:
      code = Mission::ESCORT;
      break;
    case 5:
      code = Mission::INTEL;
      break;
  }

  int squad = filter_list->GetSelectedIndex();
  Hangar* hangar = ship->GetHangar();
  Sim* sim = Sim::GetSim();
  auto call = sim->FindAvailCallsign(ship->GetIFF());
  Element* elem = sim->CreateElement(call, ship->GetIFF(), code);
  Element* tgt = nullptr;
  FlightDeck* deck = nullptr;
  int queue = 1000;
  int* load = nullptr;

  elem->SetSquadron(hangar->SquadronName(squad));
  elem->SetCarrier(ship);

  if (objective_list)
  {
    int index = objective_list->GetListIndex();
    std::string target = objective_list->GetItemText(index);

    auto objective = NEW Instruction(code, target.data());
    elem->AddObjective(objective);

    tgt = sim->FindElement(target.data());
  }

  if (loadout_list && design)
  {
    int index = loadout_list->GetListIndex();
    std::string loadname = loadout_list->GetItemText(index);

    ListIter<ShipLoad> sl = (List<ShipLoad>&)design->loadouts;
    while (++sl && !load)
    {
      if (sl->name == loadname)
      {
        load = sl->load;
        elem->SetLoadout(load);
      }
    }
  }

  for (int i = 0; i < ship->NumFlightDecks(); i++)
  {
    FlightDeck* d = ship->GetFlightDeck(i);

    if (d && d->IsLaunchDeck())
    {
      int dq = hangar->PreflightQueue(d);

      if (dq < queue)
      {
        queue = dq;
        deck = d;
      }
    }
  }

  int npackage = 0;
  int slots[4];

  for (int i = 0; i < 4; i++)
    slots[i] = -1;

  for (int i = 0; i < hangar_list->NumItems(); i++)
  {
    if (hangar_list->IsSelected(i))
    {
      int nslot = hangar_list->GetItemData(i);
      hangar->GotoAlert(squad, nslot, deck, elem, load, true);
      slots[npackage] = nslot;
      hangar_list->SetSelected(i, false);
      npackage++;

      if (npackage >= 4)
        break;
    }
  }

  NetUtil::SendElemCreate(elem, squad, slots, false);

  if (flight_planner)
  {
    switch (mission_type)
    {
      case 0: default:
        flight_planner->CreatePatrolRoute(elem, patrol_pattern++);
        break;

      case 1:
      case 2:
      case 3:
        if (tgt)
          flight_planner->CreateStrikeRoute(elem, tgt);
        else
          flight_planner->CreatePatrolRoute(elem, patrol_pattern++);
        break;

      case 4:
        if (tgt)
          flight_planner->CreateEscortRoute(elem, tgt);
        else
          flight_planner->CreatePatrolRoute(elem, patrol_pattern++);
        break;
    }

    if (patrol_pattern < 0 || patrol_pattern > 3)
      patrol_pattern = 0;
  }
}

void FltDlg::OnAlert(AWEvent* event)
{
  if (!filter_list || !hangar_list || !ship)
    return;

  int squad = filter_list->GetSelectedIndex();
  Hangar* hangar = ship->GetHangar();
  Sim* sim = Sim::GetSim();
  auto call = sim->FindAvailCallsign(ship->GetIFF());
  Element* elem = sim->CreateElement(call, ship->GetIFF());
  FlightDeck* deck = nullptr;
  int queue = 1000;
  const int* load = nullptr;

  elem->SetSquadron(hangar->SquadronName(squad));
  elem->SetCarrier(ship);

  for (int i = 0; i < ship->NumFlightDecks(); i++)
  {
    FlightDeck* d = ship->GetFlightDeck(i);

    if (d && d->IsLaunchDeck())
    {
      int dq = hangar->PreflightQueue(d);

      if (dq < queue)
      {
        queue = dq;
        deck = d;
      }
    }
  }

  int nalert = 0;
  int slots[4];

  for (int i = 0; i < 4; i++)
    slots[i] = -1;

  for (int i = 0; i < hangar_list->NumItems(); i++)
  {
    if (hangar_list->IsSelected(i))
    {
      int nslot = hangar_list->GetItemData(i);
      slots[nalert] = nslot;

      if (!load)
      {
        const HangarSlot* hangar_slot = hangar->GetSlot(squad, nslot);
        if (hangar_slot)
        {
          load = hangar->GetLoadout(hangar_slot);
          elem->SetLoadout((int*)load);
        }
      }

      hangar->GotoAlert(squad, nslot, deck, elem);
      hangar_list->SetSelected(i, false);
      nalert++;

      if (nalert >= 4)
        break;
    }
  }

  NetUtil::SendElemCreate(elem, squad, slots, true);
}

void FltDlg::OnLaunch(AWEvent* event)
{
  if (!filter_list || !hangar_list || !ship)
    return;

  int squad = filter_list->GetSelectedIndex();

  Hangar* hangar = ship->GetHangar();

  // selected squadron:
  if (squad < hangar->NumSquadrons())
  {
    for (int i = 0; i < hangar_list->NumItems(); i++)
    {
      if (hangar_list->IsSelected(i))
      {
        int nslot = hangar_list->GetItemData(i);
        hangar->Launch(squad, nslot);
        NetUtil::SendShipLaunch(ship, squad, nslot);
      }
    }
  }

  // selected pending filter:
  else if (squad == hangar->NumSquadrons())
  {
    for (int item = 0; item < hangar_list->NumItems(); item++)
    {
      if (hangar_list->IsSelected(item))
      {
        int squadron = hangar_list->GetItemData(item, 3);
        int slot = hangar_list->GetItemData(item, 4);

        if (squadron >= 0 && slot >= 0)
        {
          hangar->Launch(squadron, slot);
          NetUtil::SendShipLaunch(ship, squadron, slot);
        }
      }
    }
  }
}

void FltDlg::OnStandDown(AWEvent* event)
{
  if (!filter_list || !hangar_list || !ship)
    return;

  int seln = filter_list->GetSelectedIndex();

  Hangar* hangar = ship->GetHangar();

  // selected squadron:
  if (seln < hangar->NumSquadrons())
  {
    for (int i = 0; i < hangar_list->NumItems(); i++)
    {
      if (hangar_list->IsSelected(i))
      {
        int nslot = hangar_list->GetItemData(i);
        hangar->StandDown(seln, nslot);
      }
    }
  }

  // selected pending filter:
  else if (seln == hangar->NumSquadrons())
  {
    for (int item = 0; item < hangar_list->NumItems(); item++)
    {
      if (hangar_list->IsSelected(item))
      {
        int squadron = hangar_list->GetItemData(item, 3);
        int slot = hangar_list->GetItemData(item, 4);

        if (squadron >= 0 && slot >= 0)
          hangar->StandDown(squadron, slot);
      }
    }
  }
}

void FltDlg::OnRecall(AWEvent* event)
{
  if (!filter_list || !hangar_list || !ship)
    return;

  int seln = filter_list->GetSelectedIndex();

  Hangar* hangar = ship->GetHangar();

  // selected squadron:             or selected active filter:
  if (seln < hangar->NumSquadrons() || seln == hangar->NumSquadrons() + 1)
  {
    for (int i = 0; i < hangar_list->NumItems(); i++)
    {
      if (hangar_list->IsSelected(i))
      {
        int nsquad = seln;
        int nslot = hangar_list->GetItemData(i);

        if (seln > hangar->NumSquadrons())
        {
          nsquad = hangar_list->GetItemData(i, 1);
          nslot = hangar_list->GetItemData(i, 2);
        }

        const HangarSlot* slot = hangar->GetSlot(nsquad, nslot);
        Ship* recall = hangar->GetShip(slot);

        if (recall)
        {
          auto msg = NEW RadioMessage(recall, ship, RadioMessage::RTB);
          RadioTraffic::Transmit(msg);
        }
      }
    }
  }
}

void FltDlg::OnMissionType(AWEvent* event)
{
  mission_type = -1;

  for (int i = 0; i < 6; i++)
  {
    if (mission_btn[i])
    {
      if (mission_btn[i] == event->window)
      {
        mission_btn[i]->SetButtonState(1);
        mission_type = i;
      }
      else
        mission_btn[i]->SetButtonState(0);
    }
  }

  if (objective_list && mission_type > -1)
  {
    objective_list->ClearItems();

    char txt[32];
    Sim* sim = Sim::GetSim();
    ListIter<Element> iter = sim->GetElements();

    while (++iter)
    {
      Element* elem = iter.value();

      if (!elem->IsActive() || elem->IsFinished() || elem->IsSquadron())
        continue;

      CombatGroup* group = elem->GetCombatGroup();
      int iff = elem->GetIFF();
      Ship* s = elem->GetShip(1);
      double r = 0;
      bool con = false;

      if (iff != ship->GetIFF())
      {
        if (elem->IntelLevel() < Intel::LOCATED)
          continue;

        if (group && group->IntelLevel() < Intel::LOCATED)
          continue;
      }

      if (s)
      {
        Point s_loc = s->Location() + s->GetRegion()->Location();
        Point h_loc = ship->Location() + ship->GetRegion()->Location();

        r = (s_loc - h_loc).length();

        con = ship->FindContact(s) != nullptr;

        if (con)
          FormatNumber(txt, r);
        else
        {
          strcpy_s(txt, Game::GetText("FltDlg.Unknown").data());
          r = 2e9;
        }
      }

      switch (mission_type)
      {
        case 1:  // INTERCEPT
          if (iff && iff != ship->GetIFF() && s && s->IsDropship())
          {
            int item = objective_list->AddItem(elem->Name()) - 1;
            objective_list->SetItemText(item, 1, s->GetRegion()->Name());

            objective_list->SetItemText(item, 2, txt);
            objective_list->SetItemData(item, 2, static_cast<DWORD>(r));
          }
          break;

        case 2:  // ASSAULT
          if (iff && iff != ship->GetIFF() && s && (s->IsStarship() || s->IsStatic()))
          {
            int item = objective_list->AddItem(elem->Name()) - 1;
            objective_list->SetItemText(item, 1, s->GetRegion()->Name());

            objective_list->SetItemText(item, 2, txt);
            objective_list->SetItemData(item, 2, static_cast<DWORD>(r));
          }
          break;

        case 3:  // STRIKE
          if (iff && iff != ship->GetIFF() && s && s->IsGroundUnit())
          {
            int item = objective_list->AddItem(elem->Name()) - 1;
            objective_list->SetItemText(item, 1, s->GetRegion()->Name());

            objective_list->SetItemText(item, 2, txt);
            objective_list->SetItemData(item, 2, static_cast<DWORD>(r));
          }
          break;

        case 4:  // ESCORT
          if ((iff == 0 || iff == ship->GetIFF()) && (!s || !s->IsStatic()))
          {
            int item = objective_list->AddItem(elem->Name()) - 1;

            if (s)
            {
              objective_list->SetItemText(item, 1, s->GetRegion()->Name());
              objective_list->SetItemText(item, 2, txt);
              objective_list->SetItemData(item, 2, static_cast<DWORD>(r));
            }

            else
            {
              objective_list->SetItemText(item, 1, "0");
              objective_list->SetItemData(item, 1, 0);
            }
          }
          break;

        case 5:  // SCOUT?
          break;

        default:
          break;
      }
    }
  }

  if (loadout_list && mission_type > -1)
  {
    loadout_list->ClearItems();

    if (design)
    {
      ListIter<ShipLoad> sl = (List<ShipLoad>&)design->loadouts;
      while (++sl)
      {
        int item = loadout_list->AddItem(sl->name) - 1;

        char weight[32];
        sprintf_s(weight, "%d kg", static_cast<int>((design->mass + sl->mass) * 1000));
        loadout_list->SetItemText(item, 1, weight);
        loadout_list->SetItemData(item, 1, static_cast<DWORD>(sl->mass * 1000));
      }
    }
  }
}

void FltDlg::OnClose(AWEvent* event)
{
  if (manager)
    manager->CloseTopmost();
}

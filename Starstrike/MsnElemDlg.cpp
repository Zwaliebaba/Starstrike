#include "pch.h"
#include "MsnElemDlg.h"
#include "Button.h"
#include "Campaign.h"
#include "EditBox.h"
#include "Game.h"
#include "Instruction.h"
#include "Intel.h"
#include "Keyboard.h"
#include "MenuScreen.h"
#include "Mission.h"
#include "MsnEditDlg.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Skin.h"
#include "StarSystem.h"

DEF_MAP_CLIENT(MsnElemDlg, OnAccept);
DEF_MAP_CLIENT(MsnElemDlg, OnCancel);
DEF_MAP_CLIENT(MsnElemDlg, OnClassSelect);
DEF_MAP_CLIENT(MsnElemDlg, OnDesignSelect);
DEF_MAP_CLIENT(MsnElemDlg, OnObjectiveSelect);
DEF_MAP_CLIENT(MsnElemDlg, OnIFFChange);

MsnElemDlg::MsnElemDlg(Screen* s, FormDef& def, MenuScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    edt_name(nullptr),
    cmb_class(nullptr),
    cmb_design(nullptr),
    cmb_skin(nullptr),
    edt_size(nullptr),
    edt_iff(nullptr),
    cmb_role(nullptr),
    cmb_region(nullptr),
    edt_loc_x(nullptr),
    edt_loc_y(nullptr),
    edt_loc_z(nullptr),
    cmb_heading(nullptr),
    edt_hold_time(nullptr),
    btn_player(nullptr),
    btn_playable(nullptr),
    btn_alert(nullptr),
    btn_command_ai(nullptr),
    edt_respawns(nullptr),
    cmb_carrier(nullptr),
    cmb_squadron(nullptr),
    cmb_commander(nullptr),
    cmb_intel(nullptr),
    cmb_loadout(nullptr),
    cmb_objective(nullptr),
    cmb_target(nullptr),
    btn_accept(nullptr),
    btn_cancel(nullptr),
    mission(nullptr),
    elem(nullptr),
    exit_latch(true) { Init(def); }

MsnElemDlg::~MsnElemDlg() {}

void MsnElemDlg::RegisterControls()
{
  btn_accept = static_cast<Button*>(FindControl(1));
  if (btn_accept)
    REGISTER_CLIENT(EID_CLICK, btn_accept, MsnElemDlg, OnAccept);

  btn_cancel = static_cast<Button*>(FindControl(2));
  if (btn_accept)
    REGISTER_CLIENT(EID_CLICK, btn_cancel, MsnElemDlg, OnCancel);

  edt_name = static_cast<EditBox*>(FindControl(201));
  cmb_class = static_cast<ComboBox*>(FindControl(202));
  cmb_design = static_cast<ComboBox*>(FindControl(203));
  cmb_skin = static_cast<ComboBox*>(FindControl(213));
  edt_size = static_cast<EditBox*>(FindControl(204));
  edt_iff = static_cast<EditBox*>(FindControl(205));
  cmb_role = static_cast<ComboBox*>(FindControl(206));
  cmb_region = static_cast<ComboBox*>(FindControl(207));
  edt_loc_x = static_cast<EditBox*>(FindControl(208));
  edt_loc_y = static_cast<EditBox*>(FindControl(209));
  edt_loc_z = static_cast<EditBox*>(FindControl(210));
  cmb_heading = static_cast<ComboBox*>(FindControl(211));
  edt_hold_time = static_cast<EditBox*>(FindControl(212));

  btn_player = static_cast<Button*>(FindControl(221));
  btn_alert = static_cast<Button*>(FindControl(222));
  btn_playable = static_cast<Button*>(FindControl(223));
  btn_command_ai = static_cast<Button*>(FindControl(224));
  edt_respawns = static_cast<EditBox*>(FindControl(225));
  cmb_commander = static_cast<ComboBox*>(FindControl(226));
  cmb_carrier = static_cast<ComboBox*>(FindControl(227));
  cmb_squadron = static_cast<ComboBox*>(FindControl(228));
  cmb_intel = static_cast<ComboBox*>(FindControl(229));
  cmb_loadout = static_cast<ComboBox*>(FindControl(230));
  cmb_objective = static_cast<ComboBox*>(FindControl(231));
  cmb_target = static_cast<ComboBox*>(FindControl(232));

  if (cmb_class)
    REGISTER_CLIENT(EID_SELECT, cmb_class, MsnElemDlg, OnClassSelect);

  if (cmb_design)
    REGISTER_CLIENT(EID_SELECT, cmb_design, MsnElemDlg, OnDesignSelect);

  if (cmb_objective)
    REGISTER_CLIENT(EID_SELECT, cmb_objective, MsnElemDlg, OnObjectiveSelect);

  if (edt_iff)
    REGISTER_CLIENT(EID_KILL_FOCUS, edt_iff, MsnElemDlg, OnIFFChange);
}

void MsnElemDlg::Show()
{
  FormWindow::Show();

  if (!elem)
    return;

  int current_class = 0;

  if (cmb_class)
  {
    cmb_class->ClearItems();

    cmb_class->AddItem(Ship::ClassName(Ship::DRONE));
    cmb_class->AddItem(Ship::ClassName(Ship::FIGHTER));
    cmb_class->AddItem(Ship::ClassName(Ship::ATTACK));

    cmb_class->AddItem(Ship::ClassName(Ship::LCA));
    cmb_class->AddItem(Ship::ClassName(Ship::COURIER));
    cmb_class->AddItem(Ship::ClassName(Ship::CARGO));
    cmb_class->AddItem(Ship::ClassName(Ship::CORVETTE));
    cmb_class->AddItem(Ship::ClassName(Ship::FREIGHTER));
    cmb_class->AddItem(Ship::ClassName(Ship::FRIGATE));
    cmb_class->AddItem(Ship::ClassName(Ship::DESTROYER));
    cmb_class->AddItem(Ship::ClassName(Ship::CRUISER));
    cmb_class->AddItem(Ship::ClassName(Ship::BATTLESHIP));
    cmb_class->AddItem(Ship::ClassName(Ship::CARRIER));
    cmb_class->AddItem(Ship::ClassName(Ship::SWACS));
    cmb_class->AddItem(Ship::ClassName(Ship::DREADNAUGHT));
    cmb_class->AddItem(Ship::ClassName(Ship::STATION));
    cmb_class->AddItem(Ship::ClassName(Ship::FARCASTER));

    cmb_class->AddItem(Ship::ClassName(Ship::MINE));
    cmb_class->AddItem(Ship::ClassName(Ship::COMSAT));
    cmb_class->AddItem(Ship::ClassName(Ship::DEFSAT));

    cmb_class->AddItem(Ship::ClassName(Ship::BUILDING));
    cmb_class->AddItem(Ship::ClassName(Ship::FACTORY));
    cmb_class->AddItem(Ship::ClassName(Ship::SAM));
    cmb_class->AddItem(Ship::ClassName(Ship::EWR));
    cmb_class->AddItem(Ship::ClassName(Ship::C3I));
    cmb_class->AddItem(Ship::ClassName(Ship::STARBASE));

    const ShipDesign* design = elem->GetDesign();

    for (int i = 0; i < cmb_class->NumItems(); i++)
    {
      std::string cname = cmb_class->GetItem(i);
      int classid = Ship::ClassForName(cname);

      if (design && classid == design->type)
      {
        cmb_class->SetSelection(i);
        current_class = classid;
        break;
      }
    }
  }

  if (cmb_design)
  {
    OnClassSelect(nullptr);
    OnDesignSelect(nullptr);
  }

  if (cmb_role)
  {
    cmb_role->ClearItems();

    for (int i = Mission::PATROL; i <= Mission::OTHER; i++)
    {
      cmb_role->AddItem(Mission::RoleName(i));

      if (i == 0)
        cmb_role->SetSelection(0);

      else if (elem->MissionRole() == i)
        cmb_role->SetSelection(cmb_role->NumItems() - 1);
    }
  }

  if (cmb_region)
  {
    cmb_region->ClearItems();

    if (mission)
    {
      StarSystem* sys = mission->GetStarSystem();
      if (sys)
      {
        List<OrbitalRegion> regions;
        regions.append(sys->AllRegions());
        regions.sort();

        ListIter<OrbitalRegion> iter = regions;
        while (++iter)
        {
          OrbitalRegion* region = iter.value();
          cmb_region->AddItem(region->Name());

          if (elem->Region() == region->Name())
            cmb_region->SetSelection(cmb_region->NumItems() - 1);
        }
      }
    }
  }

  char buf[64];

  if (edt_name)
    edt_name->SetText(elem->Name());

  sprintf_s(buf, "%d", elem->Count());
  if (edt_size)
    edt_size->SetText(buf);

  sprintf_s(buf, "%d", elem->GetIFF());
  if (edt_iff)
    edt_iff->SetText(buf);

  sprintf_s(buf, "%d", static_cast<int>(elem->Location().x) / 1000);
  if (edt_loc_x)
    edt_loc_x->SetText(buf);

  sprintf_s(buf, "%d", static_cast<int>(elem->Location().y) / 1000);
  if (edt_loc_y)
    edt_loc_y->SetText(buf);

  sprintf_s(buf, "%d", static_cast<int>(elem->Location().z) / 1000);
  if (edt_loc_z)
    edt_loc_z->SetText(buf);

  sprintf_s(buf, "%d", elem->RespawnCount());
  if (edt_respawns)
    edt_respawns->SetText(buf);

  sprintf_s(buf, "%d", elem->HoldTime());
  if (edt_hold_time)
    edt_hold_time->SetText(buf);

  if (btn_player)
    btn_player->SetButtonState(elem->Player() > 0 ? 1 : 0);
  if (btn_playable)
    btn_playable->SetButtonState(elem->IsPlayable() ? 1 : 0);
  if (btn_alert)
    btn_alert->SetButtonState(elem->IsAlert() ? 1 : 0);
  if (btn_command_ai)
    btn_command_ai->SetButtonState(elem->CommandAI());

  UpdateTeamInfo();

  if (cmb_intel)
  {
    cmb_intel->ClearItems();

    for (int i = Intel::RESERVE; i < Intel::TRACKED; i++)
    {
      cmb_intel->AddItem(Intel::NameFromIntel(i));

      if (i == 0)
        cmb_intel->SetSelection(0);

      else if (elem->IntelLevel() == i)
        cmb_intel->SetSelection(cmb_intel->NumItems() - 1);
    }
  }

  Instruction* instr = nullptr;
  if (elem->Objectives().size() > 0)
    instr = elem->Objectives().at(0);

  if (cmb_objective)
  {
    cmb_objective->ClearItems();
    cmb_objective->AddItem("");
    cmb_objective->SetSelection(0);

    for (int i = 0; i < Instruction::NUM_ACTIONS; i++)
    {
      cmb_objective->AddItem(Instruction::ActionName(i));

      if (instr && instr->Action() == i)
        cmb_objective->SetSelection(i + 1);
    }
  }

  if (cmb_target)
    OnObjectiveSelect(nullptr);

  if (cmb_heading)
  {
    double heading = elem->Heading();

    while (heading > 2 * PI)
      heading -= 2 * PI;

    while (heading < 0)
      heading += 2 * PI;

    if (heading >= PI / 4 && heading < 3 * PI / 4)
      cmb_heading->SetSelection(1);

    else if (heading >= 3 * PI / 4 && heading < 5 * PI / 4)
      cmb_heading->SetSelection(2);

    else if (heading >= 5 * PI / 4 && heading < 7 * PI / 4)
      cmb_heading->SetSelection(3);

    else
      cmb_heading->SetSelection(0);
  }

  exit_latch = true;
}

void MsnElemDlg::ExecFrame()
{
  if (Keyboard::KeyDown(VK_RETURN))
  {
    if (!exit_latch && btn_accept && btn_accept->IsEnabled())
      OnAccept(nullptr);
  }

  else if (Keyboard::KeyDown(VK_ESCAPE))
  {
    if (!exit_latch)
      OnCancel(nullptr);
  }

  else
    exit_latch = false;
}

void MsnElemDlg::SetMission(Mission* m) { mission = m; }

void MsnElemDlg::SetMissionElement(MissionElement* e) { elem = e; }

void MsnElemDlg::UpdateTeamInfo()
{
  if (cmb_commander)
  {
    cmb_commander->ClearItems();
    cmb_commander->AddItem("");
    cmb_commander->SetSelection(0);

    if (mission && elem)
    {
      ListIter<MissionElement> iter = mission->GetElements();
      while (++iter)
      {
        MissionElement* e = iter.value();

        if (CanCommand(e, elem))
        {
          cmb_commander->AddItem(e->Name());

          if (elem->Commander() == e->Name())
            cmb_commander->SetSelection(cmb_commander->NumItems() - 1);
        }
      }
    }
  }

  if (cmb_squadron)
  {
    cmb_squadron->ClearItems();
    cmb_squadron->AddItem("");
    cmb_squadron->SetSelection(0);

    if (mission && elem)
    {
      ListIter<MissionElement> iter = mission->GetElements();
      while (++iter)
      {
        MissionElement* e = iter.value();

        if (e->GetIFF() == elem->GetIFF() && e != elem && e->IsSquadron())
        {
          cmb_squadron->AddItem(e->Name());

          if (elem->Squadron() == e->Name())
            cmb_squadron->SetSelection(cmb_squadron->NumItems() - 1);
        }
      }
    }
  }

  if (cmb_carrier)
  {
    cmb_carrier->ClearItems();
    cmb_carrier->AddItem("");
    cmb_carrier->SetSelection(0);

    if (mission && elem)
    {
      ListIter<MissionElement> iter = mission->GetElements();
      while (++iter)
      {
        MissionElement* e = iter.value();

        if (e->GetIFF() == elem->GetIFF() && e != elem && e->GetDesign() && e->GetDesign()->flight_decks.size())
        {
          cmb_carrier->AddItem(e->Name());

          if (elem->Carrier() == e->Name())
            cmb_carrier->SetSelection(cmb_carrier->NumItems() - 1);
        }
      }
    }
  }
}

void MsnElemDlg::OnClassSelect(AWEvent* event)
{
  if (!cmb_class || !cmb_design)
    return;

  std::string cname = cmb_class->GetSelectedItem();
  int classid = Ship::ClassForName(cname);

  cmb_design->ClearItems();

  std::vector<std::string> designs;
  ShipDesign::GetDesignList(classid, designs);

  if (designs.size() > 0)
  {
    const ShipDesign* design = elem->GetDesign();
    bool found = false;

    for (int i = 0; i < designs.size(); i++)
    {
      std::string dsn = designs[i];
      cmb_design->AddItem(dsn);

      if (design && dsn == design->m_name)
      {
        cmb_design->SetSelection(i);
        found = true;
      }
    }

    if (!found)
      cmb_design->SetSelection(0);
  }
  else
  {
    cmb_design->AddItem("");
    cmb_design->SetSelection(0);
  }

  OnDesignSelect(nullptr);
}

void MsnElemDlg::OnDesignSelect(AWEvent* event)
{
  if (!cmb_design)
    return;

  ShipDesign* design = nullptr;

  std::string dname = cmb_design->GetSelectedItem();

  if (!dname.empty())
    design = ShipDesign::Get(dname);

  if (cmb_loadout)
  {
    int load_index = 0;
    cmb_loadout->ClearItems();

    if (design)
    {
      MissionLoad* mload = nullptr;

      if (elem && elem->Loadouts().size() > 0)
        mload = elem->Loadouts().at(0);

      const List<ShipLoad>& loadouts = design->loadouts;

      if (loadouts.size() > 0)
      {
        for (int i = 0; i < loadouts.size(); i++)
        {
          const ShipLoad* load = loadouts[i];
          if (load->name[0])
          {
            cmb_loadout->AddItem(load->name);

            if (mload && mload->GetName() == load->name)
              load_index = cmb_loadout->NumItems() - 1;
          }
        }
      }
    }

    if (cmb_loadout->NumItems() < 1)
      cmb_loadout->AddItem("");

    cmb_loadout->SetSelection(load_index);
  }

  if (cmb_skin)
  {
    cmb_skin->ClearItems();

    if (design)
    {
      cmb_skin->AddItem(Game::GetText("MsnDlg.default"));
      cmb_skin->SetSelection(0);

      ListIter<Skin> iter = design->skins;

      while (++iter)
      {
        Skin* s = iter.value();
        cmb_skin->AddItem(s->Name());

        if (elem && elem->GetSkin() && s->Name() == elem->GetSkin()->Name())
          cmb_skin->SetSelection(cmb_skin->NumItems() - 1);
      }
    }
  }
}

void MsnElemDlg::OnObjectiveSelect(AWEvent* event)
{
  if (!cmb_objective || !cmb_target)
    return;

  Instruction* instr = nullptr;
  if (elem->Objectives().size() > 0)
    instr = elem->Objectives().at(0);

  int objid = cmb_objective->GetSelectedIndex() - 1;

  cmb_target->ClearItems();
  cmb_target->AddItem("");

  if (mission)
  {
    ListIter<MissionElement> iter = mission->GetElements();
    while (++iter)
    {
      MissionElement* e = iter.value();

      if (e != elem)
      {
        bool add = false;

        if (objid < Instruction::PATROL)
          add = e->GetIFF() == 0 || e->GetIFF() == elem->GetIFF();
        else
          add = e->GetIFF() != elem->GetIFF();

        if (add)
        {
          cmb_target->AddItem(e->Name());

          if (instr && instr->TargetName() == e->Name())
            cmb_target->SetSelection(cmb_target->NumItems() - 1);
        }
      }
    }
  }
}

void MsnElemDlg::OnIFFChange(AWEvent* event)
{
  if (edt_iff && elem)
  {
    int elem_iff = 0;
    sscanf_s(edt_iff->GetText().data(), "%d", &elem_iff);

    if (elem->GetIFF() == elem_iff)
      return;

    elem->SetIFF(elem_iff);
  }

  UpdateTeamInfo();

  if (cmb_target)
    OnObjectiveSelect(nullptr);
}

void MsnElemDlg::OnAccept(AWEvent* event)
{
  if (mission && elem)
  {
    char buf[64];
    int val;

    if (edt_name)
      elem->SetName(edt_name->GetText());

    if (edt_size)
    {
      strcpy_s(buf, edt_size->GetText().c_str());

      if (isdigit(*buf))
        sscanf_s(buf, "%d", &val);
      else
        val = 1;

      elem->SetCount(val);
    }

    if (edt_iff)
    {
      strcpy_s(buf, edt_iff->GetText().c_str());

      if (isdigit(*buf))
        sscanf_s(buf, "%d", &val);
      else
        val = 1;

      elem->SetIFF(val);
    }

    if (edt_loc_x && edt_loc_y && edt_loc_z)
    {
      Point loc;

      strcpy_s(buf, edt_loc_x->GetText().c_str());

      if (isdigit(*buf) || *buf == '-')
        sscanf_s(buf, "%d", &val);
      else
        val = 0;

      loc.x = val * 1000;

      strcpy_s(buf, edt_loc_y->GetText().c_str());

      if (isdigit(*buf) || *buf == '-')
        sscanf_s(buf, "%d", &val);
      else
        val = 0;

      loc.y = val * 1000;

      strcpy_s(buf, edt_loc_z->GetText().c_str());

      if (isdigit(*buf) || *buf == '-')
        sscanf_s(buf, "%d", &val);
      else
        val = 0;

      loc.z = val * 1000;

      elem->SetLocation(loc);
    }

    if (edt_respawns)
    {
      strcpy_s(buf, edt_respawns->GetText().c_str());

      if (isdigit(*buf))
        sscanf_s(buf, "%d", &val);
      else
        val = 0;

      elem->SetRespawnCount(val);
    }

    if (edt_hold_time)
    {
      strcpy_s(buf, edt_hold_time->GetText().c_str());

      if (isdigit(*buf))
        sscanf_s(buf, "%d", &val);
      else
        val = 0;

      elem->SetHoldTime(val);
    }

    if (btn_player)
    {
      if (btn_player->GetButtonState() > 0)
        mission->SetPlayer(elem);
      else
        elem->SetPlayer(0);
    }

    if (btn_playable)
      elem->SetPlayable(btn_playable->GetButtonState() ? true : false);

    if (btn_alert)
      elem->SetAlert(btn_alert->GetButtonState() ? true : false);

    if (btn_command_ai)
      elem->SetCommandAI(btn_command_ai->GetButtonState() ? 1 : 0);

    if (cmb_design)
    {
      ShipDesign* d = ShipDesign::Get(cmb_design->GetSelectedItem());

      if (d)
      {
        elem->SetDesign(d);

        if (cmb_skin)
        {
          std::string skin_name = cmb_skin->GetSelectedItem();

          if (skin_name != Game::GetText("MsnDlg.default"))
            elem->SetSkin(d->FindSkin(skin_name));
          else
            elem->SetSkin(nullptr);
        }
      }
    }

    if (cmb_role)
      elem->SetMissionRole(cmb_role->GetSelectedIndex());

    if (cmb_region)
    {
      elem->SetRegion(cmb_region->GetSelectedItem());

      if (elem->Player() > 0)
        mission->SetRegion(cmb_region->GetSelectedItem());
    }

    if (cmb_heading)
    {
      switch (cmb_heading->GetSelectedIndex())
      {
        default: case 0:
          elem->SetHeading(0);
          break;
        case 1:
          elem->SetHeading(PI / 2);
          break;
        case 2:
          elem->SetHeading(PI);
          break;
        case 3:
          elem->SetHeading(3 * PI / 2);
          break;
      }
    }

    if (cmb_commander)
      elem->SetCommander(cmb_commander->GetSelectedItem());

    if (cmb_squadron)
      elem->SetSquadron(cmb_squadron->GetSelectedItem());

    if (cmb_carrier)
      elem->SetCarrier(cmb_carrier->GetSelectedItem());

    if (cmb_intel)
      elem->SetIntelLevel(Intel::IntelFromName(cmb_intel->GetSelectedItem()));

    if (cmb_loadout && cmb_loadout->NumItems())
    {
      elem->Loadouts().destroy();

      std::string loadname = cmb_loadout->GetSelectedItem();
      if (!loadname.empty())
      {
        auto mload = NEW MissionLoad(-1, loadname);
        elem->Loadouts().append(mload);
      }
    }

    if (cmb_objective && cmb_target)
    {
      List<Instruction>& objectives = elem->Objectives();
      objectives.destroy();

      int action = cmb_objective->GetSelectedIndex() - 1;
      std::string target = cmb_target->GetSelectedItem();

      if (action >= Instruction::VECTOR)
      {
        auto obj = NEW Instruction(action, target);
        objectives.append(obj);
      }
    }

    if (elem->Player())
      mission->SetTeam(elem->GetIFF());
  }

  manager->ShowMsnEditDlg();
}

void MsnElemDlg::OnCancel(AWEvent* event) { manager->ShowMsnEditDlg(); }

bool MsnElemDlg::CanCommand(const MissionElement* commander, const MissionElement* subordinate) const
{
  if (mission && commander && subordinate && commander != subordinate)
  {
    if (commander->GetIFF() != subordinate->GetIFF())
      return false;

    if (commander->IsSquadron())
      return false;

    if (commander->Commander().length() == 0)
      return true;

    if (subordinate->Name() == commander->Commander())
      return false;

    MissionElement* elem = mission->FindElement(commander->Commander());

    if (elem)
      return CanCommand(elem, subordinate);
  }

  return false;
}

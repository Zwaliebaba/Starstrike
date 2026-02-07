#include "pch.h"
#include "MsnSelectDlg.h"
#include "Button.h"
#include "Campaign.h"
#include "ComboBox.h"
#include "ConfirmDlg.h"
#include "FormatUtil.h"
#include "Game.h"
#include "Keyboard.h"
#include "ListBox.h"
#include "MenuScreen.h"
#include "Mission.h"
#include "Mouse.h"
#include "MsnEditDlg.h"
#include "MsnEditNavDlg.h"
#include "Starshatter.h"

DEF_MAP_CLIENT(MsnSelectDlg, OnAccept);
DEF_MAP_CLIENT(MsnSelectDlg, OnCancel);
DEF_MAP_CLIENT(MsnSelectDlg, OnMod);
DEF_MAP_CLIENT(MsnSelectDlg, OnNew);
DEF_MAP_CLIENT(MsnSelectDlg, OnEdit);
DEF_MAP_CLIENT(MsnSelectDlg, OnDel);
DEF_MAP_CLIENT(MsnSelectDlg, OnDelConfirm);
DEF_MAP_CLIENT(MsnSelectDlg, OnCampaignSelect);
DEF_MAP_CLIENT(MsnSelectDlg, OnMissionSelect);

static Mission* edit_mission;

MsnSelectDlg::MsnSelectDlg(Screen* s, FormDef& def, MenuScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    btn_mod(nullptr),
    btn_new(nullptr),
    btn_edit(nullptr),
    btn_del(nullptr),
    btn_accept(nullptr),
    btn_cancel(nullptr),
    cmb_campaigns(nullptr),
    lst_campaigns(nullptr),
    lst_missions(nullptr),
    description(nullptr),
    stars(nullptr),
    campaign(nullptr),
    selected_mission(-1),
    mission_id(0),
    editable(false)
{
  stars = Starshatter::GetInstance();
  campaign = Campaign::GetCampaign();
  edit_mission = nullptr;

  Init(def);
}

MsnSelectDlg::~MsnSelectDlg() {}

void MsnSelectDlg::RegisterControls()
{
  btn_accept = static_cast<Button*>(FindControl(1));
  btn_cancel = static_cast<Button*>(FindControl(2));

  if (btn_accept)
  {
    btn_accept->SetEnabled(false);
    REGISTER_CLIENT(EID_CLICK, btn_accept, MsnSelectDlg, OnAccept);
  }

  if (btn_cancel) { REGISTER_CLIENT(EID_CLICK, btn_cancel, MsnSelectDlg, OnCancel); }

  btn_mod = static_cast<Button*>(FindControl(300));
  btn_new = static_cast<Button*>(FindControl(301));
  btn_edit = static_cast<Button*>(FindControl(302));
  btn_del = static_cast<Button*>(FindControl(303));

  if (btn_mod)
    REGISTER_CLIENT(EID_CLICK, btn_mod, MsnSelectDlg, OnMod);

  if (btn_new)
    REGISTER_CLIENT(EID_CLICK, btn_new, MsnSelectDlg, OnNew);

  if (btn_edit)
    REGISTER_CLIENT(EID_CLICK, btn_edit, MsnSelectDlg, OnEdit);

  if (btn_del)
  {
    REGISTER_CLIENT(EID_CLICK, btn_del, MsnSelectDlg, OnDel);
    REGISTER_CLIENT(EID_USER_1, btn_del, MsnSelectDlg, OnDelConfirm);
  }

  description = FindControl(200);

  cmb_campaigns = static_cast<ComboBox*>(FindControl(201));
  lst_campaigns = static_cast<ListBox*>(FindControl(203));
  lst_missions = static_cast<ListBox*>(FindControl(202));

  if (cmb_campaigns) { REGISTER_CLIENT(EID_SELECT, cmb_campaigns, MsnSelectDlg, OnCampaignSelect); }

  if (lst_campaigns)
  {
    REGISTER_CLIENT(EID_SELECT, lst_campaigns, MsnSelectDlg, OnCampaignSelect);

    lst_campaigns->SetSelectedStyle(ListBox::LIST_ITEM_STYLE_FILLED_BOX);
    lst_campaigns->SetLeading(4);
  }

  if (lst_missions)
  {
    REGISTER_CLIENT(EID_SELECT, lst_missions, MsnSelectDlg, OnMissionSelect);

    lst_missions->SetSelectedStyle(ListBox::LIST_ITEM_STYLE_FILLED_BOX);
    lst_missions->SetLeading(4);
  }
}

void MsnSelectDlg::Show()
{
  FormWindow::Show();
  campaign = Campaign::GetCampaign();

  if (cmb_campaigns)
  {
    int n = 0;
    cmb_campaigns->ClearItems();
    ListIter<Campaign> iter = Campaign::GetAllCampaigns();
    while (++iter)
    {
      Campaign* c = iter.value();

      if (c->GetCampaignId() >= Campaign::SINGLE_MISSIONS)
      {
        cmb_campaigns->AddItem(c->Name());

        if (campaign->GetCampaignId() < Campaign::SINGLE_MISSIONS)
        {
          campaign = Campaign::SelectCampaign(c->Name());
          cmb_campaigns->SetSelection(n);
        }

        else if (campaign->GetCampaignId() == c->GetCampaignId())
          cmb_campaigns->SetSelection(n);

        n++;
      }
    }
  }

  else if (lst_campaigns)
  {
    int n = 0;
    lst_campaigns->ClearItems();
    ListIter<Campaign> iter = Campaign::GetAllCampaigns();
    while (++iter)
    {
      Campaign* c = iter.value();

      if (c->GetCampaignId() >= Campaign::SINGLE_MISSIONS)
      {
        lst_campaigns->AddItem(c->Name());

        if (campaign->GetCampaignId() < Campaign::SINGLE_MISSIONS)
        {
          campaign = Campaign::SelectCampaign(c->Name());
          lst_campaigns->SetSelected(n);
        }

        else if (campaign->GetCampaignId() == c->GetCampaignId())
          lst_campaigns->SetSelected(n);

        n++;
      }
    }
  }

  if (campaign)
  {
    int id = campaign->GetCampaignId();
    editable = false;

    if (btn_new)
      btn_new->SetEnabled(editable);
    if (btn_edit)
      btn_edit->SetEnabled(false);
    if (btn_del)
      btn_del->SetEnabled(false);
  }

  if (description)
    description->SetText(Game::GetText("MsnSelectDlg.choose"));

  if (lst_missions)
  {
    lst_missions->ClearItems();

    if (campaign)
    {
      ListIter<MissionInfo> iter = campaign->GetMissionList();
      while (++iter)
      {
        MissionInfo* info = iter.value();
        Mission* m = info->mission;

        lst_missions->AddItem(info->m_name);

        if (m && m == edit_mission)
          lst_missions->SetSelected(lst_missions->NumItems() - 1);
      }

      if (selected_mission >= 0 && lst_missions->GetSelCount() == 0)
        lst_missions->SetSelected(selected_mission);
    }

    OnMissionSelect(nullptr);
    edit_mission = nullptr;
  }
}

void MsnSelectDlg::OnCampaignSelect(AWEvent* event)
{
  std::string selected_campaign;

  if (cmb_campaigns)
    selected_campaign = cmb_campaigns->GetSelectedItem();
  else if (lst_campaigns)
    selected_campaign = lst_campaigns->GetSelectedItem();

  Campaign* c = Campaign::SelectCampaign(selected_campaign);

  if (c)
  {
    campaign = c;

    if (cmb_campaigns)
    {
      cmb_campaigns->ClearItems();

      ListIter<MissionInfo> iter = campaign->GetMissionList();
      while (++iter)
        cmb_campaigns->AddItem(iter->m_name);
    }

    else if (lst_missions)
    {
      lst_missions->ClearItems();

      ListIter<MissionInfo> iter = campaign->GetMissionList();
      while (++iter)
        lst_missions->AddItem(iter->m_name);

      lst_missions->ScrollTo(0);
    }

    if (btn_accept)
      btn_accept->SetEnabled(false);

    if (description)
      description->SetText(Game::GetText("MsnSelectDlg.choose"));

    int id = c->GetCampaignId();
    editable = false;

    if (btn_new)
      btn_new->SetEnabled(editable);
    if (btn_edit)
      btn_edit->SetEnabled(false);
    if (btn_del)
      btn_del->SetEnabled(false);
  }
}

void MsnSelectDlg::OnMissionSelect(AWEvent* event)
{
  selected_mission = -1;

  for (int i = 0; i < lst_missions->NumItems(); i++)
  {
    if (lst_missions->IsSelected(i))
      selected_mission = i;
  }

  if (btn_accept && description && campaign)
  {
    List<MissionInfo>& mission_info_list = campaign->GetMissionList();

    if (selected_mission >= 0 && selected_mission < mission_info_list.size())
    {
      MissionInfo* info = mission_info_list[selected_mission];
      mission_id = info->m_id;

      char time_buf[32];
      FormatDayTime(time_buf, info->start);

      std::string d("<font Limerick12><color ffffff>");
      d += info->m_name;
      d += "<font Verdana>\n\n<color ffff80>";
      d += Game::GetText("MsnSelectDlg.mission-type");
      d += "<color ffffff>\n\t";
      d += Mission::RoleName(info->type);
      d += "\n\n<color ffff80>";
      d += Game::GetText("MsnSelectDlg.scenario");
      d += "<color ffffff>\n\t";
      d += info->description;
      d += "\n\n<color ffff80>";
      d += Game::GetText("MsnSelectDlg.location");
      d += "<color ffffff>\n\t";
      d += info->region;
      d += " ";
      d += Game::GetText("MsnSelectDlg.sector");
      d += " / ";
      d += info->system;
      d += " ";
      d += Game::GetText("MsnSelectDlg.system");
      d += "\n\n<color ffff80>";
      d += Game::GetText("MsnSelectDlg.start-time");
      d += "<color ffffff>\n\t";
      d += time_buf;

      description->SetText(d);
      btn_accept->SetEnabled(true);

      if (btn_edit)
        btn_edit->SetEnabled(editable);
      if (btn_del)
        btn_del->SetEnabled(editable);
    }

    else
    {
      description->SetText(Game::GetText("MsnSelectDlg.choose"));
      btn_accept->SetEnabled(false);

      if (btn_edit)
        btn_edit->SetEnabled(false);
      if (btn_del)
        btn_del->SetEnabled(false);
    }
  }
}

void MsnSelectDlg::ExecFrame()
{
  if (Keyboard::KeyDown(VK_RETURN))
  {
    if (btn_accept && btn_accept->IsEnabled())
      OnAccept(nullptr);
  }
}

void MsnSelectDlg::OnMod(AWEvent* event) {}

void MsnSelectDlg::OnNew(AWEvent* event)
{
  std::string cname;

  if (cmb_campaigns)
    cname = cmb_campaigns->GetSelectedItem();
  else if (lst_campaigns)
    cname = lst_campaigns->GetSelectedItem();

  Campaign* c = Campaign::SelectCampaign(cname);
  if (!c)
    return;

  MissionInfo* info = c->CreateNewMission();
  if (!info || !info->mission)
    return;

  mission_id = info->m_id;

  MsnEditDlg* editor = manager->GetMsnEditDlg();

  if (editor)
  {
    edit_mission = info->mission;

    editor->SetMissionInfo(info);
    editor->SetMission(info->mission);
    manager->ShowMsnEditDlg();
  }

  auto navdlg = static_cast<MsnEditNavDlg*>(manager->GetNavDlg());

  if (navdlg)
  {
    navdlg->SetMission(info->mission);
    navdlg->SetMissionInfo(info);
  }
}

void MsnSelectDlg::OnEdit(AWEvent* event)
{
  std::string cname;

  if (cmb_campaigns)
    cname = cmb_campaigns->GetSelectedItem();
  else if (lst_campaigns)
    cname = lst_campaigns->GetSelectedItem();

  Campaign* c = Campaign::SelectCampaign(cname);
  if (!c)
    return;

  Mission* m = c->GetMission(mission_id);
  if (!m)
    return;

  MsnEditDlg* editor = manager->GetMsnEditDlg();

  if (editor)
  {
    edit_mission = m;

    editor->SetMissionInfo(c->GetMissionInfo(mission_id));
    editor->SetMission(m);
    manager->ShowMsnEditDlg();
  }
}

void MsnSelectDlg::OnDel(AWEvent* event)
{
  std::string cname;

  if (cmb_campaigns)
    cname = cmb_campaigns->GetSelectedItem();
  else if (lst_campaigns)
    cname = lst_campaigns->GetSelectedItem();

  Campaign* c = Campaign::SelectCampaign(cname);
  if (!c)
    return;

  Mission* m = c->GetMission(mission_id);
  if (!m)
    return;

  ConfirmDlg* confirm = manager->GetConfirmDlg();
  if (confirm)
  {
    confirm->SetMessage(Game::GetText("MsnSelectDlg.are-you-sure", m->Name()));
    confirm->SetTitle(Game::GetText("MsnSelectDlg.confirm-delete"));
    confirm->SetParentControl(btn_del);

    manager->ShowConfirmDlg();
  }

  else
    OnDelConfirm(event);
}

void MsnSelectDlg::OnDelConfirm(AWEvent* event)
{
  std::string cname;

  if (cmb_campaigns)
    cname = cmb_campaigns->GetSelectedItem();
  else if (lst_campaigns)
    cname = lst_campaigns->GetSelectedItem();

  Campaign* c = Campaign::SelectCampaign(cname);
  if (!c)
    return;

  edit_mission = nullptr;
  c->DeleteMission(mission_id);
  Show();
}

void MsnSelectDlg::OnAccept(AWEvent* event)
{
  if (selected_mission >= 0)
  {
    Mouse::Show(false);

    int id = campaign->GetMissionList()[selected_mission]->m_id;
    campaign->SetMissionId(id);
    campaign->ReloadMission(id);

    stars->SetGameMode(Starshatter::PREP_MODE);
  }
}

void MsnSelectDlg::OnCancel(AWEvent* event) { manager->ShowMenuDlg(); }

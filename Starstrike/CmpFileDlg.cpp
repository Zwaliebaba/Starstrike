#include "pch.h"
#include "CmpFileDlg.h"
#include "Button.h"
#include "Campaign.h"
#include "CampaignSaveGame.h"
#include "CmpnScreen.h"
#include "CombatGroup.h"
#include "EditBox.h"
#include "FormatUtil.h"
#include "Game.h"
#include "Keyboard.h"
#include "ListBox.h"

DEF_MAP_CLIENT(CmpFileDlg, OnSave);
DEF_MAP_CLIENT(CmpFileDlg, OnCancel);
DEF_MAP_CLIENT(CmpFileDlg, OnCampaign);

CmpFileDlg::CmpFileDlg(Screen* s, FormDef& def, CmpnScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    btn_save(nullptr),
    btn_cancel(nullptr),
    edt_name(nullptr),
    lst_campaigns(nullptr),
    exit_latch(false) { Init(def); }

CmpFileDlg::~CmpFileDlg() {}

void CmpFileDlg::RegisterControls()
{
  btn_save = static_cast<Button*>(FindControl(1));
  btn_cancel = static_cast<Button*>(FindControl(2));

  if (btn_save)
    REGISTER_CLIENT(EID_CLICK, btn_save, CmpFileDlg, OnSave);

  if (btn_cancel)
    REGISTER_CLIENT(EID_CLICK, btn_cancel, CmpFileDlg, OnCancel);

  edt_name = static_cast<EditBox*>(FindControl(200));

  if (edt_name)
    edt_name->SetText("");

  lst_campaigns = static_cast<ListBox*>(FindControl(201));

  if (lst_campaigns)
    REGISTER_CLIENT(EID_SELECT, lst_campaigns, CmpFileDlg, OnCampaign);
}

void CmpFileDlg::Show()
{
  FormWindow::Show();

  if (lst_campaigns)
  {
    lst_campaigns->ClearItems();
    lst_campaigns->SetLineHeight(12);

    std::vector<std::string> save_list;

    CampaignSaveGame::GetSaveGameList(save_list);
    std::ranges::sort(save_list);

    for (int i = 0; i < save_list.size(); i++)
      lst_campaigns->AddItem(save_list[i]);
  }

  if (edt_name)
  {
    char save_name[256];
    save_name[0] = 0;

    campaign = Campaign::GetCampaign();
    if (campaign && campaign->GetPlayerGroup())
    {
      auto op_name = campaign->Name();
      char day[32];
      CombatGroup* group = campaign->GetPlayerGroup();

      if (op_name.starts_with("Operation "))
        op_name = op_name.substr(10);

      FormatDay(day, campaign->GetTime());

      sprintf_s(save_name, "%s %s (%s)", op_name.c_str(), day, group->GetRegion().data());
    }

    edt_name->SetText(save_name);
    edt_name->SetFocus();
  }
}

void CmpFileDlg::ExecFrame()
{
  if (Keyboard::KeyDown(VK_RETURN))
    OnSave(nullptr);

  if (Keyboard::KeyDown(VK_ESCAPE))
  {
    if (!exit_latch)
      OnCancel(nullptr);

    exit_latch = true;
  }
  else
    exit_latch = false;
}

void CmpFileDlg::OnSave(AWEvent* event)
{
  if (edt_name && edt_name->GetText().length() > 0)
  {
    campaign = Campaign::GetCampaign();
    CampaignSaveGame save(campaign);

    char filename[256];
    strcpy_s(filename, edt_name->GetText().c_str());
    char* newline = strchr(filename, '\n');
    if (newline)
      *newline = 0;

    save.Save(filename);
    save.SaveAuto();

    if (manager)
      manager->HideCmpFileDlg();
  }
}

void CmpFileDlg::OnCancel(AWEvent* event)
{
  if (manager)
    manager->HideCmpFileDlg();
}

void CmpFileDlg::OnCampaign(AWEvent* event)
{
  int n = lst_campaigns->GetSelection();

  if (n >= 0)
  {
    std::string cmpn = lst_campaigns->GetItemText(n);

    if (edt_name)
      edt_name->SetText(cmpn);
  }
}

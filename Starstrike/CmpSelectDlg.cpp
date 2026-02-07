#include "pch.h"
#include "CmpSelectDlg.h"
#include "Button.h"
#include "Campaign.h"
#include "CampaignSaveGame.h"
#include "CombatGroup.h"
#include "ConfirmDlg.h"
#include "FormatUtil.h"
#include "Game.h"
#include "Keyboard.h"
#include "ListBox.h"
#include "MenuScreen.h"
#include "Mouse.h"
#include "Player.h"
#include "Starshatter.h"

DEF_MAP_CLIENT(CmpSelectDlg, OnNew);
DEF_MAP_CLIENT(CmpSelectDlg, OnSaved);
DEF_MAP_CLIENT(CmpSelectDlg, OnDelete);
DEF_MAP_CLIENT(CmpSelectDlg, OnConfirmDelete);
DEF_MAP_CLIENT(CmpSelectDlg, OnAccept);
DEF_MAP_CLIENT(CmpSelectDlg, OnCancel);
DEF_MAP_CLIENT(CmpSelectDlg, OnCampaignSelect);

CmpSelectDlg::CmpSelectDlg(Screen* s, FormDef& def, MenuScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    btn_new(nullptr),
    btn_saved(nullptr),
    btn_delete(nullptr),
    btn_accept(nullptr),
    btn_cancel(nullptr),
    lst_campaigns(nullptr),
    description(nullptr),
    stars(nullptr),
    campaign(nullptr),
    selected_mission(-1),
    hproc(nullptr),
    loading(false),
    loaded(false),
    show_saved(false)
{
  stars = Starshatter::GetInstance();
  select_msg = Game::GetText("CmpSelectDlg.select_msg");
  FormWindow::Init(def);
}

CmpSelectDlg::~CmpSelectDlg()
{
  CmpSelectDlg::StopLoadProc();
  images.destroy();
}

void CmpSelectDlg::RegisterControls()
{
  btn_new = dynamic_cast<Button*>(FindControl(100));
  btn_saved = dynamic_cast<Button*>(FindControl(101));
  btn_delete = dynamic_cast<Button*>(FindControl(102));
  btn_accept = dynamic_cast<Button*>(FindControl(1));
  btn_cancel = dynamic_cast<Button*>(FindControl(2));

  if (btn_new)
    REGISTER_CLIENT(EID_CLICK, btn_new, CmpSelectDlg, OnNew);

  if (btn_saved)
    REGISTER_CLIENT(EID_CLICK, btn_saved, CmpSelectDlg, OnSaved);

  if (btn_delete)
  {
    btn_delete->SetEnabled(false);
    REGISTER_CLIENT(EID_CLICK, btn_delete, CmpSelectDlg, OnDelete);
    REGISTER_CLIENT(EID_USER_1, btn_delete, CmpSelectDlg, OnConfirmDelete);
  }

  if (btn_accept)
  {
    btn_accept->SetEnabled(false);
    REGISTER_CLIENT(EID_CLICK, btn_accept, CmpSelectDlg, OnAccept);
  }

  if (btn_cancel)
  {
    btn_cancel->SetEnabled(true);
    REGISTER_CLIENT(EID_CLICK, btn_cancel, CmpSelectDlg, OnCancel);
  }

  description = FindControl(200);

  lst_campaigns = dynamic_cast<ListBox*>(FindControl(201));

  if (lst_campaigns)
    REGISTER_CLIENT(EID_SELECT, lst_campaigns, CmpSelectDlg, OnCampaignSelect);

  ShowNewCampaigns();
}

void CmpSelectDlg::ExecFrame()
{
  if (Keyboard::KeyDown(VK_RETURN))
  {
    if (btn_accept && btn_accept->IsEnabled())
      OnAccept(nullptr);
  }

  std::unique_lock a(m_mutex);

  if (loaded)
  {
    loaded = false;

    if (btn_cancel)
      btn_cancel->SetEnabled(true);

    if (description && btn_accept)
    {
      if (campaign)
      {
        Campaign::SelectCampaign(campaign->Name());

        if (load_index >= 0)
        {
          if (lst_campaigns)
          {
            images[load_index]->CopyBitmap(*campaign->GetImage(1));
            lst_campaigns->SetItemImage(load_index, images[load_index]);
          }

          description->SetText(
            std::string("<font Limerick12><color ffffff>") + campaign->Name() + std::string("<font Verdana>\n\n") +
            std::string("<color ffff80>") + Game::GetText("CmpSelectDlg.scenario") + std::string("<color ffffff>\n\t") + campaign->
            Description());
        }
        else
        {
          char time_buf[32];
          char score_buf[32];

          double t = campaign->GetLoadTime() - campaign->GetStartTime();
          FormatDayTime(time_buf, t);

          sprintf_s(score_buf, "%d", campaign->GetPlayerTeamScore());

          std::string desc = std::string("<font Limerick12><color ffffff>") + campaign->Name() + std::string("<font Verdana>\n\n") +
            std::string("<color ffff80>") + Game::GetText("CmpSelectDlg.scenario") + std::string("<color ffffff>\n\t") + campaign->
            Description() + std::string("\n\n<color ffff80>") + Game::GetText("CmpSelectDlg.campaign-time") +
            std::string("<color ffffff>\n\t") + time_buf + std::string("\n\n<color ffff80>") + Game::GetText(
              "CmpSelectDlg.assignment") + std::string("<color ffffff>\n\t");

          if (campaign->GetPlayerGroup())
            desc += campaign->GetPlayerGroup()->GetDescription();
          else
            desc += "n/a";

          desc += std::string("\n\n<color ffff80>") + Game::GetText("CmpSelectDlg.team-score") + std::string("<color ffffff>\n\t") +
            score_buf;

          description->SetText(desc);
        }

        btn_accept->SetEnabled(true);

        if (btn_delete)
          btn_delete->SetEnabled(show_saved);
      }
      else
      {
        description->SetText(select_msg);
        btn_accept->SetEnabled(true);
      }
    }
  }
}

bool CmpSelectDlg::CanClose()
{
  std::unique_lock a(m_mutex);
  return !loading;
}

void CmpSelectDlg::ShowNewCampaigns()
{
  std::unique_lock a(m_mutex);

  if (loading && description)
  {
    description->SetText(Game::GetText("CmpSelectDlg.already-loading"));
    Button::PlaySound(Button::SND_REJECT);
    return;
  }

  if (btn_new)
    btn_new->SetButtonState(1);

  if (btn_saved)
    btn_saved->SetButtonState(0);

  if (btn_delete)
    btn_delete->SetEnabled(false);

  if (lst_campaigns)
  {
    images.destroy();

    lst_campaigns->SetSelectedStyle(ListBox::LIST_ITEM_STYLE_PLAIN);
    lst_campaigns->SetLeading(0);
    lst_campaigns->ClearItems();
    lst_campaigns->SetLineHeight(100);

    Player* player = Player::GetCurrentPlayer();
    if (!player)
      return;

    ListIter<Campaign> iter = Campaign::GetAllCampaigns();
    while (++iter)
    {
      Campaign* c = iter.value();

      if (c->GetCampaignId() < Campaign::SINGLE_MISSIONS)
      {
        auto bmp = NEW Bitmap;
        bmp->CopyBitmap(*c->GetImage(0));
        images.append(bmp);

        int n = lst_campaigns->AddImage(bmp) - 1;
        lst_campaigns->SetItemText(n, c->Name());

        // if campaign is not available, show the grayed-out image

        // FULL GAME CRITERIA (based on player record):
        if (c->GetCampaignId() > 2 && c->GetCampaignId() < 10 && !player->HasCompletedCampaign(c->GetCampaignId() - 1))
        {
          images[n]->CopyBitmap(*c->GetImage(2));
          lst_campaigns->SetItemImage(n, images[n]);
        }

        // Two additional sequences of ten campaigns (10-19 and 20-29)
        // for mod authors to use:
        else if (c->GetCampaignId() >= 10 && c->GetCampaignId() < 30 && (c->GetCampaignId() % 10) != 0 && !player->
          HasCompletedCampaign(c->GetCampaignId() - 1))
        {
          images[n]->CopyBitmap(*c->GetImage(2));
          lst_campaigns->SetItemImage(n, images[n]);
        }

        // NOTE: Campaigns 10, 20, and 30-99 are always enabled if they exist!
      }
    }
  }

  if (description)
    description->SetText(select_msg);

  if (btn_accept)
    btn_accept->SetEnabled(false);

  show_saved = false;
}

void CmpSelectDlg::ShowSavedCampaigns()
{
  std::unique_lock a(m_mutex);

  if (loading && description)
  {
    description->SetText(Game::GetText("CmpSelectDlg.already-loading"));
    Button::PlaySound(Button::SND_REJECT);
    return;
  }

  if (btn_new)
    btn_new->SetButtonState(0);

  if (btn_saved)
    btn_saved->SetButtonState(1);

  if (btn_delete)
    btn_delete->SetEnabled(false);

  if (lst_campaigns)
  {
    lst_campaigns->SetSelectedStyle(ListBox::LIST_ITEM_STYLE_FILLED_BOX);
    lst_campaigns->SetLeading(4);
    lst_campaigns->ClearItems();
    lst_campaigns->SetLineHeight(12);

    std::vector<std::string> save_list;

    CampaignSaveGame::GetSaveGameList(save_list);
    std::ranges::sort(save_list);

    for (const auto& save : save_list)
      lst_campaigns->AddItem(save);
  }

  if (description)
    description->SetText(select_msg);

  if (btn_accept)
    btn_accept->SetEnabled(false);

  show_saved = true;
}

void CmpSelectDlg::OnCampaignSelect(AWEvent* event)
{
  if (description && lst_campaigns)
  {
    std::unique_lock a(m_mutex);

    if (loading)
    {
      description->SetText(Game::GetText("CmpSelectDlg.already-loading"));
      Button::PlaySound(Button::SND_REJECT);
      return;
    }

    load_index = -1;
    load_file = "";

    Player* player = Player::GetCurrentPlayer();
    if (!player)
      return;

    // NEW CAMPAIGN:
    if (btn_new && btn_new->GetButtonState())
    {
      List<Campaign>& list = Campaign::GetAllCampaigns();

      for (int i = 0; i < lst_campaigns->NumItems(); i++)
      {
        Campaign* c = list[i];

        // is campaign available?
        // FULL GAME CRITERIA (based on player record):
        if (c->GetCampaignId() <= 2 || player->HasCompletedCampaign(c->GetCampaignId() - 1))
        {
          if (lst_campaigns->IsSelected(i))
          {
            images[i]->CopyBitmap(*c->GetImage(1));
            lst_campaigns->SetItemImage(i, images[i]);

            load_index = i;
          }
          else
          {
            images[i]->CopyBitmap(*c->GetImage(0));
            lst_campaigns->SetItemImage(i, images[i]);
          }
        }

        // if not, then don't select
        else
        {
          images[i]->CopyBitmap(*c->GetImage(2));
          lst_campaigns->SetItemImage(i, images[i]);

          if (lst_campaigns->IsSelected(i))
            description->SetText(select_msg);
        }
      }
    }

    // SAVED CAMPAIGN:
    else
    {
      int seln = lst_campaigns->GetSelection();

      if (seln < 0)
        description->SetText(select_msg);

      else
      {
        load_index = -1;
        load_file = lst_campaigns->GetItemText(seln);
      }
    }

    if (btn_accept)
      btn_accept->SetEnabled(false);
  }

  if (!loading && (load_index >= 0 || !load_file.empty()))
  {
    if (btn_cancel)
      btn_cancel->SetEnabled(false);

    StartLoadProc();
  }
}

void CmpSelectDlg::Show()
{
  FormWindow::Show();
  ShowNewCampaigns();
}

void CmpSelectDlg::OnNew(AWEvent* event) { ShowNewCampaigns(); }

void CmpSelectDlg::OnSaved(AWEvent* event) { ShowSavedCampaigns(); }

void CmpSelectDlg::OnDelete(AWEvent* event)
{
  load_file = "";

  if (lst_campaigns)
  {
    int seln = lst_campaigns->GetSelection();

    if (seln < 0)
    {
      description->SetText(select_msg);
      btn_accept->SetEnabled(false);
    }

    else
    {
      load_index = -1;
      load_file = lst_campaigns->GetItemText(seln);
    }
  }

  if (load_file.length())
  {
    ConfirmDlg* confirm = manager->GetConfirmDlg();
    if (confirm)
    {
      char msg[256];
      sprintf_s(msg, Game::GetText("CmpSelectDlg.are-you-sure").c_str(), load_file.data());
      confirm->SetMessage(msg);
      confirm->SetTitle(Game::GetText("CmpSelectDlg.confirm"));

      manager->ShowConfirmDlg();
    }

    else
      OnConfirmDelete(event);
  }

  ShowSavedCampaigns();
}

void CmpSelectDlg::OnConfirmDelete(AWEvent* event)
{
  if (!load_file.empty())
    CampaignSaveGame::Delete(load_file);

  ShowSavedCampaigns();
}

void CmpSelectDlg::OnAccept(AWEvent* event)
{
  std::unique_lock a(m_mutex);

  if (loading)
    return;

  // if this is to be a new campaign,
  // re-instaniate the campaign object
  if (btn_new->GetButtonState())
    Campaign::GetCampaign()->Load();
  else
    Game::ResetGameTime();

  Mouse::Show(false);
  stars->SetGameMode(Starshatter::CLOD_MODE);
}

void CmpSelectDlg::OnCancel(AWEvent* event) { manager->ShowMenuDlg(); }

DWORD WINAPI CmpSelectDlgLoadProc(LPVOID link);

void CmpSelectDlg::StartLoadProc()
{
  if (hproc != nullptr)
  {
    DWORD result = 0;
    GetExitCodeThread(hproc, &result);

    if (result != STILL_ACTIVE)
    {
      CloseHandle(hproc);
      hproc = nullptr;
    }
    else
      return;
  }

  if (hproc == nullptr)
  {
    campaign = nullptr;
    loading = true;
    loaded = false;

    if (description)
      description->SetText(Game::GetText("CmpSelectDlg.loading"));

    DWORD thread_id = 0;
    hproc = CreateThread(nullptr, 4096, CmpSelectDlgLoadProc, this, 0, &thread_id);

    if (hproc == nullptr)
    {
      static int report = 10;
      if (report > 0)
      {
        ::DebugTrace("WARNING: CmpSelectDlg() failed to create thread (err=%08x)\n", GetLastError());
        report--;

        if (report == 0)
          ::DebugTrace("         Further warnings of this type will be supressed.\n");
      }
    }
  }
}

void CmpSelectDlg::StopLoadProc()
{
  if (hproc != nullptr)
  {
    WaitForSingleObject(hproc, 2500);
    CloseHandle(hproc);
    hproc = nullptr;
  }
}

DWORD WINAPI CmpSelectDlgLoadProc(LPVOID link)
{
  auto dlg = static_cast<CmpSelectDlg*>(link);

  if (dlg)
    return dlg->LoadProc();

  return static_cast<DWORD>(E_POINTER);
}

DWORD CmpSelectDlg::LoadProc()
{
  Campaign* c = nullptr;

  // NEW CAMPAIGN:
  if (load_index >= 0)
  {
    List<Campaign>& list = Campaign::GetAllCampaigns();

    if (load_index < list.size())
    {
      c = list[load_index];
      c->Load();
    }
  }

  // SAVED CAMPAIGN:
  else
  {
    CampaignSaveGame savegame;
    savegame.Load(load_file);
    c = savegame.GetCampaign();
  }

  m_mutex.lock();

  loading = false;
  loaded = true;
  campaign = c;

  m_mutex.unlock();

  return 0;
}

#include "pch.h"
#include "CmdIntelDlg.h"
#include "Button.h"
#include "CameraDirector.h"
#include "Campaign.h"
#include "CombatEvent.h"
#include "FormatUtil.h"
#include "Game.h"
#include "ListBox.h"
#include "Sim.h"
#include "Starshatter.h"

DEF_MAP_CLIENT(CmdIntelDlg, OnMode);
DEF_MAP_CLIENT(CmdIntelDlg, OnSave);
DEF_MAP_CLIENT(CmdIntelDlg, OnExit);
DEF_MAP_CLIENT(CmdIntelDlg, OnNews);
DEF_MAP_CLIENT(CmdIntelDlg, OnPlay);

CmdIntelDlg::CmdIntelDlg(Screen* s, FormDef& def, CmpnScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    CmdDlg(mgr),
    manager(mgr),
    cam_view(nullptr),
    dsp_view(nullptr),
    stars(nullptr),
    campaign(nullptr),
    update_time(0),
    start_scene(0)
{
  stars = Starshatter::GetInstance();
  campaign = Campaign::GetCampaign();

  if (campaign)
    update_time = campaign->GetUpdateTime();

  FormWindow::Init(def);
}

CmdIntelDlg::~CmdIntelDlg() {}

void CmdIntelDlg::RegisterControls()
{
  lst_news = static_cast<ListBox*>(FindControl(401));
  txt_news = static_cast<RichTextBox*>(FindControl(402));
  img_news = static_cast<ImageBox*>(FindControl(403));
  mov_news = FindControl(404);
  btn_play = static_cast<Button*>(FindControl(405));

  RegisterCmdControls(this);

  if (btn_save)
    REGISTER_CLIENT(EID_CLICK, btn_save, CmdIntelDlg, OnSave);

  if (btn_exit)
    REGISTER_CLIENT(EID_CLICK, btn_exit, CmdIntelDlg, OnExit);

  if (btn_play)
    REGISTER_CLIENT(EID_CLICK, btn_play, CmdIntelDlg, OnPlay);

  for (int i = 0; i < 5; i++)
  {
    if (btn_mode[i])
      REGISTER_CLIENT(EID_CLICK, btn_mode[i], CmdIntelDlg, OnMode);
  }

  if (lst_news) { REGISTER_CLIENT(EID_SELECT, lst_news, CmdIntelDlg, OnNews); }

  if (img_news)
    img_news->GetPicture(bmp_default);

  if (mov_news)
  {
    CameraDirector* cam_dir = CameraDirector::GetInstance();
    cam_view = NEW CameraView(mov_news, cam_dir->GetCamera(), nullptr);
    if (cam_view)
      mov_news->AddView(cam_view);

    dsp_view = DisplayView::GetInstance();
    if (dsp_view)
    {
      dsp_view->SetWindow(mov_news);
      mov_news->AddView(dsp_view);
    }

    mov_news->Hide();
  }
}

void CmdIntelDlg::Show()
{
  mode = MODE_INTEL;

  FormWindow::Show();
  ShowCmdDlg();

  if (btn_play)
    btn_play->Hide();

  if (mov_news)
    mov_news->Hide();
}

void CmdIntelDlg::ExecFrame()
{
  CmdDlg::ExecFrame();

  if (campaign != Campaign::GetCampaign() || campaign->GetUpdateTime() != update_time)
  {
    campaign = Campaign::GetCampaign();
    update_time = campaign->GetUpdateTime();

    lst_news->ClearItems();
    txt_news->SetText("");

    if (img_news)
      img_news->SetPicture(bmp_default);
  }

  if (campaign)
  {
    List<CombatEvent>& events = campaign->GetEvents();
    bool auto_scroll = false;

    if (events.size() > lst_news->NumItems())
    {
      while (events.size() > lst_news->NumItems())
      {
        CombatEvent* info = events[lst_news->NumItems()];

        const char* unread = info->Visited() ? " " : "*";
        int i = lst_news->AddItemWithData(unread, (intptr_t)info) - 1;

        char dateline[32];
        FormatDayTime(dateline, info->Time());
        lst_news->SetItemText(i, 1, dateline);
        lst_news->SetItemText(i, 2, info->Title());
        lst_news->SetItemText(i, 3, info->Region());
        lst_news->SetItemText(i, 4, Game::GetText(info->SourceName()).c_str());

        if (!info->Visited())
          auto_scroll = true;
      }

      if (lst_news->GetSortColumn() > 0)
        lst_news->SortItems();
    }

    else if (events.size() < lst_news->NumItems())
    {
      lst_news->ClearItems();

      for (int i = 0; i < events.size(); i++)
      {
        CombatEvent* info = events[i];

        const char* unread = info->Visited() ? " " : "*";
        int j = lst_news->AddItemWithData(unread, (intptr_t)info) - 1;

        char dateline[32];
        FormatDayTime(dateline, info->Time());
        lst_news->SetItemText(j, 1, dateline);
        lst_news->SetItemText(j, 2, info->Title());
        lst_news->SetItemText(j, 3, info->Region());
        lst_news->SetItemText(j, 4, Game::GetText(info->SourceName()).c_str());

        if (!info->Visited())
          auto_scroll = true;
      }

      if (lst_news->GetSortColumn() > 0)
        lst_news->SortItems();

      txt_news->SetText("");

      if (img_news)
        img_news->SetPicture(bmp_default);
    }

    if (auto_scroll)
    {
      int first_unread = -1;

      for (int i = 0; i < lst_news->NumItems(); i++)
      {
        if (lst_news->GetItemText(i, 0) == "*")
        {
          first_unread = i;
          break;
        }
      }

      if (first_unread >= 0)
        lst_news->ScrollTo(first_unread);
    }
  }

  Starshatter* stars = Starshatter::GetInstance();

  if (start_scene > 0)
  {
    ShowMovie();

    start_scene--;

    if (start_scene == 0)
    {
      if (stars && campaign)
      {
        stars->ExecCutscene(event_scene, campaign->Path());

        if (stars->InCutscene())
        {
          Sim* sim = Sim::GetSim();

          if (sim)
          {
            cam_view->UseCamera(CameraDirector::GetInstance()->GetCamera());
            cam_view->UseScene(sim->GetScene());
          }
        }
      }

      event_scene = "";
    }
  }
  else
  {
    if (dsp_view)
      dsp_view->ExecFrame();

    if (stars->InCutscene())
      ShowMovie();
    else
      HideMovie();
  }
}

void CmdIntelDlg::OnSave(AWEvent* event) { CmdDlg::OnSave(event); }

void CmdIntelDlg::OnExit(AWEvent* event) { CmdDlg::OnExit(event); }

void CmdIntelDlg::OnMode(AWEvent* event) { CmdDlg::OnMode(event); }

void CmdIntelDlg::OnNews(AWEvent* e)
{
  CombatEvent* event = nullptr;
  int index = lst_news->GetSelection();

  if (index >= 0)
    event = (CombatEvent*)lst_news->GetItemData(index, 0);

  if (event)
  {
    std::string info("<font Limerick12><color ffff80>");
    info += event->Title();
    info += "<font Verdana><color ffffff>\n\n";
    info += event->Information();

    txt_news->SetText(info);
    txt_news->EnsureVisible(0);

    lst_news->SetItemText(index, 0, " ");

    if (img_news)
    {
      if (event->Image().Width() >= 64)
        img_news->SetPicture(event->Image());
      else
        img_news->SetPicture(bmp_default);
    }

    if (btn_play)
    {
      if (!event->SceneFile().empty())
        btn_play->Show();
      else
        btn_play->Hide();
    }

    if (!event->Visited() && btn_play->IsEnabled())
      OnPlay(nullptr);

    event->SetVisited(true);
  }
  else
  {
    txt_news->SetText("");

    if (img_news)
      img_news->SetPicture(bmp_default);

    if (btn_play)
      btn_play->Hide();
  }
}

void CmdIntelDlg::OnPlay(AWEvent* e)
{
  CombatEvent* event = nullptr;
  int index = lst_news->GetSelection();

  if (index >= 0)
    event = (CombatEvent*)lst_news->GetItemData(index, 0);

  if (mov_news && cam_view && event && !event->SceneFile().empty())
  {
    event_scene = event->SceneFile();
    start_scene = 2;
    ShowMovie();
  }
}

void CmdIntelDlg::ShowMovie()
{
  if (mov_news)
  {
    mov_news->Show();
    dsp_view->SetWindow(mov_news);

    if (img_news)
      img_news->Hide();
    if (txt_news)
      txt_news->Hide();
    if (btn_play)
      btn_play->Hide();
  }
}

void CmdIntelDlg::HideMovie()
{
  CombatEvent* event = nullptr;
  int index = lst_news->GetSelection();
  bool play = false;

  if (index >= 0)
  {
    event = (CombatEvent*)lst_news->GetItemData(index, 0);

    if (event && !event->SceneFile().empty())
      play = true;
  }

  if (mov_news)
  {
    mov_news->Hide();

    if (img_news)
      img_news->Show();
    if (txt_news)
      txt_news->Show();
    if (btn_play)
    {
      if (play)
        btn_play->Show();
      else
        btn_play->Hide();
    }
  }
}

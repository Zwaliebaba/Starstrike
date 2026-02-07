#include "pch.h"
#include "AwardDlg.h"
#include "Button.h"
#include "Campaign.h"
#include "Game.h"
#include "ImageBox.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "PlanScreen.h"
#include "Player.h"
#include "Sound.h"
#include "Starshatter.h"

DEF_MAP_CLIENT(AwardDlg, OnClose);

AwardDlg::AwardDlg(Screen* s, FormDef& def, PlanScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    lbl_name(nullptr),
    lbl_info(nullptr),
    img_rank(nullptr),
    btn_close(nullptr),
    exit_latch(true) { Init(def); }

AwardDlg::~AwardDlg() {}

void AwardDlg::RegisterControls()
{
  lbl_name = FindControl(203);
  lbl_info = FindControl(201);
  img_rank = static_cast<ImageBox*>(FindControl(202));

  btn_close = static_cast<Button*>(FindControl(1));
  REGISTER_CLIENT(EID_CLICK, btn_close, AwardDlg, OnClose);
}

void AwardDlg::Show()
{
  FormWindow::Show();
  ShowPlayer();

  exit_latch = true;
}

void AwardDlg::ExecFrame()
{
  if (Keyboard::KeyDown(VK_RETURN))
  {
    if (!exit_latch)
      OnClose(nullptr);
  }

  else if (Keyboard::KeyDown(VK_ESCAPE))
  {
    if (!exit_latch)
      OnClose(nullptr);
  }

  else
    exit_latch = false;
}

void AwardDlg::ShowPlayer()
{
  Player* p = Player::GetCurrentPlayer();

  if (p)
  {
    if (lbl_name)
      lbl_name->SetText(p->AwardName());

    if (lbl_info)
      lbl_info->SetText(p->AwardDesc());

    if (img_rank)
    {
      img_rank->SetPicture(*p->AwardImage());
      img_rank->Show();
    }

    Sound* congrats = p->AwardSound();
    if (congrats)
      congrats->Play();
  }
  else
  {
    if (lbl_info)
      lbl_info->SetText("");
    if (img_rank)
      img_rank->Hide();
  }
}

void AwardDlg::OnClose(AWEvent* event)
{
  Player* player = Player::GetCurrentPlayer();
  if (player)
    player->ClearShowAward();

  Starshatter* stars = Starshatter::GetInstance();

  if (stars)
  {
    Mouse::Show(false);

    Campaign* campaign = Campaign::GetCampaign();
    if (campaign && campaign->GetCampaignId() < Campaign::SINGLE_MISSIONS)
      stars->SetGameMode(Starshatter::CMPN_MODE);
    else
      stars->SetGameMode(Starshatter::MENU_MODE);
  }

  else
    Game::Panic("AwardDlg::OnClose() - Game instance not found");
}

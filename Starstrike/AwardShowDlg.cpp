#include "pch.h"
#include "AwardShowDlg.h"
#include "Button.h"
#include "ImageBox.h"
#include "Keyboard.h"
#include "MenuScreen.h"
#include "Player.h"
#include "Screen.h"

DEF_MAP_CLIENT(AwardShowDlg, OnClose);

AwardShowDlg::AwardShowDlg(Screen* s, FormDef& def, MenuScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    lbl_name(nullptr),
    lbl_info(nullptr),
    img_rank(nullptr),
    btn_close(nullptr),
    exit_latch(true),
    rank(-1),
    medal(-1) { Init(def); }

AwardShowDlg::~AwardShowDlg() {}

void AwardShowDlg::RegisterControls()
{
  lbl_name = FindControl(203);
  lbl_info = FindControl(201);
  img_rank = static_cast<ImageBox*>(FindControl(202));

  btn_close = static_cast<Button*>(FindControl(1));
  REGISTER_CLIENT(EID_CLICK, btn_close, AwardShowDlg, OnClose);
}

void AwardShowDlg::Show()
{
  FormWindow::Show();
  ShowAward();
}

void AwardShowDlg::ExecFrame()
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

void AwardShowDlg::SetRank(int r)
{
  rank = r;
  medal = -1;
}

void AwardShowDlg::SetMedal(int m)
{
  rank = -1;
  medal = m;
}

void AwardShowDlg::ShowAward()
{
  if (rank >= 0)
  {
    if (lbl_name)
      lbl_name->SetText(std::string("Rank of ") + Player::RankName(rank));

    if (lbl_info)
      lbl_info->SetText(Player::RankDescription(rank));

    if (img_rank)
    {
      img_rank->SetPicture(*Player::RankInsignia(rank, 1));
      img_rank->Show();
    }
  }

  else if (medal >= 0)
  {
    if (lbl_name)
      lbl_name->SetText(Player::MedalName(medal));

    if (lbl_info)
      lbl_info->SetText(Player::MedalDescription(medal));

    if (img_rank)
    {
      img_rank->SetPicture(*Player::MedalInsignia(medal, 1));
      img_rank->Show();
    }
  }

  else
  {
    if (lbl_name)
      lbl_name->SetText("");
    if (lbl_info)
      lbl_info->SetText("");
    if (img_rank)
      img_rank->Hide();
  }
}

void AwardShowDlg::OnClose(AWEvent* event) { manager->ShowPlayerDlg(); }

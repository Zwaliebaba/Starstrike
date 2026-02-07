#include "pch.h"
#include "LoadDlg.h"
#include "ComboBox.h"
#include "Game.h"
#include "Slider.h"
#include "Starshatter.h"

LoadDlg::LoadDlg(Screen* s, const FormDef& def)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    activity(nullptr),
    progress(nullptr) { FormWindow::Init(def); }

LoadDlg::~LoadDlg() {}

void LoadDlg::RegisterControls()
{
  title = FindControl(100);
  activity = FindControl(101);
  progress = static_cast<Slider*>(FindControl(102));
}

void LoadDlg::ExecFrame()
{
  Starshatter* stars = Starshatter::GetInstance();

  if (stars)
  {
    if (title)
    {
      if (stars->GetGameMode() == Starshatter::CLOD_MODE || stars->GetGameMode() == Starshatter::CMPN_MODE)
        title->SetText(Game::GetText("LoadDlg.campaign"));

      else if (stars->GetGameMode() == Starshatter::MENU_MODE)
        title->SetText(Game::GetText("LoadDlg.tac-ref"));

      else
        title->SetText(Game::GetText("LoadDlg.mission"));
    }

    activity->SetText(stars->GetLoadActivity());
    progress->SetValue(stars->GetLoadProgress());
  }
}

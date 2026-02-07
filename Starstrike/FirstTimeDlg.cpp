#include "pch.h"
#include "FirstTimeDlg.h"
#include "Button.h"
#include "ComboBox.h"
#include "EditBox.h"
#include "KeyMap.h"
#include "Keyboard.h"
#include "MenuScreen.h"
#include "Player.h"
#include "Random.h"
#include "Ship.h"
#include "Starshatter.h"

DEF_MAP_CLIENT(FirstTimeDlg, OnApply);

FirstTimeDlg::FirstTimeDlg(Screen* s, FormDef& def, MenuScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr) { FormWindow::Init(def); }

FirstTimeDlg::~FirstTimeDlg() {}

void FirstTimeDlg::RegisterControls()
{
  edt_name = static_cast<EditBox*>(FindControl(200));
  cmb_playstyle = static_cast<ComboBox*>(FindControl(201));
  cmb_experience = static_cast<ComboBox*>(FindControl(202));

  btn_apply = static_cast<Button*>(FindControl(1));
  REGISTER_CLIENT(EID_CLICK, btn_apply, FirstTimeDlg, OnApply);
}

void FirstTimeDlg::Show()
{
  if (!IsShown())
    FormWindow::Show();

  if (edt_name)
    edt_name->SetText("Noobie");
}

void FirstTimeDlg::ExecFrame() {}

void FirstTimeDlg::OnApply(AWEvent* event)
{
  Starshatter* stars = Starshatter::GetInstance();
  Player* player = Player::GetCurrentPlayer();

  if (player)
  {
    if (edt_name)
    {
      char password[16];
      sprintf_s(password, "%08x", static_cast<DWORD>(Random(0, 2e9)));

      player->SetName(edt_name->GetText());
      player->SetPassword(password);
    }

    if (cmb_playstyle)
    {
      // ARCADE:
      if (cmb_playstyle->GetSelectedIndex() == 0)
      {
        player->SetFlightModel(2);
        player->SetLandingModel(1);
        player->SetHUDMode(0);
        player->SetGunsight(1);

        if (stars)
        {
          KeyMap& keymap = stars->GetKeyMap();

          keymap.Bind(KEY_CONTROL_MODEL, 1, 0);
          keymap.SaveKeyMap("key.cfg", 256);

          stars->MapKeys();
        }

        Ship::SetControlModel(1);
      }

      // HARDCORE:
      else
      {
        player->SetFlightModel(0);
        player->SetLandingModel(0);
        player->SetHUDMode(0);
        player->SetGunsight(0);

        if (stars)
        {
          KeyMap& keymap = stars->GetKeyMap();

          keymap.Bind(KEY_CONTROL_MODEL, 0, 0);
          keymap.SaveKeyMap("key.cfg", 256);

          stars->MapKeys();
        }

        Ship::SetControlModel(0);
      }
    }

    if (cmb_experience && cmb_experience->GetSelectedIndex() > 0)
    {
      player->SetRank(2);        // Lieutenant
      player->SetTrained(255);   // Fully Trained
    }

    Player::Save();
  }

  manager->ShowMenuDlg();
}

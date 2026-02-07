#include "pch.h"
#include "ExitDlg.h"
#include "Button.h"
#include "DataLoader.h"
#include "FormatUtil.h"
#include "Game.h"
#include "Keyboard.h"
#include "MenuScreen.h"
#include "MusicDirector.h"
#include "RichTextBox.h"
#include "Starshatter.h"

DEF_MAP_CLIENT(ExitDlg, OnApply);
DEF_MAP_CLIENT(ExitDlg, OnCancel);

ExitDlg::ExitDlg(Screen* s, FormDef& def, MenuScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    credits(nullptr),
    apply(nullptr),
    cancel(nullptr),
    def_rect(def.GetRect()),
    exit_latch(false) { Init(def); }

ExitDlg::~ExitDlg() {}

void ExitDlg::RegisterControls()
{
  if (apply)
    return;

  credits = static_cast<RichTextBox*>(FindControl(201));

  apply = static_cast<Button*>(FindControl(1));
  REGISTER_CLIENT(EID_CLICK, apply, ExitDlg, OnApply);

  cancel = static_cast<Button*>(FindControl(2));
  REGISTER_CLIENT(EID_CLICK, cancel, ExitDlg, OnCancel);
}

void ExitDlg::ExecFrame()
{
  if (credits && credits->GetLineCount() > 0)
  {
    credits->SmoothScroll(ScrollWindow::SCROLL_DOWN, Game::GUITime());

    if (credits->GetTopIndex() >= credits->GetLineCount() - 1)
      credits->ScrollTo(0);
  }

  if (Keyboard::KeyDown(VK_RETURN))
    OnApply(nullptr);

  if (Keyboard::KeyDown(VK_ESCAPE))
  {
    if (!exit_latch)
      OnCancel(nullptr);
  }
  else
    exit_latch = false;
}

void ExitDlg::Show()
{
  if (!IsShown())
  {
    Rect r = def_rect;

    if (r.w > screen->Width())
    {
      int extra = r.w - screen->Width();
      r.w -= extra;
    }

    if (r.h > screen->Height())
    {
      int extra = r.h - screen->Height();
      r.h -= extra;
    }

    r.x = (screen->Width() - r.w) / 2;
    r.y = (screen->Height() - r.h) / 2;

    MoveTo(r);

    exit_latch = true;
    Button::PlaySound(Button::SND_CONFIRM);
    MusicDirector::SetMode(MusicDirector::CREDITS);

    DataLoader* loader = DataLoader::GetLoader();
    BYTE* block = nullptr;

    loader->SetDataPath();
    loader->LoadBuffer("credits.txt", block, true);

    if (block && credits)
      credits->SetText((const char*)block);

    loader->ReleaseBuffer(block);
  }

  FormWindow::Show();
}

void ExitDlg::OnApply(AWEvent* event)
{
  Starshatter* stars = Starshatter::GetInstance();

  if (stars)
  {
    ::DebugTrace("Exit Confirmed.\n");
    stars->Exit();
  }
}

void ExitDlg::OnCancel(AWEvent* event)
{
  manager->ShowMenuDlg();
  MusicDirector::SetMode(MusicDirector::MENU);
}

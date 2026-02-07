#include "pch.h"
#include "CmpCompleteDlg.h"
#include "Button.h"
#include "Campaign.h"
#include "CmpnScreen.h"
#include "CombatEvent.h"
#include "DataLoader.h"
#include "ImageBox.h"
#include "Starshatter.h"

DEF_MAP_CLIENT(CmpCompleteDlg, OnClose);

CmpCompleteDlg::CmpCompleteDlg(Screen* s, FormDef& def, CmpnScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    img_title(nullptr),
    lbl_info(nullptr),
    btn_close(nullptr),
    manager(mgr) { Init(def); }

CmpCompleteDlg::~CmpCompleteDlg() {}

void CmpCompleteDlg::RegisterControls()
{
  img_title = static_cast<ImageBox*>(FindControl(100));
  lbl_info = FindControl(101);
  btn_close = static_cast<Button*>(FindControl(1));

  REGISTER_CLIENT(EID_CLICK, btn_close, CmpCompleteDlg, OnClose);
}

void CmpCompleteDlg::Show()
{
  FormWindow::Show();

  Campaign* c = Campaign::GetCampaign();

  if (img_title && c)
  {
    DataLoader* loader = DataLoader::GetLoader();
    Starshatter* stars = Starshatter::GetInstance();
    CombatEvent* event = c->GetLastEvent();
    char img_name[256];

    if (event)
    {
      strcpy_s(img_name, event->ImageFile().c_str());

      if (!strstr(img_name, ".pcx"))
        strcat_s(img_name, ".pcx");

      if (loader)
      {
        loader->SetDataPath(c->Path());
        loader->LoadBitmap(img_name, banner);
        loader->SetDataPath();

        Rect tgt_rect;
        tgt_rect.w = img_title->Width();
        tgt_rect.h = img_title->Height();

        img_title->SetTargetRect(tgt_rect);
        img_title->SetPicture(banner);
      }
    }
  }
}

void CmpCompleteDlg::ExecFrame() {}

void CmpCompleteDlg::OnClose(AWEvent* event)
{
  if (manager)
    manager->ShowCmdDlg();
}

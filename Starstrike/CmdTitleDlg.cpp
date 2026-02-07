#include "pch.h"
#include "CmdTitleDlg.h"
#include "Campaign.h"
#include "ComboBox.h"
#include "Starshatter.h"

CmdTitleDlg::CmdTitleDlg(Screen* s, FormDef& def, CmpnScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    stars(nullptr),
    campaign(nullptr),
    showTime(0)
{
  stars = Starshatter::GetInstance();
  campaign = Campaign::GetCampaign();

  Init(def);
}

CmdTitleDlg::~CmdTitleDlg() {}

void CmdTitleDlg::RegisterControls() { img_title = static_cast<ImageBox*>(FindControl(200)); }

void CmdTitleDlg::Show() { FormWindow::Show(); }

void CmdTitleDlg::ExecFrame() {}

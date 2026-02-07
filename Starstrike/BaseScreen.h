#pragma once

class Screen;
class Sim;
class Window;
class Font;
class NavDlg;
class MsnElemDlg;
class AudDlg;
class VidDlg;
class OptDlg;
class CtlDlg;
class KeyDlg;
class MsgDlg;

class BaseScreen
{
  public:
    BaseScreen() {}
    virtual ~BaseScreen() {}

    virtual void ShowNavDlg() {}
    virtual void HideNavDlg() {}
    virtual bool IsNavShown() { return false; }
    virtual NavDlg* GetNavDlg() { return nullptr; }

    virtual void ShowMsnElemDlg() {}
    virtual void HideMsnElemDlg() {}
    virtual MsnElemDlg* GetMsnElemDlg() { return nullptr; }

    virtual AudDlg* GetAudDlg() const { return nullptr; }
    virtual VidDlg* GetVidDlg() const { return nullptr; }
    virtual OptDlg* GetOptDlg() const { return nullptr; }
    virtual CtlDlg* GetCtlDlg() const { return nullptr; }
    virtual KeyDlg* GetKeyDlg() const { return nullptr; }

    virtual void ShowAudDlg() {}
    virtual void ShowVidDlg() {}
    virtual void ShowOptDlg() {}
    virtual void ShowCtlDlg() {}
    virtual void ShowKeyDlg() {}

    virtual void ShowMsgDlg() {}
    virtual void HideMsgDlg() {}
    virtual bool IsMsgShown() { return false; }
    virtual MsgDlg* GetMsgDlg() { return nullptr; }

    virtual void ApplyOptions() {}
    virtual void CancelOptions() {}
};

#ifndef KeyDlg_h
#define KeyDlg_h

#include "FormWindow.h"

class BaseScreen;

class KeyDlg : public FormWindow
{
  public:
    KeyDlg(Screen* s, FormDef& def, BaseScreen* mgr);
    ~KeyDlg() override;

    void RegisterControls() override;
    void Show() override;

    // Operations:
    virtual void ExecFrame();

    virtual void OnApply(AWEvent* event);
    virtual void OnCancel(AWEvent* event);
    virtual void OnClear(AWEvent* event);

    int GetKeyMapIndex() const { return key_index; }
    void SetKeyMapIndex(int i);

  protected:
    BaseScreen* manager;

    int key_index;
    int key_key;
    int key_shift;
    int key_clear;

    Button* clear;
    Button* apply;
    Button* cancel;

    ActiveWindow* command;
    ActiveWindow* current_key;
    ActiveWindow* new_key;
};

#endif KeyDlg_h

#ifndef NetClientDlg_h
#define NetClientDlg_h

#include "Button.h"
#include "ComboBox.h"
#include "FormWindow.h"
#include "ListBox.h"
#include "NetClientConfig.h"
#include "NetLobby.h"

class MenuScreen;

class NetClientDlg : public FormWindow
{
  public:
    NetClientDlg(Screen* s, FormDef& def, MenuScreen* mgr);
    ~NetClientDlg() override;

    void RegisterControls() override;
    void Show() override;
    virtual void ExecFrame();

    // Operations:
    virtual void OnSelect(AWEvent* event);
    virtual void OnAdd(AWEvent* event);
    virtual void OnDel(AWEvent* event);
    virtual void OnServer(AWEvent* event);
    virtual void OnHost(AWEvent* event);
    virtual void OnJoin(AWEvent* event);
    virtual void OnCancel(AWEvent* event);

    virtual void ShowServers();
    virtual void UpdateServers();
    virtual void PingServer(int n);
    virtual bool PingComplete();
    virtual void StopNetProc();

  protected:
    MenuScreen* manager;
    NetClientConfig* config;

    Button* btn_add;
    Button* btn_del;
    ListBox* lst_servers;
    ActiveWindow* lbl_info;
    int server_index;
    int ping_index;
    HANDLE hnet;
    NetServerInfo ping_info;

    Button* btn_server;
    Button* btn_host;
    Button* btn_join;
    Button* btn_cancel;
};

#endif NetClientDlg_h

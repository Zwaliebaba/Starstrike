#ifndef NetAdminServer_h
#define NetAdminServer_h

#include "HttpServletExec.h"
#include "HttpServlet.h"

class Mission;
class MissionElement;
class NetChatEntry;
class NetUser;

class NetAdminServer : public HttpServletExec
{
  public:
    virtual ~NetAdminServer();

    int operator ==(const NetAdminServer& s) const { return this == &s; }

    virtual HttpServlet* GetServlet(HttpRequest& request);

    virtual void AddUser(NetUser* user);
    virtual void DelUser(NetUser* user);
    virtual int NumUsers();
    virtual bool HasHost();
    virtual List<NetUser>& GetUsers();

    virtual NetUser* FindUserBySession(std::string id);

    virtual void AddChat(NetUser* user, std::string_view msg);
    ListIter<NetChatEntry> GetChat();
    DWORD GetStartTime() const { return start_time; }

    virtual void GameOn() {}
    virtual void GameOff() {}

    // singleton locator:
    static NetAdminServer* GetInstance(WORD port = 0);

  protected:
    NetAdminServer(WORD port);
    virtual void DoSyncedCheck();

    DWORD start_time;
    List<NetUser> admin_users;
};

class NetAdminServlet : public HttpServlet
{
  public:
    NetAdminServlet();
    virtual ~NetAdminServlet() {}

    virtual bool DoGet(HttpRequest& request, HttpResponse& response);
    virtual bool CheckUser(HttpRequest& request, HttpResponse& response);

    virtual std::string GetCSS();
    virtual std::string GetHead(std::string_view title = {});
    virtual std::string GetBody();
    virtual std::string GetTitleBar(std::string_view statline = {}, std::string_view onload = {});
    virtual std::string GetStatLine();
    virtual std::string GetCopyright();
    virtual std::string GetContent();
    virtual std::string GetBodyClose();

  protected:
    NetAdminServer* admin;
    NetUser* user;
};

#endif NetAdminServer_h

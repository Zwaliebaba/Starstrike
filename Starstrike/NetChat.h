#ifndef NetChat_h
#define NetChat_h

#include "NetUser.h"

#undef GetMessage

class NetChatEntry
{
  public:
    NetChatEntry(const NetUser* user, std::string_view msg);
    NetChatEntry(int id, std::string_view user, std::string_view msg);
    ~NetChatEntry();

    int operator ==(const NetChatEntry& c) const { return id == c.id; }
    int operator <(const NetChatEntry& c) const { return id < c.id; }

    int GetID() const { return id; }
    std::string GetUser() const { return user; }
    Color GetColor() const { return color; }
    std::string GetMessage() const { return msg; }
    DWORD GetTime() const { return time; }

  private:
    int id;
    std::string user;
    std::string msg;
    Color color;
    DWORD time;
};

#endif NetChat_h

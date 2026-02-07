#include "pch.h"
#include "NetChat.h"
#include "NetLayer.h"

static int chat_id_key = 1000;

NetChatEntry::NetChatEntry(const NetUser* u, std::string_view s)
  : id(chat_id_key++),
    msg(s)
{
  if (u)
  {
    user = u->Name();
    color = u->GetColor();
  }
  else
  {
    user = "unknown";
    color = Color::Gray;
  }

  time = NetLayer::GetUTC();
}

NetChatEntry::NetChatEntry(int msg_id, std::string_view u, std::string_view s)
  : id(msg_id),
    user(u),
    msg(s)
{
  color = Color::Gray;
  time = NetLayer::GetUTC();

  if (id >= chat_id_key)
    chat_id_key = id + 1;
}

NetChatEntry::~NetChatEntry() {}

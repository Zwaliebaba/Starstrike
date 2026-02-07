#include "pch.h"
#include "NetServerConfig.h"
#include "NetAddr.h"
#include "NetAuth.h"
#include "NetGame.h"
#include "NetUser.h"
#include "ParseUtil.h"
#include "Reader.h"

NetServerConfig* NetServerConfig::instance = nullptr;
extern const char* g_versionInfo;

NetServerConfig::NetServerConfig()
{
  instance = this;

  name = "Starshatter ";
  admin_name = "system";
  admin_pass = "manager";
  admin_port = 11111;
  lobby_port = 11100;
  game_port = 11101;
  game_type = NET_GAME_PUBLIC;
  auth_level = NetAuth::NET_AUTH_STANDARD;
  poolsize = 8;
  session_timeout = 300;

  name += g_versionInfo;

  Load();
}

NetServerConfig::~NetServerConfig()
{
  instance = nullptr;

  banned_addrs.destroy();
}

void NetServerConfig::Initialize()
{
  if (!instance)
    instance = NEW NetServerConfig();
}

void NetServerConfig::Close()
{
  delete instance;
  instance = nullptr;
}

void NetServerConfig::Load()
{
  // read the config file:
  BYTE* block = nullptr;
  int blocklen = 0;
  int port = 0;

  char filename[64];
  strcpy_s(filename, "server.cfg");

  FILE* f;
  fopen_s(&f, filename, "rb");

  if (f)
  {
    fseek(f, 0, SEEK_END);
    blocklen = ftell(f);
    fseek(f, 0, SEEK_SET);

    block = NEW BYTE[blocklen + 1];
    block[blocklen] = 0;

    fread(block, blocklen, 1, f);
    fclose(f);
  }

  if (blocklen == 0)
  {
    delete [] block;
    return;
  }

  Parser parser(NEW BlockReader((const char*)block));
  Term* term = parser.ParseTerm();

  if (!term)
  {
    DebugTrace("ERROR: could not parse '%s'.\n", filename);
    delete [] block;
    return;
  }
  TermText* file_type = term->isText();
  if (!file_type || file_type->value() != "SERVER_CONFIG")
  {
    DebugTrace("WARNING: invalid '%s' file.  Using defaults\n", filename);
    delete [] block;
    return;
  }

  do
  {
    delete term;

    term = parser.ParseTerm();

    if (term)
    {
      TermDef* def = term->isDef();
      if (def)
      {
        if (def->name()->value() == "name")
          GetDefText(instance->name, def, filename);

        else if (def->name()->value() == "admin_name")
          GetDefText(instance->admin_name, def, filename);

        else if (def->name()->value() == "admin_pass")
          GetDefText(instance->admin_pass, def, filename);

        else if (def->name()->value() == "game_pass")
          GetDefText(instance->game_pass, def, filename);

        else if (def->name()->value() == "mission")
          GetDefText(instance->mission, def, filename);

        else if (def->name()->value() == "auth_level")
        {
          std::string level;

          if (def->term() && def->term()->isText())
          {
            GetDefText(level, def, filename);

            std::ranges::transform(level, level.begin(), ::tolower);

            if (level.starts_with("min"))
              instance->auth_level = NetAuth::NET_AUTH_MINIMAL;

            else if (level == "standard" || level == "std")
              instance->auth_level = NetAuth::NET_AUTH_STANDARD;

            else if (level == "secure")
              instance->auth_level = NetAuth::NET_AUTH_SECURE;
          }

          else
            GetDefNumber(instance->auth_level, def, filename);
        }

        else if (def->name()->value() == "admin_port")
        {
          GetDefNumber(port, def, filename);
          if (port > 1024 && port < 48000)
            instance->admin_port = static_cast<WORD>(port);
        }

        else if (def->name()->value() == "lobby_port")
        {
          GetDefNumber(port, def, filename);
          if (port > 1024 && port < 48000)
            instance->lobby_port = static_cast<WORD>(port);
        }

        else if (def->name()->value() == "game_port")
        {
          GetDefNumber(port, def, filename);
          if (port > 1024 && port < 48000)
            instance->game_port = static_cast<WORD>(port);
        }

        else if (def->name()->value() == "game_type")
        {
          std::string type;
          GetDefText(type, def, filename);

          if (type == "LAN")
            instance->game_type = NET_GAME_LAN;

          else if (type == "private")
            instance->game_type = NET_GAME_PRIVATE;

          else
            instance->game_type = NET_GAME_PUBLIC;
        }

        else if (def->name()->value() == "poolsize")
          GetDefNumber(instance->poolsize, def, filename);

        else if (def->name()->value() == "session_timeout")
          GetDefNumber(instance->session_timeout, def, filename);

        else { DebugTrace("WARNING: unknown label '%s' in '%s'\n", def->name()->value().data(), filename); }
      }
      else
      {
        DebugTrace("WARNING: term ignored in '%s'\n", filename);
        term->print();
      }
    }
  }
  while (term);

  delete [] block;

  LoadBanList();
}

void NetServerConfig::Save()
{
  FILE* f;
  fopen_s(&f, "server.cfg", "w");
  if (f)
  {
    fprintf(f, "SERVER_CONFIG\n\n");
    fprintf(f, "name:            \"%s\"\n", instance->name.data());
    fprintf(f, "admin_name:      \"%s\"\n", instance->admin_name.data());
    fprintf(f, "admin_pass:      \"%s\"\n", instance->admin_pass.data());
    fprintf(f, "game_pass:       \"%s\"\n", instance->game_pass.data());
    fprintf(f, "\n");
    fprintf(f, "admin_port:      %d\n", instance->admin_port);
    fprintf(f, "lobby_port:      %d\n", instance->lobby_port);
    fprintf(f, "game_port:       %d\n", instance->game_port);

    switch (instance->game_type)
    {
      case NET_GAME_LAN:
        fprintf(f, "game_type:       LAN\n");
        break;

      case NET_GAME_PRIVATE:
        fprintf(f, "game_type:       private\n");
        break;

      case NET_GAME_PUBLIC: default:
        fprintf(f, "game_type:       public\n");
        break;
    }

    switch (instance->auth_level)
    {
      case NetAuth::NET_AUTH_MINIMAL:
        fprintf(f, "auth_level:      minimal\n");
        break;

      case NetAuth::NET_AUTH_STANDARD: default:
        fprintf(f, "auth_level:      standard\n");
        break;

      case NetAuth::NET_AUTH_SECURE:
        fprintf(f, "auth_level:      secure\n");
        break;
    }

    fprintf(f, "\n");
    fprintf(f, "poolsize:        %d\n", instance->poolsize);
    fprintf(f, "session_timeout: %d\n", instance->session_timeout);

    if (mission.length() > 0)
      fprintf(f, "\nmission:         \"%s\"\n", instance->mission.data());

    fclose(f);
  }
}

std::string NetServerConfig::Clean(std::string_view s)
{
  if (s.empty())
    return {};

  int len = s.length();
  auto buff = NEW char[len + 1];
  ZeroMemory(buff, len+1);

  char* p = buff;

  for (int i = 0; i < len; i++)
  {
    char c = s[i];

    if (c >= 32 && c < 127)
      *p++ = c;
  }

  std::string result(buff);
  delete [] buff;

  return result;
}

void NetServerConfig::LoadBanList()
{
  // read the config file:
  BYTE* block = nullptr;
  int blocklen = 0;
  int port = 0;

  char filename[64];
  strcpy_s(filename, "banned.cfg");

  FILE* f;
  fopen_s(&f, filename, "rb");

  if (f)
  {
    fseek(f, 0, SEEK_END);
    blocklen = ftell(f);
    fseek(f, 0, SEEK_SET);

    block = NEW BYTE[blocklen + 1];
    block[blocklen] = 0;

    fread(block, blocklen, 1, f);
    fclose(f);
  }

  if (blocklen == 0)
  {
    delete [] block;
    return;
  }

  Parser parser(NEW BlockReader((const char*)block));
  Term* term = parser.ParseTerm();

  if (!term)
  {
    DebugTrace("ERROR: could not parse '%s'.\n", filename);
    delete [] block;
    return;
  }
  TermText* file_type = term->isText();
  if (!file_type || file_type->value() != "BANNED_CONFIG")
  {
    DebugTrace("WARNING: invalid '%s' file.\n", filename);
    delete [] block;
    return;
  }

  banned_addrs.destroy();
  banned_names.clear();

  do
  {
    delete term;

    term = parser.ParseTerm();

    if (term)
    {
      TermDef* def = term->isDef();
      if (def)
      {
        if (def->name()->value() == "name")
        {
          std::string name;
          GetDefText(name, def, filename);
          banned_names.emplace_back(name);
        }

        else if (def->name()->value() == "addr")
        {
          DWORD addr;
          GetDefNumber(addr, def, filename);
          banned_addrs.append(NEW NetAddr(addr));
        }
      }

      else
      {
        DebugTrace("WARNING: term ignored in '%s'\n", filename);
        term->print();
      }
    }
  }
  while (term);

  delete [] block;
}

void NetServerConfig::BanUser(NetUser* user)
{
  if (!user || IsUserBanned(user))
    return;

  auto user_addr = NEW NetAddr(user->GetAddress().IPAddr());

  banned_addrs.append(user_addr);
  banned_names.emplace_back(user->Name());

  FILE* f;
  fopen_s(&f, "banned.cfg", "w");
  if (f)
  {
    fprintf(f, "BANNED_CONFIG\n\n");

    ListIter<NetAddr> a_iter = banned_addrs;
    while (++a_iter)
    {
      NetAddr* addr = a_iter.value();
      fprintf(f, "addr: 0x%08x  // %d.%d.%d.%d\n", addr->IPAddr(), addr->B1(), addr->B2(), addr->B3(), addr->B4());
    }

    fprintf(f, "\n");

    for (auto& name : banned_names)
    {
      fprintf(f, "name: \"%s\"\n", name.c_str());
    }

    fclose(f);
  }
}

bool NetServerConfig::IsUserBanned(const NetUser* user)
{
  if (user)
  {
    NetAddr user_addr = user->GetAddress();

    user_addr.SetPort(0);

    return banned_addrs.contains(&user_addr) || std::ranges::find(banned_names, user->Name()) != banned_names.end();
  }

  return false;
}

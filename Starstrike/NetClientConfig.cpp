#include "pch.h"
#include "NetClientConfig.h"
#include "NetAddr.h"
#include "NetBrokerClient.h"
#include "NetLayer.h"
#include "NetLobbyClient.h"
#include "ParseUtil.h"
#include "Reader.h"

NetClientConfig* NetClientConfig::instance = nullptr;

NetClientConfig::NetClientConfig()
  : server_index(-1),
    host_request(false),
    conn(nullptr)
{
  instance = this;
  Load();
}

NetClientConfig::~NetClientConfig()
{
  Logout();

  instance = nullptr;
  servers.destroy();
}

void NetClientConfig::Initialize()
{
  if (!instance)
    instance = NEW NetClientConfig();
}

void NetClientConfig::Close()
{
  delete instance;
  instance = nullptr;
}

void NetClientConfig::AddServer(std::string_view name, std::string_view addr, WORD port, std::string_view pass, bool save)
{
  if (addr.empty() || port < 1024 || port > 48000)
    return;

  char buffer[1024];
  if (!name.empty())
    strcpy_s(buffer, name.data());
  else
    sprintf_s(buffer, "%s:%d", addr.data(), port);

  auto server = NEW NetServerInfo;
  server->name = buffer;
  server->hostname = addr;
  server->addr = NetAddr(addr, port);
  server->port = port;
  server->password = pass;
  server->save = save;

  if (server->addr.IPAddr() == 0)
  {
    DebugTrace("NetClientConfig::AddServer({}, {}, {}) failed to resolve IP Addr\n", name, addr, port);
  }

  servers.append(server);
}

void NetClientConfig::DelServer(int index)
{
  if (index >= 0 && index < servers.size())
    delete servers.removeIndex(index);
}

NetServerInfo* NetClientConfig::GetServerInfo(int n)
{
  if (n >= 0 && n < servers.size())
    return servers.at(n);

  return nullptr;
}

NetServerInfo* NetClientConfig::GetSelectedServer()
{
  if (server_index >= 0 && server_index < servers.size())
    return servers.at(server_index);

  return nullptr;
}

void NetClientConfig::Download()
{
  Load();

  List<NetServerInfo> list;
  if (NetBrokerClient::GameList("Starshatter", list))
    servers.append(list);
}

void NetClientConfig::Load()
{
  server_index = -1;

  // read the config file:
  BYTE* block = nullptr;
  int blocklen = 0;

  char filename[64];
  strcpy_s(filename, "client.cfg");

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
    return;

  servers.destroy();

  Parser parser(NEW BlockReader((const char*)block));
  Term* term = parser.ParseTerm();

  if (!term)
  {
    DebugTrace("ERROR: could not parse '%s'.\n", filename);
    return;
  }
  TermText* file_type = term->isText();
  if (!file_type || file_type->value() != "CLIENT_CONFIG")
  {
    DebugTrace("WARNING: invalid '%s' file.  Using defaults\n", filename);
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
        if (def->name()->value() == "server")
        {
          if (!def->term() || !def->term()->isStruct())
            DebugTrace("WARNING: server struct missing in '%s'\n", filename);
          else
          {
            TermStruct* val = def->term()->isStruct();

            std::string name;
            std::string addr;
            std::string pass;
            int port = {};

            for (int i = 0; i < val->elements()->size(); i++)
            {
              TermDef* pdef = val->elements()->at(i)->isDef();
              if (pdef)
              {
                if (pdef->name()->value() == "name")
                  GetDefText(name, pdef, filename);
                else if (pdef->name()->value() == "addr")
                  GetDefText(addr, pdef, filename);
                else if (pdef->name()->value() == "pass")
                  GetDefText(pass, pdef, filename);
                else if (pdef->name()->value() == "port")
                  GetDefNumber(port, pdef, filename);
              }
            }

            AddServer(name, addr, static_cast<WORD>(port), pass, true);
          }
        }
        else { DebugTrace("WARNING: unknown label '%s' in '%s'\n", def->name()->value().data(), filename); }
      }
    }
  }
  while (term);

  delete [] block;
}

void NetClientConfig::Save()
{
  FILE* f;
  fopen_s(&f, "client.cfg", "w");
  if (f)
  {
    fprintf(f, "CLIENT_CONFIG\n\n");

    ListIter<NetServerInfo> iter = servers;
    while (++iter)
    {
      NetServerInfo* server = iter.value();

      if (server->save)
      {
        int port = server->port;
        fprintf(f, "server: {\n");
        fprintf(f, "  name: \"%s\",\n", server->name.c_str());
        fprintf(f, "  addr: \"%s\",\n", server->hostname.c_str());
        fprintf(f, "  port: %d,\n", port);

        if (!server->password.empty())
          fprintf(f, "  pass: \"%s\",\n", server->password.c_str());

        fprintf(f, "}\n\n");
      }
    }

    fclose(f);
  }
}

void NetClientConfig::CreateConnection()
{
  NetServerInfo* s = GetSelectedServer();

  if (s)
  {
    NetAddr addr = s->addr;

    if (conn)
    {
      if (conn->GetServerAddr().IPAddr() != addr.IPAddr() || conn->GetServerAddr().Port() != addr.Port())
      {
        conn->Logout();
        DropConnection();
      }
    }

    if (addr.IPAddr() && addr.Port() && !conn)
      conn = NEW NetLobbyClient; // (addr);
  }

  else if (conn)
  {
    conn->Logout();
    DropConnection();
  }
}

NetLobbyClient* NetClientConfig::GetConnection() { return conn; }

bool NetClientConfig::Login()
{
  bool result = false;

  if (!conn)
    CreateConnection();

  if (conn)
    result = conn->Login(host_request);

  return result;
}

bool NetClientConfig::Logout()
{
  bool result = false;

  if (conn)
  {
    result = conn->Logout();
    DropConnection();
  }

  return result;
}

void NetClientConfig::DropConnection()
{
  delete conn;
  conn = nullptr;
}

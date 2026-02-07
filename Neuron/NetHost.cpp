#include "pch.h"

#include "NetHost.h"
#include "NetLayer.h"
#include <ctype.h>

NetHost::NetHost()
{
  char host_name[256];
  gethostname(host_name, sizeof(host_name));

  Init(host_name);
}

NetHost::NetHost(const char* host_name) { Init(host_name); }

void NetHost::Init(const char* host_name)
{
  if (host_name && *host_name)
  {
    HOSTENT* h = 0;

    if (isdigit(*host_name))
    {
      DWORD addr = inet_addr(host_name);
      h = gethostbyaddr((const char*)&addr, 4, AF_INET);
    }
    else { h = gethostbyname(host_name); }

    if (h)
    {
      name = h->h_name;

      char** alias = h->h_aliases;
      while (*alias)
      {
        aliases.emplace_back(*alias);
        alias++;
      }

      char** addr = h->h_addr_list;
      while (*addr)
      {
        NetAddr* pna = NEW NetAddr(**(DWORD**)addr);
        if (pna)
          addresses.append(pna);
        addr++;
      }
    }
  }
}

NetHost::NetHost(const NetHost& n)
{
  if (&n != this)
  {
    NetHost& nh = (NetHost&)n;

    name = nh.name;

    auto alias = nh.aliases;
    for (auto& alias : nh.aliases)
      aliases.emplace_back(alias);

    ListIter<NetAddr> addr = nh.addresses;
    while (++addr)
      addresses.append(new NetAddr(*addr.value()));
  }
}

NetHost::~NetHost() { addresses.destroy(); }

std::string NetHost::Name() { return name; }

NetAddr NetHost::Address()
{
  if (addresses.size())
    return *(addresses[0]);

  return NetAddr((DWORD)0);
}

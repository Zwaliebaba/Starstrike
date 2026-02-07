#include "pch.h"
#include "NetAddr.h"

NetAddr::NetAddr(std::string_view host_name, WORD p)
  : addr(0),
    port(p)
{
  if (!host_name.empty())
  {
    HOSTENT* h = 0;

    if (isdigit(host_name[0]))
    {
      DWORD a = inet_addr(host_name.data());
      h = gethostbyaddr((const char*)&a, 4, AF_INET);
    }
    else { h = gethostbyname(host_name.data()); }

    if (h) { if (h->h_addr_list) { addr = **(DWORD**)(h->h_addr_list); } }
  }

  Init();
}

NetAddr::NetAddr(DWORD a, WORD p)
  : addr(a),
    port(p) { Init(); }

NetAddr::NetAddr(const NetAddr& n)
  : addr(n.addr),
    port(n.port) { Init(); }

void NetAddr::Init()
{
  ZeroMemory(&sadr, sizeof(sadr));

  sadr.sin_family = AF_INET;
  sadr.sin_port = ::htons(port);
  sadr.sin_addr.s_addr = addr;
}

void NetAddr::InitFromSockAddr()
{
  addr = sadr.sin_addr.s_addr;
  port = ::ntohs(sadr.sin_port);
}

sockaddr* NetAddr::GetSockAddr() const { return (sockaddr*)&sadr; }

size_t NetAddr::GetSockAddrLength() const { return sizeof(sadr); }

void NetAddr::SetSockAddr(sockaddr* s, int size)
{
  if (s)
  {
    ZeroMemory(&sadr, sizeof(sadr));

    if (size > sizeof(sadr))
      CopyMemory(&sadr, s, sizeof(sadr));
    else
      CopyMemory(&sadr, s, size);

    InitFromSockAddr();
  }
}

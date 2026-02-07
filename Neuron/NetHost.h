#ifndef NET_HOST_H
#define NET_HOST_H

#include "List.h"
#include "NetAddr.h"

class NetHost
{
  public:
    NetHost();
    NetHost(const char* host_addr);
    NetHost(const NetHost& n);
    ~NetHost();

    std::string Name();
    NetAddr Address();

    std::vector<std::string>& Aliases() { return aliases; }
    List<NetAddr>& AddressList() { return addresses; }

  private:
    void Init(const char* host_name);

    std::string name;
    std::vector<std::string> aliases;
    List<NetAddr> addresses;
};

#endif // NET_HOST_H

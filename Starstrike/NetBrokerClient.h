#ifndef NetBrokerClient_h
#define NetBrokerClient_h

#include "HttpClient.h"
#include "NetLobby.h"

class NetBrokerClient
{
  public:
    static void Enable() { broker_available = true; }
    static void Disable() { broker_available = false; }

    static bool GameOn(std::string_view name, std::string_view type, std::string_view addr, WORD port,
                       std::string_view password);
    static bool GameList(std::string_view type, List<NetServerInfo>& server_list);

  protected:
    static bool broker_available;
    static bool broker_found;
};

#endif NetBrokerClient_h

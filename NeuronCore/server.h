#pragma once

#include "ClientPeer.h"
#include "NetLib.h"
#include "NetworkUpdate.h"
#include "ServerToClientLetter.h"

class ServerTeam
{
  public:
    int m_clientId;

    ServerTeam(int _clientId);
};

class Server
{
  NetLib* m_netLib;
  NetSocket* m_socket;

  std::vector<ServerToClientLetter> m_history;

  public:
    int m_sequenceId;

    DArray<ClientPeer*> m_clients;
    DArray<ServerTeam*> m_teams;

    Concurrency::concurrent_queue<NetworkUpdate> m_inbox;
    Concurrency::concurrent_queue<ServerToClientLetter> m_outbox;

    DArray<unsigned char> m_sync;                                     // Synchronisation values for each sequenceId

    Server();
    ~Server();

    void Initialize();

    bool GetNextLetter(NetworkUpdate& _update);

    void ReceiveLetter(NetworkUpdate& update, char* fromIP);
    void SendLetter(ServerToClientLetter& letter);

    int GetClientId(const char* _ip);
    void RegisterNewClient(const char* _ip);
    void RemoveClient(const char* _ip);
    void RegisterNewTeam(char* _ip, int _teamType, int _desiredTeamId);

    void AdvanceSender();
    void Advance();

    static int ConvertIPToInt(const char* _ip);
    static char* ConvertIntToIP(int _ip);
};

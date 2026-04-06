#pragma once

#include "llist.h"
#include "darray.h"
#include <unordered_set>


class NetLib;
class NetMutex;
class NetSocketListner;
class ServerToClient;
class ServerToClientLetter;
class NetworkUpdate;


class ServerTeam
{
public:
    int m_clientId;

    ServerTeam(int _clientId);
};


// Per-client chunk subscription state for AoI-aware pheromone delivery (§A.2).
struct ClientChunkState
{
    int m_clientId = -1;
    std::unordered_set<unsigned int> m_subscribed;  // packed (cx << 16 | cz)

    static unsigned int PackChunkId(int _cx, int _cz)
    {
        return (static_cast<unsigned int>(_cx) << 16) | static_cast<unsigned int>(_cz);
    }
};


class Server
{
private:
    NetLib	        *m_netLib;

    LList           <ServerToClientLetter *> m_history;

public:
    int             m_sequenceId;

    DArray          <ServerToClient *> m_clients;
    DArray          <ServerTeam *> m_teams;

    NetMutex         *m_inboxMutex;
    NetMutex         *m_outboxMutex;
    LList           <NetworkUpdate *> m_inbox;
    LList           <ServerToClientLetter *> m_outbox;

    DArray          <unsigned char> m_sync;                                     // Synchronisation values for each sequenceId

    DArray          <ClientChunkState *> m_chunkStates;  // parallel to m_clients

public:
    Server();
    ~Server();

    void Initialise			();

    NetworkUpdate *GetNextLetter();

    void ReceiveLetter      ( NetworkUpdate *update, char *fromIP );
    void SendLetter         ( ServerToClientLetter *letter );

    int  GetClientId        ( char *_ip );
    void RegisterNewClient  ( char *_ip );
    void RemoveClient       ( char *_ip );
    void RegisterNewTeam    ( char *_ip, int _teamType, int _desiredTeamId );

	void AdvanceSender		();
    void Advance			();

    void LoadHistory        ( const char *_filename );
    void SaveHistory        ( const char *_filename );

    // --- Chunk subscription (AoI) ---
    void SendLetterToClient          ( ServerToClientLetter *_letter, int _clientId );
    void SendLetterToChunkSubscribers( ServerToClientLetter *_letter, int _chunkX, int _chunkZ );
    void SubscribeClientToChunk      ( int _clientId, int _chunkX, int _chunkZ );
    void UnsubscribeClientFromChunk  ( int _clientId, int _chunkX, int _chunkZ );

    static int   ConvertIPToInt( const char *_ip );
    static const char *ConvertIntToIP( const int _ip );
};


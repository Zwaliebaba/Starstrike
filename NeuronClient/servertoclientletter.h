#pragma once

#include "llist.h"
#include "networkupdate.h"


#define SERVERTOCLIENTLETTER_BYTESTREAMSIZE	1024


// ****************************************************************************
//  Class ServerToClientLetter
// ****************************************************************************

class ServerToClientLetter
{
public:
    enum LetterType
    {
        Invalid,
        HelloClient,
        GoodbyeClient,
        TeamAssign,
        Update,
        ChunkPheromoneUpdate,    // Bulk pheromone delta for one chunk (§A.5)
        ChunkPheromoneFullSync   // Full pheromone state for one chunk
    };

    LetterType m_type;                      // If you add any new data here, remember to update the copy constructor
    unsigned char m_teamId;
    unsigned char m_teamType;
    int m_ip;                               // This tells you specifically which client gets the HelloClient or TeamAssign

    LList<NetworkUpdate *> m_updates;

    // Variable-length bulk payload for pheromone data (§A.5).
    // Heap-allocated; nullptr when unused.  Layout depends on m_type:
    //   ChunkPheromoneUpdate:   [int chunkX][int chunkZ][ushort count][PhDelta × count]
    //   ChunkPheromoneFullSync: [int chunkX][int chunkZ][raw float pairs]
    char* m_bulkData;
    int   m_bulkDataSize;

private:
    int m_clientId;                 // An index into the server's DArray of ServerToClient objects
    int m_sequenceId;

public:
    ServerToClientLetter();
    ServerToClientLetter( ServerToClientLetter &copyMe );
	ServerToClientLetter(char *_byteStream, int _len);
    ~ServerToClientLetter();

    void SetClientId    (int _id);
    void SetType        ( LetterType _type );
    void SetSequenceId  (int seqId);
    void SetTeamId      (int teamId);
    void SetTeamType    (int teamType);
    void SetIp          (int ip);

    int GetClientId();
    int GetSequenceId();

    void AddUpdate              ( NetworkUpdate *_update );

	// Writes all the current data into a sequential byte stream suitable to
	// be stuffed into a UDP packet. Sets linearSize to be the stream length.
	// Do NOT DELETE the returned pointer - it is part of this object.
	char *GetByteStream(int *_linearSize);
};


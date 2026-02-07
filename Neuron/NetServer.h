#ifndef NetServer_h
#define NetServer_h

#include "NetAddr.h"
#include "NetSock.h"


class NetServer
{
  public:
    NetServer(WORD port, int poolsize = 1);
    virtual ~NetServer();

    int operator ==(const NetServer& l) const { return addr == l.addr; }

    virtual void Shutdown();
    virtual DWORD Listener();
    virtual DWORD Reader(int index);

    virtual std::string ProcessRequest(std::string request, const NetAddr& addr);
    virtual std::string DefaultResponse();
    virtual std::string ErrorResponse();

    const NetAddr& GetAddress() const { return addr; }
    int GetLastError() const { return err; }

  protected:
    NetAddr addr;
    NetSock sock;
    NetAddr client_addr;

    int poolsize;
    HANDLE hreader;
    HANDLE* pool;
    NetSock** conn;
    NetAddr* clients;
    int err;
    bool server_shutdown;

    std::mutex sync;
};

#endif NetServer_h

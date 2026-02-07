#include "pch.h"
#include "NetServer.h"
#include <mmsystem.h>
#include "NetHost.h"
#include "NetLayer.h"

DWORD WINAPI NetServerListenerProc(LPVOID link);
DWORD WINAPI NetServerReaderProc(LPVOID link);

struct PoolItem
{
  NetServer* server;
  int thread_index;
};

NetServer::NetServer(WORD port, int nthreads)
  : sock(true),
    poolsize(nthreads),
    hreader(nullptr),
    pool(nullptr),
    conn(nullptr),
    err(0),
    server_shutdown(false)
{
  NetHost host;
  addr = NetAddr(host.Address().IPAddr(), port);

  sock.bind(addr);
  sock.listen(3);

  if (poolsize < 1)
    poolsize = 1;

  pool = NEW HANDLE[poolsize];
  conn = NEW NetSock*[poolsize];
  clients = NEW NetAddr[poolsize];

  if (pool && conn && clients)
  {
    ZeroMemory(pool, poolsize * sizeof(HANDLE));
    ZeroMemory(conn, poolsize * sizeof(NetSock*));

    DWORD thread_id = 0;

    for (int i = 0; i < poolsize; i++)
    {
      thread_id = 0;
      auto item = new PoolItem;
      item->server = this;
      item->thread_index = i;

      pool[i] = CreateThread(nullptr, 4096, NetServerReaderProc, item, 0, &thread_id);
    }

    thread_id = 0;
    hreader = CreateThread(nullptr, 4096, NetServerListenerProc, this, 0, &thread_id);
  }
}

NetServer::~NetServer()
{
  if (!server_shutdown)
  {
    server_shutdown = true;
    sock.close();
  }

  if (hreader)
  {
    WaitForSingleObject(hreader, 1000);
    CloseHandle(hreader);
  }

  if (pool && poolsize)
  {
    for (int i = 0; i < poolsize; i++)
    {
      WaitForSingleObject(pool[i], 1000);
      CloseHandle(pool[i]);
      delete conn[i];
      conn[i] = nullptr;
    }

    delete [] pool;
    delete [] conn;
    delete [] clients;
  }
}

void NetServer::Shutdown() { server_shutdown = true; }

DWORD WINAPI NetServerListenerProc(LPVOID link)
{
  auto net_server = static_cast<NetServer*>(link);

  if (net_server)
    return net_server->Listener();

  return static_cast<DWORD>(E_POINTER);
}

DWORD NetServer::Listener()
{
  while (!server_shutdown)
  {
    NetSock* s = sock.accept(&client_addr);

    while (s)
    {
      sync.lock();

      for (int i = 0; i < poolsize; i++)
      {
        if (conn[i] == nullptr)
        {
          conn[i] = s;
          clients[i] = client_addr;
          s = nullptr;
          break;
        }
      }

      sync.unlock();

      // wait for a thread to become not busy
      if (s)
        Sleep(10);
    }
  }

  return 0;
}

DWORD WINAPI NetServerReaderProc(LPVOID link)
{
  if (!link)
    return static_cast<DWORD>(E_POINTER);

  auto item = static_cast<PoolItem*>(link);
  NetServer* net_server = item->server;
  int index = item->thread_index;

  delete item;

  if (net_server)
    return net_server->Reader(index);

  return static_cast<DWORD>(E_POINTER);
}

DWORD NetServer::Reader(int index)
{
  // init random seed for this thread:
  srand(timeGetTime());

  while (!server_shutdown)
  {
    sync.lock();
    NetSock* s = conn[index];
    sync.unlock();

    if (s)
    {
      constexpr int MAX_REQUEST = 4096;
      std::string request;

      /***
       *** NOT SURE WHY, BUT THIS DOESN'T WORK FOR SHIT
       ***
       *** Setting the socket timeout to 2 seconds caused it
       *** to wait for two seconds, read nothing, and give up
       *** with a WSAETIMEDOUT error.  Meanwhile, the client
       *** immediately registered a failure (during the 2 sec
       *** delay) and aborted the request.
       ***

      s->set_timeout(2000);
      std::string msg = s->recv();

      while (msg.length() > 0 && request.length() < MAX_REQUEST) {
          request += msg;
          msg = s->recv();
      }

       ***/

      request = s->recv();

      if (!request.empty() && !s->is_closed())
      {
        std::string response = ProcessRequest(request, clients[index]);
        err = s->send(response);
        if (err < 0)
          err = NetLayer::GetLastError();
      }

      sync.lock();
      delete conn[index];
      conn[index] = nullptr;
      sync.unlock();
    }
    else
      Sleep(5);
  }

  return 0;
}

std::string NetServer::ProcessRequest(std::string msg, const NetAddr& addr)
{
  if (msg.starts_with("GET "))
    return DefaultResponse();

  return ErrorResponse();
}

std::string NetServer::DefaultResponse()
{
  std::string response =
    "HTTP/1.0 200 OK\nServer: Generic NetServer 1.0\nMIME-Version: 1.0\nContent-type: text/html\n\n";

  response += "<html><head><title>Generic NetServer 1.0</title></head>\n\n";
  response += "<body bgcolor=\"black\" text=\"white\">\n";
  response += "<h1>Generic NetServer 1.0</h1>\n";
  response += "<p>Didn't think I could do it, did ya?\n";
  response += "</body></html>\n\n";

  return response;
}

std::string NetServer::ErrorResponse()
{
  std::string response =
    "HTTP/1.0 501 Not Implemented\nServer: Generic NetServer 1.0\nMIME-Version: 1.0\nContent-type: text/html\n\n";

  response += "<html><head><title>Generic NetServer 1.0</title></head>\n\n";
  response += "<body bgcolor=\"black\" text=\"white\">\n";
  response += "<h1>Generic NetServer 1.0</h1>\n";
  response += "<p>Sorry charlie...  I'm not a magician.\n";
  response += "</body></html>\n\n";

  return response;
}

#include "pch.h"
#include "NetBrokerClient.h"
#include "HttpClient.h"
#include "HttpServlet.h"
#include "NetLayer.h"

extern const char* g_versionInfo;
auto HOSTNAME = "www.starshatter.com";
constexpr WORD HTTPPORT = 80;

bool NetBrokerClient::broker_available = false;
bool NetBrokerClient::broker_found = false;

bool NetBrokerClient::GameOn(std::string_view name, std::string_view type, std::string_view addr, WORD port, std::string_view password)
{
  bool result = false;

  if (!broker_available)
    return result;

  std::string msg;
  char buffer[8];
  NetAddr broker(HOSTNAME, HTTPPORT);

  if (broker.IPAddr() == 0)
    return result;

  sprintf_s(buffer, "%d", port);

  msg = "GET http://";
  msg += HOSTNAME;
  msg += "/GameNet/GameOn.php?name=";
  msg += HttpRequest::EncodeParam(name);
  msg += "&addr=";
  msg += HttpRequest::EncodeParam(addr);
  msg += "&game=";
  msg += HttpRequest::EncodeParam(type);
  msg += "&vers=";
  msg += HttpRequest::EncodeParam(g_versionInfo);
  msg += "&port=";
  msg += buffer;
  msg += "&pass=";
  msg += HttpRequest::EncodeParam(password);
  msg += " HTTP/1.1\n\n";

  HttpClient client(broker);
  HttpRequest request(msg);
  request.SetHeader("Host", HOSTNAME);

  HttpResponse* response = client.DoRequest(request);

  if (response && response->Status() == 200)
  {
    broker_found = true;
    result = true;
  }
  else
  {
    DebugTrace("NetBrokerClient unable to contact GameNet!\n");

    if (response)
    {
      DebugTrace("  Response Status:  %d\n", response->Status());
      DebugTrace("  Response Content: %s\n", response->Content().data());
    }
    else
      DebugTrace("  No response.\n");

    if (!broker_found)
      broker_available = false;
  }

  delete response;
  return result;
}

bool NetBrokerClient::GameList(std::string_view type, List<NetServerInfo>& server_list)
{
  bool result = false;

  if (!broker_available)
    return result;

  std::string msg;
  NetAddr broker(HOSTNAME, HTTPPORT);

  if (broker.IPAddr() == 0)
    return result;

  msg = "GET http://";
  msg += HOSTNAME;
  msg += "/GameNet/GameList.php?game=";
  msg += HttpRequest::EncodeParam(type);
  msg += "&vers=";
  msg += HttpRequest::EncodeParam(g_versionInfo);
  msg += " HTTP/1.1\n\n";

  HttpClient client(broker);
  HttpRequest request(msg);
  request.SetHeader("Host", HOSTNAME);

  HttpResponse* response = client.DoRequest(request);

  if (response && response->Status() == 200)
  {
    result = true;

    std::string name;
    std::string type;
    std::string addr;
    int port = {};
    std::string pass;
    std::string vers;
    char buff[1024];

    const char* p = response->Content().c_str();

    // skip size
    while (*p && strncmp(p, "name:", 5))
      p++;

    while (*p)
    {
      if (!strncmp(p, "name:", 5))
      {
        p += 5;
        ZeroMemory(buff, sizeof(buff));
        char* d = buff;
        while (*p && *p != '\n')
          *d++ = *p++;
        if (*p)
          p++;

        name = buff;
      }
      else if (!strncmp(p, "addr:", 5))
      {
        p += 5;
        ZeroMemory(buff, sizeof(buff));
        char* d = buff;
        while (*p && *p != '\n')
          *d++ = *p++;
        if (*p)
          p++;

        addr = buff;
      }
      else if (!strncmp(p, "port:", 5))
      {
        p += 5;
        ZeroMemory(buff, sizeof(buff));
        char* d = buff;
        while (*p && *p != '\n')
          *d++ = *p++;
        if (*p)
          p++;

        sscanf_s(buff, "%d", &port);
      }
      else if (!strncmp(p, "pass:", 5))
      {
        p += 5;
        ZeroMemory(buff, sizeof(buff));
        char* d = buff;
        while (*p && *p != '\n')
          *d++ = *p++;
        if (*p)
          p++;

        pass = buff;
      }
      else if (!strncmp(p, "game:", 5))
      {
        p += 5;
        ZeroMemory(buff, sizeof(buff));
        char* d = buff;
        while (*p && *p != '\n')
          *d++ = *p++;
        if (*p)
          p++;

        type = buff;

        if (type.contains("lan"))
          type = "LAN";
        else
          type = "Public";
      }
      else if (!strncmp(p, "vers:", 5))
      {
        p += 5;
        ZeroMemory(buff, sizeof(buff));
        char* d = buff;
        while (*p && *p != '\n')
          *d++ = *p++;
        if (*p)
          p++;

        vers = buff;
      }
      else if (!strncmp(p, "time:", 5))
      {
        while (*p && *p != '\n')
          p++;
        if (*p)
          p++;
      }

      else if (!strncmp(p, "###", 3))
      {
        auto server = NEW NetServerInfo;
        server->name = name;
        server->hostname = addr;
        server->type = type;
        server->addr = NetAddr(addr, port);
        server->port = port;
        server->password = pass;
        server->version = vers;
        server->save = false;

        server_list.append(server);

        while (*p && strncmp(p, "name:", 5))
          p++;
      }
      else
      {
        while (*p && *p != '\n')
          p++;
        if (*p)
          p++;
      }
    }
  }
  else if (!broker_found)
    broker_available = false;

  delete response;
  return result;
}

#include "pch.h"
#include "net_lib.h"
#include "net_socket.h"
#include "GameApp.h"

#include "servertoclient.h"

ServerToClient::ServerToClient(char* _ip)
  : m_socket(nullptr)
{
  strncpy(m_ip, _ip, sizeof(m_ip));
  m_ip[sizeof(m_ip) - 1] = '\0';

  if (!g_context->m_bypassNetworking)
  {
    m_socket = new NetSocket();
    NetRetCode retCode = m_socket->Connect(_ip, 4001);
    DEBUG_ASSERT(retCode == NetOk);
  }

  m_lastKnownSequenceId = -1;
}

char* ServerToClient::GetIP() { return m_ip; }

NetSocket* ServerToClient::GetSocket() { return m_socket; }

#include "pch.h"
#include "ClientPeer.h"
#include "Packet.h"

ClientPeer::ClientPeer(const char* _ip)
{
  DebugTrace("[ClientPeer] Creating peer for IP: {}\n", _ip);
  
  strcpy(m_clientIP, _ip);

  m_clientHost = {};
  m_clientHost.sin_family = AF_INET;
  m_clientHost.sin_port = htons(4001);

  // Use inet_addr for IP address strings, fall back to gethostbyname for hostnames
  unsigned long addr = inet_addr(_ip);
  if (addr != INADDR_NONE)
  {
    m_clientHost.sin_addr.s_addr = addr;
    DebugTrace("[ClientPeer] Address resolved via inet_addr\n");
  }
  else
  {
    HOSTENT* pHostent = gethostbyname(_ip);
    if (pHostent && pHostent->h_addr_list[0])
    {
      m_clientHost.sin_addr.s_addr = *(unsigned long*)pHostent->h_addr_list[0];
      DebugTrace("[ClientPeer] Address resolved via gethostbyname\n");
    }
    else
    {
      DebugTrace("[ClientPeer] Host address resolution failed for {}, using loopback\n", _ip);
      m_clientHost.sin_addr.s_addr = INADDR_LOOPBACK;
    }
  }

  char resolvedIp[16];
  IpToString(m_clientHost.sin_addr, resolvedIp);
  DebugTrace("[ClientPeer] Final address: {}:{}\n", resolvedIp, ntohs(m_clientHost.sin_port));

  m_lastKnownSequenceId = -1;
}

void ClientPeer::SendSYN(bool _startup)
{
  RUDPSync syncPacket;
  syncPacket.m_header.m_sequence = m_seqNumber++;
  syncPacket.m_optionFlag = 1; // Set the CHK flag

  if (!_startup)
  {
    
  }

}

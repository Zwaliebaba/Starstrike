#include "pch.h"
#include "NetLink.h"
#include "NetGram.h"
#include "NetLayer.h"
#include "NetMsg.h"
#include "NetPeer.h"

DWORD WINAPI NetLinkProc(LPVOID link);

constexpr DWORD UDP_HEADER_SIZE = 34;

// client-side ctor
NetLink::NetLink()
  : hnet(nullptr),
    shutdown(false),
    resend_time(300),
    traffic_time(50),
    packets_sent(0),
    packets_recv(0),
    bytes_sent(0),
    bytes_recv(0),
    retries(0),
    drops(0),
    lag(100)
{
  ZeroMemory(lag_samples, sizeof(lag_samples));
  lag_index = 0;

  DWORD thread_id = 0;
  hnet = CreateThread(nullptr, 4096, NetLinkProc, this, 0, &thread_id);
}

// server-side ctor
NetLink::NetLink(NetAddr& a)
  : addr(a),
    hnet(nullptr),
    shutdown(false),
    resend_time(300),
    traffic_time(50),
    packets_sent(0),
    packets_recv(0),
    bytes_sent(0),
    bytes_recv(0),
    retries(0),
    drops(0),
    lag(100)
{
  ZeroMemory(lag_samples, sizeof(lag_samples));
  lag_index = 0;

  sock.bind(addr);
  DWORD thread_id = 0;
  hnet = CreateThread(nullptr, 4096, NetLinkProc, this, 0, &thread_id);
}

NetLink::~NetLink()
{
  if (!shutdown)
    shutdown = true;

  if (hnet)
  {
    WaitForSingleObject(hnet, 2000);
    CloseHandle(hnet);
  }

  send_list.destroy();    // packets waiting to be ack'ed must be destroyed
  recv_list.clear();      // received messages are owned by the peers
  peer_list.destroy();    // but the net link owns the peers!
}

static DWORD base_netid = 1000;

DWORD NetLink::AddPeer(const char* a, WORD p) { return AddPeer(NetAddr(a, p)); }

DWORD NetLink::AddPeer(DWORD a, WORD p) { return AddPeer(NetAddr(a, p)); }

DWORD NetLink::AddPeer(const NetAddr& a)
{
  if (!a.IPAddr())
    return 0;

  std::unique_lock auto_sync(m_mutex);

  NetPeer* peer = FindPeer(a);

  if (!peer)
  {
    peer = NEW NetPeer(a, base_netid++);
    if (peer)
      peer_list.append(peer);
  }

  if (peer)
    return peer->NetID();

  return 0;
}

bool NetLink::SendMessage(DWORD nid, void* d, int l, BYTE f) { return SendMessage(NEW NetMsg(nid, d, l, f)); }

bool NetLink::SendMessage(DWORD nid, BYTE type, const char* text, int len, BYTE f)
{
  return SendMessage(NEW NetMsg(nid, type, text, len, f));
}

bool NetLink::SendMessage(NetMsg* msg)
{
  if (msg)
  {
    if (msg->Type() != NetMsg::INVALID && msg->Type() < NetMsg::RESERVED && msg->NetID())
    {
      NetPeer* p = FindPeer(msg->NetID());
      if (p)
        return p->SendMessage(msg);
    }

    delete msg;
  }

  return false;
}

NetMsg* NetLink::GetMessage(DWORD netid)
{
  NetMsg* msg = nullptr;

  // receive from specific host:
  if (netid)
  {
    NetPeer* p = FindPeer(netid);
    if (p)
    {
      msg = p->GetMessage();

      m_mutex.lock();
      recv_list.remove(msg);
      m_mutex.unlock();
    }
  }

  return msg;
}

NetMsg* NetLink::GetMessage()
{
  NetMsg* msg = nullptr;

  // get first available packet:

  // Double-checked locking:
  if (recv_list.size())
  {
    m_mutex.lock();
    if (recv_list.size())
      msg = recv_list.removeIndex(0);
    m_mutex.unlock();

    if (msg && msg->NetID())
    {
      NetPeer* p = FindPeer(msg->NetID());
      if (p)
      {
        p->GetMessage();  // remove message from peer's list
        // don't do this inside of m_mutex block -
        // it might cause a deadlock
      }
    }
  }

  return msg;
}

void NetLink::Shutdown() { shutdown = true; }

DWORD WINAPI NetLinkProc(LPVOID link)
{
  auto netlink = static_cast<NetLink*>(link);

  if (netlink)
    return netlink->DoSendRecv();

  return static_cast<DWORD>(E_POINTER);
}

DWORD NetLink::DoSendRecv()
{
  while (!shutdown)
  {
    ReadPackets();
    SendPackets();

    // discard reeeeally old peers:
    m_mutex.lock();

    ListIter<NetPeer> iter = peer_list;
    while (!shutdown && ++iter)
    {
      NetPeer* peer = iter.value();

      if ((NetLayer::GetUTC() - peer->LastReceiveTime()) > 300)
        delete iter.removeItem();
    }

    m_mutex.unlock();
    Sleep(traffic_time);
  }

  return 0;
}

void NetLink::ReadPackets()
{
  while (!shutdown && sock.select(NetSock::SELECT_READ) > 0)
  {
    NetGram* gram = RecvNetGram();

    if (gram && gram->IsReliable())
    {
      if (gram->IsAck())
      {
        ProcessAck(gram);
        delete gram;
      }
      else
      {
        AckNetGram(gram);
        QueueNetGram(gram);
      }
    }
    else
      QueueNetGram(gram);
  }
}

void NetLink::SendPackets()
{
  if (shutdown)
    return;

  if (sock.select(NetSock::SELECT_WRITE) > 0)
    DoRetries();

  std::unique_lock auto_sync(m_mutex);

  ListIter<NetPeer> iter = peer_list;
  while (!shutdown && ++iter)
  {
    NetPeer* p = iter.value();
    NetGram* g = nullptr;

    do
    {
      if (sock.select(NetSock::SELECT_WRITE) > 0)
      {
        g = p->ComposeGram();
        if (g)
          SendNetGram(g);
      }
      else
        g = nullptr;
    }
    while (!shutdown && g);
  }
}

void NetLink::SendNetGram(NetGram* gram)
{
  if (gram)
  {
    if (gram->IsReliable())
      send_list.append(gram);

    int err = sock.sendto(gram->Body(), gram->Address());

    if (err < 0)
      err = NetLayer::GetLastError();
    else
    {
      packets_sent += 1;
      bytes_sent += gram->Size() + UDP_HEADER_SIZE;
    }

    if (!gram->IsReliable())
      delete gram;
  }
}

NetGram* NetLink::RecvNetGram()
{
  NetAddr from;
  std::string msg = sock.recvfrom(&from);

  packets_recv += 1;
  bytes_recv += msg.length() + UDP_HEADER_SIZE;

  return NEW NetGram(from, msg);
}

void NetLink::AckNetGram(NetGram* gram)
{
  if (gram)
  {
    NetGram ack = gram->Ack();

    int err = sock.sendto(ack.Body(), gram->Address());
    if (err < 0)
      err = NetLayer::GetLastError();
  }
  else
    DebugTrace("NetLink::AckNetGram( NULL!!! )\n");
}

void NetLink::ProcessAck(NetGram* gram)
{
  if (!shutdown && send_list.size())
  {
    std::unique_lock auto_sync(m_mutex);

    // remove the ack flag:
    gram->ClearAck();

    // find a matching outgoing packet:
    int sent = send_list.index(gram);
    if (sent >= 0)
    {
      NetGram* orig = send_list.removeIndex(sent);
      DWORD time = NetLayer::GetTime();
      DWORD msec = time - orig->SendTime();
      double dlag = 0.75 * lag + 0.25 * msec;

      if (lag_index >= 10)
        lag_index = 0;
      lag_samples[lag_index++] = msec;

      NetPeer* peer = FindPeer(orig->Address());
      if (peer)
        peer->SetLastReceiveTime(NetLayer::GetUTC());

      delete orig;

      lag = static_cast<DWORD>(dlag);

      if (lag > 100)
        resend_time = 3 * lag;
      else
        resend_time = 300;
    }
  }
}

void NetLink::QueueNetGram(NetGram* gram)
{
  if (!shutdown)
  {
    std::unique_lock auto_sync(m_mutex);

    DWORD sequence = 0;
    NetPeer* peer = FindPeer(gram->Address());

    if (peer)
      sequence = peer->Sequence();
    else
    {
      peer = NEW NetPeer(gram->Address(), base_netid++);
      if (peer)
        peer_list.append(peer);
    }

    if (!gram->IsReliable())
    {
      if (gram->Sequence() < sequence)
      {  // discard, too old
        delete gram;
        return;
      }
    }

    // sort this gram into the recv list(s) based on sequence:
    if (peer)
      peer->ReceiveGram(gram, &recv_list);
  }
}

void NetLink::DoRetries()
{
  if (!shutdown)
  {
    std::unique_lock auto_sync(m_mutex);

    if (send_list.size())
    {
      int time = static_cast<int>(NetLayer::GetTime());

      ListIter<NetGram> iter = send_list;
      while (!shutdown && ++iter)
      {
        NetGram* gram = iter.value();

        // still trying ?
        if (gram->Retries() > 0)
        {
          DWORD last_send = gram->SendTime();
          DWORD delta = time - last_send;

          if (delta > resend_time)
          {
            gram->Retry();
            sock.sendto(gram->Body(), gram->Address());
            retries++;
          }
        }

        // oh, give it up:
        else
        {
          iter.removeItem();
          delete gram;
          drops++;
        }
      }
    }
  }
}

NetPeer* NetLink::FindPeer(DWORD netid)
{
  std::unique_lock auto_sync(m_mutex);
  NetPeer* peer = nullptr;

  ListIter<NetPeer> iter = peer_list;
  while (++iter && !peer)
  {
    NetPeer* p = iter.value();

    if (p->NetID() == netid)
      peer = p;
  }

  return peer;
}

NetPeer* NetLink::FindPeer(const NetAddr& a)
{
  std::unique_lock auto_sync(m_mutex);
  NetPeer* peer = nullptr;

  ListIter<NetPeer> iter = peer_list;
  while (++iter && !peer)
  {
    NetPeer* p = iter.value();

    if (p->Address() == a)
      peer = p;
  }

  return peer;
}

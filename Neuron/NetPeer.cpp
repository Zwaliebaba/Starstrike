#include "pch.h"
#include "NetPeer.h"
#include "NetGram.h"
#include "NetLayer.h"
#include "NetMsg.h"

constexpr int MULTIPART_CHUNKSIZE = 232;
constexpr int MULTIPART_HEADER = 16;
constexpr int UDP_HEADER_SIZE = 34;

static NetMsgMultipart multi_part_buffer;
static DWORD multi_msg_sequence = 1;

NetPeer::NetPeer(const NetAddr& a, DWORD id)
  : addr(a),
    sequence(0),
    netid(id),
    pps(0),
    bps(0),
    max_qsize(0),
    status(OK),
    chunk_size(MULTIPART_CHUNKSIZE),
    m_histIndx(0),
    m_sendSize(0),
    m_recvSize(0)
{
  ZeroMemory(hist_time, sizeof(hist_time));
  ZeroMemory(hist_size, sizeof(hist_size));

  last_recv_time = NetLayer::GetUTC();
}

NetPeer::~NetPeer()
{
  m_sendList.destroy();
  m_recvList.destroy();

  m_multiSendList.destroy();
  m_multiRecvList.destroy();
}

bool NetPeer::SendMessage(NetMsg* msg)
{
  if (msg)
  {
    if (max_qsize > 0 && msg->Length() + m_sendSize > max_qsize)
    {
      status = SEND_OVERFLOW;
      delete msg;
      return false;
    }

    // simple message
    if (msg->Length() <= static_cast<int>(chunk_size))
    {
      if (msg->IsPriority())
        m_sendList.insert(msg);
      else
        m_sendList.append(msg);

      m_sendSize += msg->Length();
    }

    // multipart message
    else
    {
      List<NetMsg>* list = &m_sendList;

      if (msg->IsScatter())
        list = &m_multiSendList;

      DWORD nparts = msg->Length() / chunk_size;
      DWORD extra = msg->Length() % chunk_size;

      if (extra > 0)
        nparts++;

      multi_part_buffer.type = NetMsg::MULTIPART;
      multi_part_buffer.msgid = multi_msg_sequence++;
      multi_part_buffer.nparts = nparts;

      size_t header_size = reinterpret_cast<ptrdiff_t>(&multi_part_buffer.payload) - reinterpret_cast<ptrdiff_t>(&multi_part_buffer);

      const BYTE* p = msg->Data();

      for (DWORD i = 0; i < nparts; i++)
      {
        multi_part_buffer.partno = i;
        DWORD part_size = chunk_size;

        if (i == nparts - 1 && extra > 0) // last partial payload
          part_size = extra;

        CopyMemory(multi_part_buffer.payload, p, part_size);
        p += part_size;
        auto part = NEW NetMsg(msg->NetID(), &multi_part_buffer, header_size + part_size, msg->Flags());

        if (part)
        {
          list->append(part);
          m_sendSize += part->Length();
        }
      }
    }

    return true;
  }

  return false;
}

NetMsg* NetPeer::GetMessage()
{
  if (m_recvList.size() > 0)
  {
    NetMsg* msg = m_recvList.removeIndex(0);
    m_recvSize -= msg->Length();
    return msg;
  }

  return nullptr;
}

NetGram* NetPeer::ComposeGram()
{
  NetGram* g = nullptr;

  if ((m_sendList.size() || m_multiSendList.size()) && OKtoSend())
  {
    std::unique_lock auto_sync(m_sync);

    int xmit_size = m_sendSize;
    int nmsg = m_sendList.size();
    int limit = NET_GRAM_MAX_SIZE;
    bool reliable = false;
    bool is_multi = false;

    NetMsg* multi_msg = nullptr;
    List<NetMsg>* list = &m_sendList;

    if (xmit_size > limit)
    {
      xmit_size = 0;
      nmsg = 0;

      if (m_sendList.size() > 0)
      {
        // if there is regular traffic, and multipart traffic
        if (m_multiSendList.size())
        {
          // just send one multipart message in this packet
          multi_msg = m_multiSendList.removeIndex(0);
          limit -= multi_msg->Length();
          reliable = true;
          is_multi = true;
        }

        for (int i = 0; i < m_sendList.size(); i++)
        {
          NetMsg* msg = m_sendList[i];

          if (xmit_size + msg->Length() < limit)
          {
            xmit_size += msg->Length();
            nmsg++;
          }
          else
            break;
        }
      }
      else
      {
        // if there is only multipart traffic,
        // send as many multipart messages as will fit:
        list = &m_multiSendList;
        reliable = true;
        is_multi = true;

        for (int i = 0; i < m_multiSendList.size(); i++)
        {
          NetMsg* msg = m_multiSendList[i];

          if (xmit_size + msg->Length() < limit)
          {
            xmit_size += msg->Length();
            nmsg++;
          }
          else
            break;
        }
      }
    }

    if (xmit_size > 0 && nmsg > 0)
    {
      auto buffer = NEW BYTE[xmit_size];
      BYTE* p = buffer;

      if (multi_msg)
      {
        if (buffer)
        {
          CopyMemory(p, multi_msg->Data(), multi_msg->Length());
          p[1] = multi_msg->Length();
          p += multi_msg->Length();
        }
        delete multi_msg;
      }

      while (nmsg-- && p < buffer + xmit_size)
      {
        NetMsg* msg = list->removeIndex(0);

        if (msg)
        {
          if (msg->IsReliable())
            reliable = true;
          if (buffer)
          {
            CopyMemory(p, msg->Data(), msg->Length());
            p[1] = msg->Length();
            p += msg->Length();
          }
          delete msg;
        }
      }

      if (buffer)
      {
        std::string user_data((const char*)buffer, xmit_size);
        int retries = 0;

        if (reliable)
          retries = 5;

        if (is_multi)
          retries = 10;

        m_sendSize -= xmit_size;

        hist_size[m_histIndx] = xmit_size + UDP_HEADER_SIZE;
        hist_time[m_histIndx] = NetLayer::GetTime();
        m_histIndx++;

        if (m_histIndx >= HIST_SIZE)
          m_histIndx = 0;

        g = NEW NetGram(addr, user_data, retries);
        delete[] buffer;
      }
    }

    // the next msg is too big to fit in a single packet
    else
    {
      NetMsg* m = m_sendList.removeIndex(0);
      m_sendSize -= m->Length();
      delete m;
    }
  }

  return g;
}

bool NetPeer::ReceiveGram(NetGram* g, List<NetMsg>* q)
{
  if (g)
  {
    if (max_qsize > 0 && m_recvSize + g->Size() > max_qsize)
    {
      status = RECV_OVERFLOW;
      delete g;
      return false;
    }

    sequence = g->Sequence();
    m_recvSize += g->Size() - NET_GRAM_HEADER_SIZE;

    // PARSE THE BLOCKS:
    BYTE* p = g->UserData();

    while (p < g->Data() + g->Size())
    {
      BYTE block_type = p[0];
      BYTE block_size = p[1];

      if (!block_type || !block_size)
        break;

      auto msg = NEW NetMsg(netid, p, block_size);

      if (msg)
      {
        if (msg->Type() < NetMsg::RESERVED)
        {
          msg->SetSequence(sequence);

          m_recvList.insertSort(msg);

          if (q)
            q->insertSort(msg);

          p += block_size;
        }

        else if (msg->Type() == NetMsg::MULTIPART)
        {
          m_multiRecvList.insertSort(msg);
          p += block_size;

          CheckMultiRecv(q);
        }
      }
    }

    last_recv_time = NetLayer::GetUTC();

    delete g;
    return true;
  }

  return false;
}

bool NetPeer::OKtoSend() const
{
  if (pps || bps)
  {
    DWORD hist_total = 0;
    DWORD hist_count = 0;
    DWORD now = NetLayer::GetTime();
    DWORD hist_oldest = now;
    DWORD hist_newest = 0;

    for (int i = 0; i < HIST_SIZE; i++)
    {
      if (hist_size[i] > 0)
      {
        hist_total += hist_size[i];
        hist_count++;
      }

      if (hist_time[i] > 0)
      {
        if (hist_time[i] < hist_oldest)
          hist_oldest = hist_time[i];

        if (hist_time[i] > hist_newest)
          hist_newest = hist_time[i];
      }
    }

    if (now - hist_newest < static_cast<DWORD>(pps))
      return false;

    DWORD delta = now - hist_oldest;
    DWORD avg_bps = hist_total / delta;

    if (bps > 0 && avg_bps > static_cast<DWORD>(bps))
      return false;
  }

  return true;
}

struct PacketAssembly
{
  DWORD msgid;
  DWORD netid;
  int nreq;
  int nparts;
  int nbytes;
};

void NetPeer::CheckMultiRecv(List<NetMsg>* q)
{
  constexpr int MAX_SIMULTANEOUS_MULTI_SEQUENCES = 8;

  PacketAssembly assy[MAX_SIMULTANEOUS_MULTI_SEQUENCES];
  ZeroMemory(assy, sizeof(assy));

  size_t header_size = reinterpret_cast<ptrdiff_t>(&multi_part_buffer.payload) - reinterpret_cast<ptrdiff_t>(&multi_part_buffer);

  // Catalog how much of each multipart sequence has been received:
  for (int i = 0; i < m_multiRecvList.size(); i++)
  {
    NetMsg* msg = m_multiRecvList[i];
    auto m = (NetMsgMultipart*)msg->Data();

    for (int n = 0; n < MAX_SIMULTANEOUS_MULTI_SEQUENCES; n++)
    {
      PacketAssembly* a = assy + n;

      if (a->msgid == 0 || (a->msgid == m->msgid && a->netid == msg->NetID()))
      {
        a->msgid = m->msgid;
        a->netid = msg->NetID();
        a->nreq = m->nparts;
        a->nparts += 1;
        a->nbytes += m->len - header_size;
        break;
      }
    }
  }

  for (int n = 0; n < MAX_SIMULTANEOUS_MULTI_SEQUENCES; n++)
  {
    PacketAssembly* a = assy + n;

    // is this sequence complete?
    if (a->msgid && a->nparts == a->nreq)
    {
      auto buffer = new BYTE[a->nbytes];
      BYTE* p = buffer;
      WORD nid = 0;

      ListIter<NetMsg> iter = m_multiRecvList;
      while (++iter)
      {
        netid = iter->NetID();
        auto m = (NetMsgMultipart*)iter->Data();

        // found part of the sequence
        if (m->msgid == a->msgid && netid == a->netid)
        {
          // copy it into the buffer
          CopyMemory(p, m->payload, m->len - header_size);
          p += m->len - header_size;

          delete iter.removeItem();
        }
      }

      auto msg = NEW NetMsg(netid, buffer, a->nbytes, NetMsg::RELIABLE);
      if (msg)
      {
        m_recvList.insertSort(msg);

        if (q)
          q->insertSort(msg);
      }
    }
  }
}

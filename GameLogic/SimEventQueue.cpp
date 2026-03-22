#include "pch.h"
#include "SimEventQueue.h"

SimEventQueue g_simEventQueue;

void SimEventQueue::Push(const SimEvent& _event)
{
  if (m_count < MAX_EVENTS)
  {
    m_events[m_count] = _event;
    ++m_count;
  }
}

const SimEvent& SimEventQueue::Get(int _index) const
{
  DEBUG_ASSERT(_index >= 0 && _index < m_count);
  return m_events[_index];
}

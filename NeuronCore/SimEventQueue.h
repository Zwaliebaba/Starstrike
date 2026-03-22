#pragma once

// ---------------------------------------------------------------------------
// SimEventQueue<TEvent, Capacity>
//
// Fixed-capacity, flat-array queue of deferred simulation side-effects.
// TEvent must be trivially copyable.  No per-frame allocations.
//
// Producers call Push() during simulation ticks.
// The client drains via Count()/Get() after the simulation step, then Clear().
// ---------------------------------------------------------------------------

template<typename TEvent, int Capacity>
class SimEventQueue
{
public:
  void Push(const TEvent& _event)
  {
    DEBUG_ASSERT(m_count < Capacity); // Overflow diagnostic in debug builds
    if (m_count < Capacity)
    {
      m_events[m_count] = _event;
      ++m_count;
    }
  }

  int Count() const { return m_count; }

  const TEvent& Get(int _index) const
  {
    DEBUG_ASSERT(_index >= 0 && _index < m_count);
    return m_events[_index];
  }

  void Clear() { m_count = 0; }

private:
  TEvent m_events[Capacity] = {};
  int    m_count = 0;
};

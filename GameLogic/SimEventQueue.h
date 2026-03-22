#pragma once

#include "SimEvent.h"

// ---------------------------------------------------------------------------
// SimEventQueue — fixed-capacity queue of deferred simulation side-effects.
//
// Usage:
//   Simulation code calls g_simEventQueue.Push() during Advance / Damage.
//   Client main loop calls DrainSimEvents() after Location::Advance().
//   Server ignores the queue (Clear without draining).
//
// The queue is a flat array reset every frame — no per-frame allocations.
// ---------------------------------------------------------------------------

class SimEventQueue
{
public:
  void Push(const SimEvent& _event);
  int  Count() const { return m_count; }
  const SimEvent& Get(int _index) const;
  void Clear() { m_count = 0; }

private:
  static constexpr int MAX_EVENTS = 1024;
  SimEvent m_events[MAX_EVENTS];
  int      m_count = 0;
};

extern SimEventQueue g_simEventQueue;

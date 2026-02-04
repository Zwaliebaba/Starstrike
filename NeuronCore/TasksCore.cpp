#include "pch.h"
#include "TasksCore.h"

Thread::~Thread() { Stop(); }

void Thread::Start(std::function<void()> _callback, const uint32_t _ticksPerSecond)
{
  if (m_running.load(std::memory_order_acquire))
    return;

  m_callback = std::move(_callback);
  SetTickRate(_ticksPerSecond);

  // Create manual-reset event for stop signaling
  m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  if (!m_stopEvent)
    return;

  m_running.store(true, std::memory_order_release);

  // Use Win32 thread pool (works on Server Core)
  m_work = CreateThreadpoolWork(WorkCallback, this, nullptr);
  if (m_work)
    SubmitThreadpoolWork(m_work);
  else
  {
    m_running.store(false, std::memory_order_release);
    CloseHandle(m_stopEvent);
    m_stopEvent = nullptr;
  }
}

void Thread::Stop()
{
  if (!m_running.load(std::memory_order_acquire))
    return;

  m_running.store(false, std::memory_order_release);

  // Signal stop event to wake up sleeping thread
  if (m_stopEvent)
    SetEvent(m_stopEvent);

  // Wait for work item to complete (FALSE = don't cancel pending callbacks)
  // Note: If called from the callback thread, this would deadlock.
  // Use TRUE to cancel if that's a concern.
  if (m_work)
  {
    WaitForThreadpoolWorkCallbacks(m_work, FALSE);
    CloseThreadpoolWork(m_work);
    m_work = nullptr;
  }

  if (m_stopEvent)
  {
    CloseHandle(m_stopEvent);
    m_stopEvent = nullptr;
  }
}

void CALLBACK Thread::WorkCallback(PTP_CALLBACK_INSTANCE /*instance*/, PVOID context, PTP_WORK /*work*/)
{
  auto* self = static_cast<Thread*>(context);
  self->ThreadLoop();
}

void Thread::SetTickRate(uint32_t ticksPerSecond)
{
  // Clamp to 10-100 range
  ticksPerSecond = (std::max)(10u, (std::min)(100u, ticksPerSecond));
  m_tickRate.store(ticksPerSecond, std::memory_order_relaxed);

  // Calculate tick interval in nanoseconds
  m_tickIntervalNs.store(1000000000LL / ticksPerSecond, std::memory_order_relaxed);
}

void Thread::ThreadLoop() const
{
  using clock = std::chrono::high_resolution_clock;

  auto nextTick = clock::now();

  while (m_running.load(std::memory_order_relaxed))
  {
    const auto currentTime = clock::now();
    const auto tickInterval = std::chrono::nanoseconds(m_tickIntervalNs.load(std::memory_order_relaxed));

    if (currentTime >= nextTick)
    {
      if (m_callback)
        m_callback();

      // Schedule next tick
      nextTick += tickInterval;

      // If we've fallen behind, reset to catch up
      if (nextTick < currentTime)
        nextTick = currentTime + tickInterval;
    }
    else
    {
      // Calculate sleep duration in milliseconds
      const auto sleepDuration = nextTick - currentTime;
      const auto sleepMs = std::chrono::duration_cast<std::chrono::milliseconds>(sleepDuration).count();

      if (sleepMs > 1)
      {
        // Use WaitForSingleObject with stop event for interruptible sleep
        // Subtract 1ms to account for timer resolution and allow fine-tuning
        WaitForSingleObject(m_stopEvent, static_cast<DWORD>(sleepMs - 1));
      }
      // Spin for remaining sub-millisecond time for precision
    }
  }
}

#pragma once

#include <functional>
#include <atomic>

namespace Neuron
{
  class Thread : NonCopyable
  {
  public:
    Thread() = default;
    ~Thread();

    // Start the thread with a callback function running at the specified ticks per second (60-100)
    void Start(std::function<void()> _callback, uint32_t _ticksPerSecond = 60);

    // Stop the thread
    void Stop();

    // Check if the thread is running
    bool IsRunning() const { return m_running.load(); }

    // Get the current tick rate
    uint32_t GetTickRate() const { return m_tickRate; }

    // Set a new tick rate (60-100)
    void SetTickRate(uint32_t ticksPerSecond);

  private:
    static void CALLBACK WorkCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work);
    void ThreadLoop() const;

    std::function<void()> m_callback;
    PTP_WORK m_work{ nullptr };
    HANDLE m_stopEvent{ nullptr };
    std::atomic<bool> m_running{ false };
    std::atomic<uint32_t> m_tickRate{ 60 };
    std::atomic<int64_t> m_tickIntervalNs{ 16666667 }; // ~60Hz default in nanoseconds
  };
}

#pragma once

class ASyncLoader
{
  public:
    [[nodiscard]] bool IsValid() const noexcept { return m_isValid; }

    void WaitForLoad() const noexcept { m_isLoading.wait(true); }

    [[nodiscard]] bool IsLoading() const noexcept { return m_isLoading; }

  protected:
    void StartLoading() noexcept
    {
      bool expected = false;
      bool exchanged = m_isLoading.compare_exchange_strong(expected, true);
      DEBUG_ASSERT_TEXT(exchanged, "Already loading");
    }

    void FinishLoading() noexcept
    {
      DEBUG_ASSERT_TEXT(m_isLoading, "Not loading");
      m_isValid = true;
      m_isLoading = false;
      m_isLoading.notify_all();
    }

    void FailLoading() noexcept
    {
      DEBUG_ASSERT_TEXT(m_isLoading, "Not loading");
      m_isLoading = false;
      m_isLoading.notify_all();
    }

    std::atomic_bool m_isValid{false};
    std::atomic_bool m_isLoading{false};
};

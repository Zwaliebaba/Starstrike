#pragma once

class ASyncLoader
{
  public:
    [[nodiscard]] bool IsValid() const { return m_isValid; }

    void WaitForLoad() const
    {
      while (m_isLoading)
        std::this_thread::yield();
    }

    bool IsLoading() const { return m_isLoading; }

  protected:
    void StartLoading()
    {
      DEBUG_ASSERT_TEXT(!m_isLoading, "Already loading");
      m_isLoading = true;
    }

    void FinishLoading()
    {
      DEBUG_ASSERT_TEXT(m_isLoading, "Not loading");
      m_isLoading = false;
      m_isValid = true;
    }

    std::atomic_bool m_isValid{false};
    std::atomic_bool m_isLoading{false};
};

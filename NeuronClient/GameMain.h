#pragma once

#include "ASyncLoader.h"
#include "DeviceNotify.h"
#include "TimerCore.h"

namespace Neuron
{
  class GameMain : public implements<GameMain, Windows::Foundation::IInspectable>, public IDeviceNotify, protected ASyncLoader
  {
    public:
      GameMain() = default;
      ~GameMain() override = default;

      // This function can be used to initialize application state and will run after essential
      // hardware resources are allocated.  Some state that does not depend on these resources
      // should still be initialized in the constructor such as pointers and flags.
      virtual void Startup() = 0;
      virtual void Shutdown() = 0;

      virtual void CreateDeviceDependentResources() {}
      virtual void CreateWindowSizeDependentResources() {}
      virtual void ReleaseDeviceDependentResources() {}
      virtual void ReleaseWindowSizeDependentResources() {}

      // The update method will be invoked once per frame.  Both state updating and scene
      // rendering should be handled by this method.
      // 
      virtual void Update(float _deltaT) = 0;

      // Official rendering pass
      virtual void RenderScene() = 0;

      // Optional UI (overlay) rendering pass.  This is LDR.  The buffer is already cleared.
      virtual void RenderCanvas() {}

      virtual void AddTouch([[maybe_unused]] int _id, [[maybe_unused]] Windows::Foundation::Point _point) {}
      virtual void UpdateTouch([[maybe_unused]] int _id, [[maybe_unused]] Windows::Foundation::Point _point) {}
      virtual void RemoveTouch([[maybe_unused]] int _id) {}

      // --- Activation ---

      virtual void OnActivated()
      {
        m_isActive = true;
        // Ensure the cursor is hidden — loop handles ShowCursor reference-count drift.
        while (ShowCursor(FALSE) >= 0) {}
        Timer::Core::ResetElapsedTime();
      }

      virtual void OnDeactivated()
      {
        m_isActive = false;
        // Ensure the cursor is visible — loop handles ShowCursor reference-count drift.
        while (ShowCursor(TRUE) < 0) {}
        ClipCursor(nullptr);
      }

      // --- Suspend / Resume ---

      virtual void OnSuspending()
      {
        m_isSuspended = true;
        Graphics::Core::Get().WaitForGpu();
      }

      virtual void OnResuming()
      {
        m_isSuspended = false;
        Timer::Core::ResetElapsedTime();
      }

      // --- Window events ---

      virtual void OnWindowMoved()
      {
        const auto r = Graphics::Core::Get().GetOutputSize();
        Graphics::Core::Get().WindowSizeChanged(r.Width, r.Height);
      }

      virtual void OnDisplayChange() { Graphics::Core::Get().UpdateColorSpace(); }

      virtual void OnWindowSizeChanged(int width, int height)
      {
        if (Graphics::Core::Get().WindowSizeChanged(width, height))
        {
          ReleaseWindowSizeDependentResources();
          CreateWindowSizeDependentResources();
        }
      }

      // --- IDeviceNotify ---

      void OnDeviceLost() override
      {
        ReleaseWindowSizeDependentResources();
        ReleaseDeviceDependentResources();
      }

      void OnDeviceRestored() override
      {
        CreateDeviceDependentResources();
        CreateWindowSizeDependentResources();
      }

      // --- State accessors ---

      bool IsActive() const { return m_isActive; }
      bool IsSuspended() const { return m_isSuspended; }

    protected:
      bool m_isActive = true;
      bool m_isSuspended = false;
  };
}

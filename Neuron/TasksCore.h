#pragma once

namespace Neuron::Tasks
{
  using namespace Windows::Foundation;

  class Core
  {
    public:
      static void Startup();
      static IAsyncAction Shutdown();
      static Windows::System::DispatcherQueue GetDispatcherQueue() { return sm_dispatcherQueue; }

      static std::pair<Windows::System::DispatcherQueueTimer, event_token> PlanTask(TimeSpan _recurance, bool _repeat,
                           const TypedEventHandler<Windows::System::DispatcherQueueTimer, IInspectable>& _handler);
      template<typename Func> bool PlanTask(Func&& f)
      {
        return sm_dispatcherQueue.TryEnqueue(std::forward<Func>(f));
      }

    protected:
      static inline Windows::System::DispatcherQueueController sm_dispatcherController{nullptr};
      static inline Windows::System::DispatcherQueue sm_dispatcherQueue{nullptr};
  };
}

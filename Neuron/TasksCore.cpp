#include "pch.h"
#include "TasksCore.h"

void Tasks::Core::Startup()
{
  sm_dispatcherController = Windows::System::DispatcherQueueController::CreateOnDedicatedThread();
  sm_dispatcherQueue = sm_dispatcherController.DispatcherQueue();
}

Windows::Foundation::IAsyncAction Tasks::Core::Shutdown()
{
  co_await resume_background();

  co_await sm_dispatcherController.ShutdownQueueAsync();
}

std::pair<Windows::System::DispatcherQueueTimer, event_token> Tasks::Core::PlanTask(TimeSpan _recurance, bool _repeat,
                           const TypedEventHandler<Windows::System::DispatcherQueueTimer, IInspectable>& _handler)
{
  auto timer = sm_dispatcherQueue.CreateTimer();
  timer.Interval(_recurance);
  timer.IsRepeating(_repeat);
  auto token = timer.Tick(_handler);
  timer.Start();
  return std::make_pair(timer, token);
}

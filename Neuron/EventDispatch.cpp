#include "pch.h"
#include "EventDispatch.h"
#include "Mouse.h"

int GetKeyPlus(int& key, int& shift);

EventDispatch* EventDispatch::dispatcher = nullptr;

EventDispatch::EventDispatch()
  : mouse_x(0),
    mouse_y(0),
    mouse_l(0),
    mouse_r(0),
    capture(nullptr),
    current(nullptr),
    focus(nullptr),
    click_tgt(nullptr) {}

EventDispatch::~EventDispatch() {}

void EventDispatch::Create() { dispatcher = NEW EventDispatch; }

void EventDispatch::Close()
{
  delete dispatcher;
  dispatcher = nullptr;
}

void EventDispatch::Dispatch()
{
  int ml = Mouse::LButton();
  int mr = Mouse::RButton();
  int mx = Mouse::X();
  int my = Mouse::Y();
  int mw = Mouse::Wheel();

  EventTarget* mouse_tgt = capture;
  EventTarget* key_tgt = focus;
  EventTarget* do_click = nullptr;

  if (!mouse_tgt)
  {
    ListIter<EventTarget> iter = clients;
    while (++iter)
    {
      EventTarget* test = iter.value();
      if (test->IsFormActive())
      {
        if (test->TargetRect().Contains(mx, my))
          mouse_tgt = test;

        if (test->HasFocus())
          key_tgt = test;
      }
    }
  }

  // Mouse Events:

  if (mouse_tgt != current)
  {
    if (current && current->IsEnabled() && current->IsVisible())
      current->OnMouseExit(mx, my);

    current = mouse_tgt;

    if (current && current->IsEnabled() && current->IsVisible())
      current->OnMouseEnter(mx, my);
  }

  if (mouse_tgt && mouse_tgt->IsEnabled())
  {
    if (mx != mouse_x || my != mouse_y)
      mouse_tgt->OnMouseMove(mx, my);

    if (mw != 0)
      mouse_tgt->OnMouseWheel(mw);

    if (ml != mouse_l)
    {
      if (ml)
      {
        mouse_tgt->OnLButtonDown(mx, my);
        click_tgt = mouse_tgt;
      }
      else
      {
        mouse_tgt->OnLButtonUp(mx, my);

        if (click_tgt == mouse_tgt)
        {
          if (click_tgt->TargetRect().Contains(mx, my))
            do_click = click_tgt;
          click_tgt = nullptr;
        }
      }
    }

    if (mr != mouse_r)
    {
      if (mr)
        mouse_tgt->OnRButtonDown(mx, my);
      else
        mouse_tgt->OnRButtonUp(mx, my);
    }
  }

  mouse_l = ml;
  mouse_r = mr;
  mouse_x = mx;
  mouse_y = my;

  // Keyboard Events:

  if (click_tgt && click_tgt != key_tgt)
  {
    if (key_tgt)
      key_tgt->KillFocus();
    key_tgt = click_tgt;

    if (key_tgt != focus)
    {
      if (focus)
        focus->KillFocus();

      if (key_tgt && key_tgt->IsEnabled() && key_tgt->IsVisible())
        focus = key_tgt;
      else
        key_tgt = nullptr;

      if (focus)
        focus->SetFocus();
    }
  }

  if (key_tgt && key_tgt->IsEnabled())
  {
    int key = 0;
    int shift = 0;

    while (GetKeyPlus(key, shift))
    {
      if (key == VK_ESCAPE)
      {
        key_tgt->KillFocus();
        focus = nullptr;
        break;
      }
      if (key == VK_TAB && focus)
      {
        int key_index = clients.index(focus) + 1;

        if (shift & 1)
          key_index -= 2;

        if (key_index >= clients.size())
          key_index = 0;
        else if (key_index < 0)
          key_index = clients.size() - 1;

        if (focus)
          focus->KillFocus();
        focus = clients[key_index];
        if (focus)
          focus->SetFocus();

        break;
      }

      key_tgt->OnKeyDown(key, shift);
    }
  }

  if (do_click)
    do_click->OnClick();
}

void EventDispatch::MouseEnter(EventTarget* mouse_tgt)
{
  if (mouse_tgt != current)
  {
    if (current)
      current->OnMouseExit(0, 0);
    current = mouse_tgt;
    if (current)
      current->OnMouseEnter(0, 0);
  }
}

void EventDispatch::Register(EventTarget* tgt)
{
  if (!clients.contains(tgt))
    clients.append(tgt);
}

void EventDispatch::Unregister(EventTarget* tgt)
{
  clients.remove(tgt);

  if (capture == tgt)
    capture = nullptr;
  if (current == tgt)
    current = nullptr;
  if (focus == tgt)
    focus = nullptr;
  if (click_tgt == tgt)
    click_tgt = nullptr;
}

EventTarget* EventDispatch::GetCapture() { return capture; }

int EventDispatch::CaptureMouse(EventTarget* tgt)
{
  if (tgt)
  {
    capture = tgt;
    return 1;
  }

  return 0;
}

int EventDispatch::ReleaseMouse(EventTarget* tgt)
{
  if (capture == tgt)
  {
    capture = nullptr;
    return 1;
  }

  return 0;
}

EventTarget* EventDispatch::GetFocus() { return focus; }

void EventDispatch::SetFocus(EventTarget* tgt)
{
  if (focus != tgt)
  {
    if (focus)
      focus->KillFocus();

    focus = tgt;

    if (focus && focus->IsEnabled() && focus->IsVisible())
      focus->SetFocus();
  }
}

void EventDispatch::KillFocus(EventTarget* tgt)
{
  if (focus && focus == tgt)
  {
    focus = nullptr;
    tgt->KillFocus();
  }
}

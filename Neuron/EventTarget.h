#ifndef EventTarget_h
#define EventTarget_h

#include "Geometry.h"

class EventTarget
{
  public:
    
    virtual ~EventTarget() = default;

    int operator ==(const EventTarget& t) const { return this == &t; }

    virtual int OnMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y) { return 0; }
    virtual int OnLButtonDown([[maybe_unused]] int x, [[maybe_unused]] int y) { return 0; }
    virtual int OnLButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y) { return 0; }
    virtual int OnClick() { return 0; }
    virtual int OnSelect() { return 0; }
    virtual int OnRButtonDown([[maybe_unused]] int x, [[maybe_unused]] int y) { return 0; }
    virtual int OnRButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y) { return 0; }
    virtual int OnMouseEnter([[maybe_unused]] int x, [[maybe_unused]] int y) { return 0; }
    virtual int OnMouseExit([[maybe_unused]] int x, [[maybe_unused]] int y) { return 0; }
    virtual int OnMouseWheel([[maybe_unused]] int wheel) { return 0; }

    virtual int OnKeyDown(int vk, int flags) { return 0; }

    virtual void SetFocus() {}
    virtual void KillFocus() {}
    virtual bool HasFocus() const { return false; }

    virtual bool IsEnabled() const { return true; }
    virtual bool IsVisible() const { return true; }
    virtual bool IsFormActive() const { return true; }

    virtual Rect TargetRect() const { return {}; }

    virtual std::string GetDescription() const { return "EventTarget"; }
};

#endif EventTarget_h

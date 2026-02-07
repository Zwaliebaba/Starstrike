#include "pch.h"
#include "FadeView.h"
#include "Color.h"
#include "Game.h"

FadeView::FadeView(Window* c, double in, double out, double hold)
  : View(c),
    fade_in(in * 1000),
    fade_out(out * 1000),
    hold_time(hold * 1000),
    time(0),
    step_time(0),
    fast(1) { state = StateStart; }

FadeView::~FadeView() {}

void FadeView::FadeIn(double in) { fade_in = in * 1000; }
void FadeView::FadeOut(double out) { fade_out = out * 1000; }
void FadeView::FastFade(int fade_fast) { fast = fade_fast; }

void FadeView::StopHold()
{
  hold_time = 0;
}

void FadeView::Refresh()
{
  double msec = 0;

  if (state == StateStart)
    time = Game::RealTime();
  else if (state != StateDone)
  {
    double new_time = Game::RealTime();
    msec = new_time - time;
    time = new_time;
  }

  switch (state)
  {
    case StateStart:
      if (fade_in)
      {
        Color::SetFade(0);
      }

      step_time = 0;
      state = State2;
      break;

    case State2:
      if (fade_in)
      {
        Color::SetFade(0);
      }

      step_time = 0;
      state = StateIn;
      break;

    case StateIn:
      if (step_time < fade_in)
      {
        double fade = step_time / fade_in;
        Color::SetFade(fade);
        step_time += msec;
      }
      else
      {
        Color::SetFade(1);
        step_time = 0;
        state = StateHold;
      }
      break;

    case StateHold:
      if (step_time < hold_time)
      {
        step_time += msec;
      }
      else
      {
        step_time = 0;
        state = StateOut;
      }
      break;

    case StateOut:
      if (fade_out > 0)
      {
        if (step_time < fade_out)
        {
          double fade = 1 - step_time / fade_out;
          Color::SetFade(fade);
          step_time += msec;
        }
        else
        {
          Color::SetFade(0);
          step_time = 0;
          state = StateDone;
        }
      }
      else
      {
        Color::SetFade(1);
        step_time = 0;
        state = StateDone;
      }
      break;

    default: case StateDone:
      break;
  }
}

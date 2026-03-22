#pragma once

class Spirit;

// Render companion for Spirit — billboard quad with team colour.
// Called directly from Location::RenderSpirits() (spirits have their own
// container, not m_effects).
class SpiritRenderer
{
  public:
    static void RenderSpirit(const Spirit& _spirit, float _predictionTime);
};

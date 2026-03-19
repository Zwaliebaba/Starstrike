#pragma once

#include "LegacyVector3.h"

// Extracted from Entity::BeginRenderShadow / RenderShadow / EndRenderShadow
// (entity.cpp).  Provides ground-projected shadow quads used by entity render
// companions across Waves 1-3.
//
// Usage:
//   ShadowRenderer::Begin();
//   ShadowRenderer::Render(entityPos, shadowSize);
//   ShadowRenderer::End();
class ShadowRenderer
{
  public:
    static void Begin();
    static void Render(const LegacyVector3& _pos, float _size);
    static void End();
};

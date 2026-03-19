#pragma once

#include "BuildingRenderer.h"

// Thin adapter that wraps the existing TreeRenderer DX12 pipeline behind
// the standard BuildingRenderer interface.  Registered in
// BuildingRenderRegistry so location.cpp can dispatch Tree rendering via
// the registry instead of a manual type check.
//
// The adapter receives const Building& from the registry, performs a
// static_cast to const Tree& (safe because the registry guarantees the
// type -> renderer mapping), and delegates to TreeRenderer::DrawTree().
class TreeBuildingRenderer : public BuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};

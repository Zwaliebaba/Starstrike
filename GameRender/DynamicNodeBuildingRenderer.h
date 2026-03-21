#pragma once

#include "DefaultBuildingRenderer.h"

// Renderer companion for DynamicNode.
// Render() adds smooth shading around the default shape render, and applies
// an editor-mode normal-map alignment fix.
// RenderAlphas() and RenderPorts() delegate to DefaultBuildingRenderer.
class DynamicNodeBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
};

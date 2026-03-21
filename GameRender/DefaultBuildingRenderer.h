#pragma once

#include "BuildingRenderer.h"

// Default building renderer that implements the base Building::Render /
// RenderAlphas / RenderLights / RenderPorts pattern.  Register it for any
// building type that does not override these methods.
//
// Other building-type renderers may inherit from this class and override
// only the methods they specialise, reusing the base rendering logic for
// the rest.
class DefaultBuildingRenderer : public BuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderLights(const Building& _building) override;
    void RenderPorts(const Building& _building) override;
};

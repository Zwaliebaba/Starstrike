#pragma once

struct BuildingRenderContext
{
    float predictionTime;
};

class Building;

// Base class for all building render companions.
// Subclasses implement the rendering logic for a specific building type.
// Registered in BuildingRenderRegistry for type-indexed dispatch.
class BuildingRenderer
{
  public:
    virtual ~BuildingRenderer() = default;
    virtual void Render(const Building& _building, const BuildingRenderContext& _ctx) = 0;
    virtual void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) {}
    virtual void RenderLights(const Building& _building) {}
    virtual void RenderPorts(const Building& _building) {}
};

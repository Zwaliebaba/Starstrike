#pragma once

struct EntityRenderContext
{
    float predictionTime;
    float highDetailFactor;   // 0.0-1.0, computed from camera distance
};

class Entity;

// Base class for all entity render companions.
// Subclasses implement the rendering logic for a specific entity type.
// Registered in EntityRenderRegistry for type-indexed dispatch.
class EntityRenderer
{
  public:
    virtual ~EntityRenderer() = default;
    virtual void Render(const Entity& _entity, const EntityRenderContext& _ctx) = 0;
    virtual void RenderAlphas([[maybe_unused]] const Entity& _entity, [[maybe_unused]] const EntityRenderContext& _ctx) {}
};

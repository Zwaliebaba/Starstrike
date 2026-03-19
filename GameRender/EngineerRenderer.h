#pragma once

#include "EntityRenderer.h"

// Render companion for Engineer entities.
// Renders the hovering shape, shadow, and reprogramming laser beam.
class EngineerRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;

  private:
    void RenderShape(const class Engineer& _engineer, float _predictionTime);
    void RenderLaser(const class Engineer& _engineer, float _predictionTime);
};

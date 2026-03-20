#pragma once

#include "EntityRenderer.h"

class LegacyVector3;

class SoulDestroyerRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;

  private:
    void RenderShapes(const Entity& _entity, float _predictionTime);
    void RenderSpirit(const LegacyVector3& _pos, float _alpha);
};

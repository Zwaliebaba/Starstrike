#pragma once

#include "EntityRenderer.h"
#include "LegacyVector3.h"

// Render companion for Officer entities.
// Renders shield spirits, flag banner, and flag-marker starburst.
class OfficerRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;

  private:
    void RenderShield(const class Officer& _officer, float _predictionTime);
    void RenderSpirit(const LegacyVector3& _pos);
    void RenderFlag(const class Officer& _officer, float _predictionTime);
};

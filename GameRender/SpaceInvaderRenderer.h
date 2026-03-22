#pragma once

#include "EntityRenderer.h"

class SpaceInvaderRenderer : public EntityRenderer
{
public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;
};

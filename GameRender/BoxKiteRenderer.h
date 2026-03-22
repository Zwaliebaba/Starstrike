#pragma once

#include "WorldObjectRenderer.h"

class BoxKiteRenderer : public WorldObjectRenderer
{
public:
    void Render(const WorldObject& _object, const WorldObjectRenderContext& _ctx) override;
};

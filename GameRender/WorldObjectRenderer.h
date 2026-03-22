#pragma once

struct WorldObjectRenderContext
{
    float predictionTime;
};

class WorldObject;

// Base class for all WorldObject (effect) render companions.
// Subclasses implement the rendering logic for a specific effect type
// (Snow, weapons, Flag, SpaceInvader, BoxKite, etc.).
// Registered in WorldObjectRenderRegistry for type-indexed dispatch.
class WorldObjectRenderer
{
  public:
    virtual ~WorldObjectRenderer() = default;
    virtual void Render(const WorldObject& _object, const WorldObjectRenderContext& _ctx) = 0;
};

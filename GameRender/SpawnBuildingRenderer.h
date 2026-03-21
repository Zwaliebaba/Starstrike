#pragma once

#include "DefaultBuildingRenderer.h"

class LegacyVector3;

// Renderer for the SpawnBuilding family.  Inherits the default opaque pass
// and adds spirit-link line rendering + in-transit spirit dots in the alpha
// pass.  Register directly for SpawnLink; SpawnPoint and MasterSpawnPoint
// inherit and specialise further.
class SpawnBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;

  private:
    static void RenderSpirit(const LegacyVector3& _pos);
};

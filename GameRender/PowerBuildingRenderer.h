#pragma once

#include "DefaultBuildingRenderer.h"

// Renderer for the PowerBuilding family.  Inherits the default opaque pass
// (shape render) and adds power-line quad + surge starbursts in the alpha
// pass.  Register directly for Pylon and PylonStart; Generator and
// SolarPanel inherit and specialise further.
class PowerBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};

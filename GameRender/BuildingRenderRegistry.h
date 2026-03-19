#pragma once

#include "building.h"

class BuildingRenderer;

// Type-indexed table of building render companions.
// Replaces polymorphic Building::Render() / RenderAlphas() dispatch.
// Call Get() with the building type to retrieve the companion,
// or nullptr if no companion is registered (fallback to legacy path).
class BuildingRenderRegistry
{
  public:
    void Register(int _buildingType, BuildingRenderer* _renderer);
    BuildingRenderer* Get(int _buildingType) const;

  private:
    BuildingRenderer* m_renderers[Building::NumBuildingTypes] = {};
};

extern BuildingRenderRegistry g_buildingRenderRegistry;

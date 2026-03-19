#include "pch.h"

#include "BuildingRenderRegistry.h"
#include "BuildingRenderer.h"

BuildingRenderRegistry g_buildingRenderRegistry;

void BuildingRenderRegistry::Register(int _buildingType, BuildingRenderer* _renderer)
{
    DEBUG_ASSERT(_buildingType >= 0 && _buildingType < Building::NumBuildingTypes);
    m_renderers[_buildingType] = _renderer;
}

BuildingRenderer* BuildingRenderRegistry::Get(int _buildingType) const
{
    if (_buildingType < 0 || _buildingType >= Building::NumBuildingTypes)
        return nullptr;
    return m_renderers[_buildingType];
}

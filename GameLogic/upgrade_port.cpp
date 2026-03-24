#include "pch.h"
#include "resource.h"
#include "upgrade_port.h"
#include "GameContext.h"
#include "global_world.h"

UpgradePort::UpgradePort()
  : Building()
{
  m_type = TypeUpgradePort;
  SetShape(Resource::GetShapeStatic("upgrade_port.shp"));
}

PrimaryUpgradePort::PrimaryUpgradePort()
  : Building(),
    m_controlTowersOwned(0)
{
  m_type = TypePrimaryUpgradePort;
  SetShape(Resource::GetShapeStatic("primaryupgradeport.shp"));
}

void PrimaryUpgradePort::ReprogramComplete()
{
  m_controlTowersOwned++;

  if (m_controlTowersOwned == 3)
  {
    GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_context->m_locationId);
    if (gb)
    {
      gb->m_online = true;
      g_context->m_globalWorld->EvaluateEvents();
    }
  }
}

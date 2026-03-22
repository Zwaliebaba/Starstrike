#include "pch.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "math_utils.h"
#include "input.h"
#include "globals.h"
#include "GameApp.h"
#include "location.h"
#include "team.h"
#include "entity_grid.h"
#include "camera.h"
#include "user_input.h"
#include "main.h"
#include "global_world.h"
#include "level_file.h"
#include "gunturret.h"
#include "GameSimEventQueue.h"

GunTurret::GunTurret()
  : Building(),
    m_turret(nullptr),
    m_aiTargetCreated(false),
    m_fireTimer(0.0f),
    m_nextBarrel(0),
    m_retargetTimer(0.0f),
    m_targetCreated(false),
    m_health(60.0f),
    m_ownershipTimer(0.0f)
{
  SetShape(g_app->m_resource->GetShapeStatic("battlecannonbase.shp"));
  m_type = TypeGunTurret;

  m_turret = g_app->m_resource->GetShapeStatic("battlecannonturret.shp");
  m_barrel = g_app->m_resource->GetShapeStatic("battlecannonbarrel.shp");

  m_barrelMount = m_turret->GetMarkerData("MarkerBarrel");

  for (int i = 0; i < GUNTURRET_NUMBARRELS; ++i)
  {
    char name[64];
    snprintf(name, sizeof(name), "MarkerBarrelEnd0%d", i + 1);
    m_barrelEnd[i] = m_barrel->GetMarkerData(name);
  }

  for (int i = 0; i < GUNTURRET_NUMSTATUSMARKERS; ++i)
  {
    char name[64];
    snprintf(name, sizeof(name), "MarkerStatus0%d", i + 1);
    m_statusMarkers[i] = m_shape->GetMarkerData(name);
  }
}

void GunTurret::Initialise(Building* _template)
{
  _template->m_up = g_app->m_location->m_landscape.m_normalMap->GetValue(_template->m_pos.x, _template->m_pos.z);
  LegacyVector3 right(1, 0, 0);
  _template->m_front = right ^ _template->m_up;

  Building::Initialise(_template);

  m_turretFront = m_front;
  m_barrelUp = g_upVector;
  m_centerPos = m_pos;
  m_radius = 30.0f;

  SearchForRandomPos();
}

void GunTurret::SetDetail(int _detail)
{
  Building::SetDetail(_detail);

  m_centerPos = m_pos;
  m_radius = 30.0f;
}

void GunTurret::Damage(float _damage)
{
  m_health += _damage;

  if (m_health <= 0.0f)
    ExplodeBody();
}

void GunTurret::ExplodeBody()
{
  Matrix34 turretPos(m_turretFront, m_up, m_pos);
  LegacyVector3 barrelPos = m_turret->GetMarkerWorldMatrix(m_barrelMount, turretPos).pos;
  LegacyVector3 barrelFront = m_turret->GetMarkerWorldMatrix(m_barrelMount, turretPos).f;
  Matrix34 barrelMat(barrelFront, m_up, barrelPos);

  g_simEventQueue.Push(SimEvent::MakeExplosion(m_turret, turretPos, 1.0f));
  g_simEventQueue.Push(SimEvent::MakeExplosion(m_barrel, barrelMat, 1.0f));
}

bool GunTurret::SearchForTargets()
{
  WorldObjectId previousTarget = m_targetId;
  m_targetId.SetInvalid();

  if (m_id.GetTeamId() != 255)
  {
    m_targetId = g_app->m_location->m_entityGrid->GetBestEnemy(m_pos.x, m_pos.z, GUNTURRET_MINRANGE, GUNTURRET_MAXRANGE, m_id.GetTeamId());
  }

  Entity* entity = g_app->m_location->GetEntity(m_targetId);

  if (entity && m_targetId != previousTarget)
    g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "TargetSighted"));

  return (m_targetId.IsValid());
}

void GunTurret::SearchForRandomPos()
{
  float angle = syncfrand(2.0f * M_PI);
  float distance = 200.0f + syncfrand(200.0f);
  float height = syncfrand(100.0f);

  m_target = m_pos + LegacyVector3(cosf(angle) * distance, height, sinf(angle) * distance);
}

void GunTurret::RecalculateOwnership()
{
  int teamCount[NUM_TEAMS];
  memset(teamCount, 0, NUM_TEAMS * sizeof(int));

  for (int i = 0; i < GetNumPorts(); ++i)
  {
    WorldObjectId id = GetPortOccupant(i);
    if (id.IsValid())
      teamCount[id.GetTeamId()]++;
  }

  int winningTeam = -1;
  for (int i = 0; i < NUM_TEAMS; ++i)
  {
    if (teamCount[i] > 0 && winningTeam == -1)
      winningTeam = i;
    else if (winningTeam != -1 && teamCount[i] > 0 && teamCount[i] > teamCount[winningTeam])
      winningTeam = i;
  }

  if (winningTeam == -1)
    SetTeamId(255);
  else if (winningTeam == 0)
  {
    // Green own the building, but set to yellow so the player
    // can control the building
    SetTeamId(2);
  }
  else
  {
    // Red own the building
    SetTeamId(winningTeam);
  }
}

void GunTurret::PrimaryFire()
{
  bool fired = false;

  for (int i = 0; i < 2; ++i)
  {
    if (GetPortOccupant(m_nextBarrel).IsValid())
    {
      Matrix34 turretMat(m_turretFront, g_upVector, m_pos);
      LegacyVector3 barrelMountPos = m_turret->GetMarkerWorldMatrix(m_barrelMount, turretMat).pos;
      LegacyVector3 barrelMountFront = m_turret->GetMarkerWorldMatrix(m_barrelMount, turretMat).f;
      LegacyVector3 barrelRight = barrelMountFront ^ m_barrelUp;
      barrelMountFront = m_barrelUp ^ barrelRight;
      Matrix34 barrelMountMat(barrelMountFront, m_barrelUp, barrelMountPos);
      Matrix34 barrelMat = m_barrel->GetMarkerWorldMatrix(m_barrelEnd[m_nextBarrel], barrelMountMat);

      LegacyVector3 shellPos = barrelMat.pos;
      LegacyVector3 shellVel = barrelMountFront.SetLength(500.0f);

      g_app->m_location->FireTurretShell(shellPos, shellVel);
      fired = true;
    }

    ++m_nextBarrel;
    if (m_nextBarrel >= GUNTURRET_NUMBARRELS)
      m_nextBarrel = 0;
  }

  if (fired)
    g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "FireShell"));
}

bool GunTurret::Advance()
{
  if (m_health <= 0.0f)
    return true;

  //
  // Create an AI target on top of us,
  // to get Darwinians to fight hard for control

  if (!m_aiTargetCreated)
  {
    Building* aiTarget = CreateBuilding(TypeAITarget);
    aiTarget->m_pos = m_pos;
    aiTarget->m_front = m_front;
    g_app->m_location->m_buildings.PutData(aiTarget);
    int uniqueId = g_app->m_globalWorld->GenerateBuildingId();
    aiTarget->m_id.SetUniqueId(uniqueId);
    m_aiTargetCreated = true;
  }

  //
  // Create a GunTurret target that darwinians can run from

  if (!m_targetCreated)
  {
    auto target = new GunTurretTarget(m_id.GetUniqueId());
    int index = g_app->m_location->m_effects.PutData(target);
    target->m_id.Set(m_id.GetTeamId(), UNIT_EFFECTS, index, -1);
    target->m_id.GenerateUniqueId();
    m_targetCreated = true;
  }

  //
  // Time to recalculate ownership?

  m_ownershipTimer -= SERVER_ADVANCE_PERIOD;
  if (m_ownershipTimer <= 0.0f)
  {
    RecalculateOwnership();
    m_ownershipTimer = GUNTURRET_OWNERSHIPTIMER;
  }

  Team* team = g_app->m_location->GetMyTeam();
  bool underPlayerControl = (team && team->m_currentBuildingId == m_id.GetUniqueId());

  //
  // Look for a new target

  bool primaryFire = false;
  bool secondaryFire = false;

  if (underPlayerControl)
  {
    if (m_id.GetTeamId() != 2)
    {
      // Player has lost control of the building
      team->SelectUnit(-1, -1, -1);
      g_app->m_camera->RequestMode(Camera::ModeFreeMovement);
      return Building::Advance();
    }
    m_target = g_app->m_userInput->GetMousePos3d();

    primaryFire = g_inputManager->controlEvent(ControlUnitPrimaryFireTarget) || g_inputManager->controlEvent(
      ControlUnitStartSecondaryFireDirected);
    secondaryFire = g_inputManager->controlEvent(ControlUnitSecondaryFireTarget);
  }
  else
  {
    m_retargetTimer -= SERVER_ADVANCE_PERIOD;
    if (m_retargetTimer <= 0.0f)
    {
      bool targetFound = SearchForTargets();
      if (!targetFound && m_id.GetTeamId() != 255)
        SearchForRandomPos();
      m_retargetTimer = GUNTURRET_RETARGETTIMER;
    }

    if (m_targetId.IsValid())
    {
      // Check our target is still alive
      Entity* target = g_app->m_location->GetEntity(m_targetId);
      if (!target)
      {
        m_targetId.SetInvalid();
        return Building::Advance();
      }
      m_target = target->m_pos;
      m_target += LegacyVector3(sinf(g_gameTime * 3) * 20, fabs(sinf(g_gameTime * 2) * 10), cosf(g_gameTime * 3) * 20);

      LegacyVector3 targetFront = (m_target - m_pos);
      targetFront.HorizontalAndNormalise();
      float angle = acosf(targetFront * m_turretFront);

      primaryFire = (angle < 0.25f);
    }
  }

  float distance = (m_target - m_pos).Mag();
  if (distance < 75.0f)
    m_target = m_pos + (m_target - m_pos).SetLength(75.0f);

  //
  // Face our target

  Matrix34 turretPos(m_turretFront, m_up, m_pos);
  LegacyVector3 barrelPos = m_turret->GetMarkerWorldMatrix(m_barrelMount, turretPos).pos;
  LegacyVector3 toTarget = m_target - barrelPos;
  toTarget.HorizontalAndNormalise();
  float angle = acosf(toTarget * m_turretFront);

  LegacyVector3 rotation = m_turretFront ^ toTarget;
  rotation.SetLength(angle * 0.2f);
  m_turretFront.RotateAround(rotation);
  m_turretFront.Normalise();

  toTarget = (m_target - barrelPos).Normalise();
  LegacyVector3 barrelRight = toTarget ^ g_upVector;
  LegacyVector3 targetBarrelUp = toTarget ^ barrelRight;
  float factor = SERVER_ADVANCE_PERIOD * 2.0f;
  m_barrelUp = m_barrelUp * (1.0f - factor) + targetBarrelUp * factor;

  //
  // Open fire

  if (primaryFire)
    PrimaryFire();

  return Building::Advance();
}

LegacyVector3 GunTurret::GetTarget() { return m_target; }

bool GunTurret::IsInView()
{
  Team* team = g_app->m_location->GetMyTeam();
  bool underPlayerControl = (team && team->m_currentBuildingId == m_id.GetUniqueId());

  if (underPlayerControl)
    return true;
  return Building::IsInView();
}

bool GunTurret::DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, LegacyVector3* _pos,
                           LegacyVector3* norm)
{
  if (g_app->m_editing)
    return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
  return Building::DoesRayHit(_rayStart, _rayDir, _rayLen, _pos, norm);
}

GunTurretTarget::GunTurretTarget(int _buildingId)
  : WorldObject(),
    m_buildingId(_buildingId) { m_type = EffectGunTurretTarget; }

bool GunTurretTarget::Advance()
{
  auto turret = static_cast<GunTurret*>(g_app->m_location->GetBuilding(m_buildingId));
  if (!turret || turret->m_type != Building::TypeGunTurret)
    return true;

  if (turret->GetNumPortsOccupied() > 0)
    m_pos = turret->GetTarget();
  else
    m_pos.Zero();

  return false;
}

void GunTurretTarget::Render(float _time)
{
  //RenderSphere( m_pos, 10.0f );
}

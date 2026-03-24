#include "pch.h"
#include "math_utils.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "GameContext.h"
#include "camera.h"
#include "entity_grid.h"
#include "globals.h"
#include "location.h"
#include "main.h"
#include "GameSimEventQueue.h"
#include "team.h"
#include "global_world.h"
#include "unit.h"
#include "radardish.h"

RadarDish::RadarDish()
  : Teleport(),
    m_receiverId(-1),
    m_range(0.0f),
    m_signal(0.0f),
    m_newlyCreated(true),
    m_horizontallyAligned(true),
    m_verticallyAligned(true),
    m_movementSoundsPlaying(false)
{
  m_type = TypeRadarDish;
  m_target.Zero();
  m_front.Set(0, 0, 1);
  m_sendPeriod = RADARDISH_TRANSPORTPERIOD;

  ShapeStatic* master = Resource::GetShapeStatic("radardish.shp");
  SetShape(master);
  m_shapeInstance = ShapeInstance(master);
  m_dishIndex = master->GetFragmentIndex("Dish");
  m_upperMountIndex = master->GetFragmentIndex("UpperMount");
  m_focusMarker = m_shape->GetMarkerData("MarkerFocus");
}

RadarDish::~RadarDish()
{
  // ShapeInstance is a value member — destroyed automatically.
  // Building::m_shape is non-owning; Resource owns all masters.
}

void RadarDish::SetDetail(int _detail)
{
  Teleport::SetDetail(_detail);

  Matrix34 rootMat(m_front, m_up, m_pos);
  Matrix34 worldMat = m_shapeInstance.GetMarkerWorldMatrix(m_entrance, rootMat);
  m_entrancePos = worldMat.pos;
  m_entranceFront = worldMat.f;
}

bool RadarDish::GetEntrance(LegacyVector3& _pos, LegacyVector3& _front)
{
  _pos = m_entrancePos;
  _front = m_entranceFront;
  return true;
}

bool RadarDish::Advance()
{
  //
  // If this is our first advance, align to our dish
  // if we saved a target dish previously
  if (m_newlyCreated)
  {
    GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_context->m_locationId);
    if (gb)
    {
      Building* targetBuilding = g_context->m_location->GetBuilding(gb->m_link);
      if (targetBuilding && targetBuilding->m_type == TypeRadarDish)
      {
        auto dish = static_cast<RadarDish*>(targetBuilding);
        Aim(dish->GetStartPoint());
      }
    }
    m_newlyCreated = false;
  }

  if (m_target.MagSquared() < 0.001f)
    return Building::Advance();

  Matrix34 rootMat(m_front, g_upVector, m_pos);
  Matrix34 worldMat = m_shapeInstance.GetMarkerWorldMatrix(m_focusMarker, rootMat);
  LegacyVector3 dishPos = worldMat.pos;
  LegacyVector3 dishFront = worldMat.f;

  //
  // Rotate slowly to face our target (horiz)

  if (!m_horizontallyAligned)
  {
    LegacyVector3 targetFront = (m_target - dishPos);
    LegacyVector3 currentFront = worldMat.f;
    targetFront.HorizontalAndNormalise();
    currentFront.HorizontalAndNormalise();
    LegacyVector3 rotationAxis = currentFront ^ targetFront;
    rotationAxis.y /= 4.0f;
    if (fabsf(rotationAxis.y) > M_PI / 3000.0f)
      m_shapeInstance.GetState(m_upperMountIndex).angVel.y = signf(rotationAxis.y) * sqrtf(fabsf(rotationAxis.y));
    else
    {
      m_horizontallyAligned = true;
      m_shapeInstance.GetState(m_upperMountIndex).angVel.y = 0.0f;
    }
  }

  //
  // Rotate slowly to face our target (vert)

  m_shapeInstance.GetState(m_dishIndex).angVel.Zero();

  if (m_horizontallyAligned && !m_verticallyAligned)
  {
    LegacyVector3 targetFront = (m_target - dishPos);
    targetFront.Normalise();
    constexpr float maxAngle = 0.3f;
    constexpr float minAngle = -0.2f;
    if (targetFront.y > maxAngle)
      targetFront.y = maxAngle;
    if (targetFront.y < minAngle)
      targetFront.y = minAngle;
    targetFront.Normalise();
    float amount = worldMat.u * targetFront;
    auto right = m_shapeInstance.GetState(m_dishIndex).transform.RightF3();
    m_shapeInstance.GetState(m_dishIndex).angVel = LegacyVector3(right.x, right.y, right.z) * amount;

    m_verticallyAligned = (m_shapeInstance.GetState(m_dishIndex).angVel.Mag() < 0.001f);
  }

  FragmentState& upperState = m_shapeInstance.GetState(m_upperMountIndex);
  upperState.transform.RotateAround(DirectX::XMVectorSet(
    upperState.angVel.x * SERVER_ADVANCE_PERIOD,
    upperState.angVel.y * SERVER_ADVANCE_PERIOD,
    upperState.angVel.z * SERVER_ADVANCE_PERIOD, 0.0f));
  FragmentState& dishState = m_shapeInstance.GetState(m_dishIndex);
  dishState.transform.RotateAround(DirectX::XMVectorSet(
    dishState.angVel.x * SERVER_ADVANCE_PERIOD,
    dishState.angVel.y * SERVER_ADVANCE_PERIOD,
    dishState.angVel.z * SERVER_ADVANCE_PERIOD, 0.0f));

  if (m_movementSoundsPlaying && m_horizontallyAligned && dishState.angVel.Mag() < 0.05f)
  {
    g_simEventQueue.Push(SimEvent::MakeSoundStop(m_id));
    g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "EndRotation"));
    m_movementSoundsPlaying = false;
  }

  //
  // Are there any receiving Radar dishes?

  m_range = 99999.9f;
  bool found = false;

  bool previouslyAligned = (m_receiverId != -1);

  for (int i = 0; i < g_context->m_location->m_buildings.Size(); ++i)
  {
    // Skip empty slots
    if (!g_context->m_location->m_buildings.ValidIndex(i))
      continue;

    // Filter out non radar dish buildings
    Building* building = g_context->m_location->m_buildings.GetData(i);
    if (building->m_type != TypeRadarDish)
      continue;

    // Don't compare against ourself
    if (building == this)
      continue;

    // Does our "ray" hit their dish
    auto otherDish = static_cast<RadarDish*>(building);
    bool hit = RaySphereIntersection(dishPos, dishFront, otherDish->m_centerPos, otherDish->m_radius, 1e9);
    if (hit)
    {
      // Make sure we aren't hitting the back of the receiving dish
      LegacyVector3 theirFront = otherDish->GetDishFront(0.0f);
      float dotProd = dishFront * theirFront;
      if (dotProd < 0.0f)
      {
        if (g_context->m_location->IsVisible(dishPos, otherDish->GetDishPos(0.0f)))
        {
          float newRange = (otherDish->GetDishPos(0.0f) - dishPos).Mag();
          if (newRange < m_range)
          {
            m_receiverId = otherDish->m_id.GetUniqueId();
            m_range = newRange;
            m_signal = 1.0f;
            found = true;
          }
        }
      }
    }
  }

  if (previouslyAligned && !found)
  {
    m_range = 0.0f;
    m_signal = 0.0f;
    m_receiverId = -1;
    g_simEventQueue.Push(SimEvent::MakeSoundStop(m_id, "RadarDish ConnectionEstablished"));
    g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "ConnectionLost"));

    GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_context->m_locationId);
    if (gb)
      gb->m_link = -1;
  }

  if (!previouslyAligned && found)
  {
    g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "ConnectionEstablished"));

    GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_context->m_locationId);
    if (gb)
      gb->m_link = m_receiverId;
  }

  return Teleport::Advance();
}

LegacyVector3 RadarDish::GetDishPos([[maybe_unused]] float _predictionTime)
{
  Matrix34 rootMat(m_front, g_upVector, m_pos);
  Matrix34 worldMat = m_shapeInstance.GetMarkerWorldMatrix(m_focusMarker, rootMat);
  return worldMat.pos;
}

LegacyVector3 RadarDish::GetDishFront(float _predictionTime)
{
  if (m_receiverId != -1)
  {
    auto receiver = static_cast<RadarDish*>(g_context->m_location->GetBuilding(m_receiverId));
    if (receiver)
    {
      LegacyVector3 ourDishPos = GetDishPos(_predictionTime);
      LegacyVector3 receiverDishPos = receiver->GetDishPos(_predictionTime);
      return (receiverDishPos - ourDishPos).Normalise();
    }
  }

  Matrix34 rootMat(m_front, g_upVector, m_pos);
  Matrix34 worldMat = m_shapeInstance.GetMarkerWorldMatrix(m_focusMarker, rootMat);
  return worldMat.f;
}

void RadarDish::Aim(LegacyVector3 _worldPos)
{
  m_target = _worldPos;

  m_horizontallyAligned = false;
  m_verticallyAligned = false;

  if (m_movementSoundsPlaying)
    g_simEventQueue.Push(SimEvent::MakeSoundStop(m_id, "RadarDish BeginRotation"));

  g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "BeginRotation"));
  m_movementSoundsPlaying = true;
}

bool RadarDish::Connected() { return (m_receiverId != -1 && m_signal > 0.0f); }

int RadarDish::GetConnectedDishId() { return m_receiverId; }

bool RadarDish::ReadyToSend() { return (Connected() && Teleport::ReadyToSend()); }

LegacyVector3 RadarDish::GetStartPoint() { return GetDishPos(0.0f); }

LegacyVector3 RadarDish::GetEndPoint() { return GetDishPos(0.0f) + GetDishFront(0.0f) * m_range; }

bool RadarDish::GetExit(LegacyVector3& _pos, LegacyVector3& _front)
{
  auto receiver = static_cast<RadarDish*>(g_context->m_location->GetBuilding(m_receiverId));
  if (receiver)
  {
    Matrix34 rootMat(receiver->m_front, g_upVector, receiver->m_pos);
    Matrix34 worldMat = receiver->m_shapeInstance.GetMarkerWorldMatrix(receiver->m_entrance, rootMat);
    _pos = worldMat.pos;
    _front = worldMat.f;
    return true;
  }
  return false;
}

bool RadarDish::UpdateEntityInTransit(Entity* _entity)
{
  LegacyVector3 dishPos = GetDishPos(0.0f);
  LegacyVector3 dishFront = GetDishFront(0.0f);
  LegacyVector3 targetPos = dishPos + dishFront * m_range;
  LegacyVector3 ourOffset = (targetPos - _entity->m_pos);

  WorldObjectId id(_entity->m_id);

  _entity->m_vel = dishFront * RADARDISH_TRANSPORTSPEED;
  _entity->m_pos += _entity->m_vel * SERVER_ADVANCE_PERIOD;
  _entity->m_onGround = false;
  _entity->m_enabled = false;

  if (_entity->m_id.GetUnitId() != -1)
  {
    Unit* unit = g_context->m_location->GetUnit(_entity->m_id);
    unit->UpdateEntityPosition(_entity->m_pos, _entity->m_radius);
  }

  float distTravelled = (_entity->m_pos - dishPos).Mag();

  if (m_signal == 0.0f)
  {
    // Shit - we lost the carrier signal, so we die
    _entity->ChangeHealth(-500);
    _entity->m_enabled = true;
    _entity->m_vel += LegacyVector3(syncsfrand(10.0f), syncfrand(10.0f), syncsfrand(10.0f));

    g_context->m_location->m_entityGrid->AddObject(id, _entity->m_pos.x, _entity->m_pos.z, _entity->m_radius);
    return true;
  }
  if (distTravelled >= m_range)
  {
    // We are there
    LegacyVector3 exitPos, exitFront;
    GetExit(exitPos, exitFront);
    _entity->m_pos = exitPos;
    _entity->m_front = exitFront;
    _entity->m_enabled = true;
    _entity->m_onGround = true;
    _entity->m_vel.Zero();

    g_context->m_location->m_entityGrid->AddObject(id, _entity->m_pos.x, _entity->m_pos.z, _entity->m_radius);

    g_simEventQueue.Push(SimEvent::MakeSoundEntity(_entity->m_id, "ExitTeleport"));
    return true;
  }

  return false;
}

bool RadarDish::DoesSphereHit(const LegacyVector3& _pos, float _radius)
{
  // This method is overridden for Radar Dish in order to expand the number
  // of cells the Radar Dish is placed into.  We were having problems where
  // entities couldn't get into the radar dish because its door was right on
  // the edge of an obstruction grid cell, so the entity didn't see the
  // building at all

  SpherePackage sphere(_pos, _radius * 1.5f);
  Matrix34 transform(m_front, m_up, m_pos);
  return m_shapeInstance.SphereHit(&sphere, transform);
}

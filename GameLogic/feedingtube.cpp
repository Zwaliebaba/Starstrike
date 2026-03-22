#include "pch.h"
#include "feedingtube.h"
#include "GameApp.h"
#include "camera.h"
#include "file_writer.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "text_stream_readers.h"

FeedingTube::FeedingTube()
  : m_receiverId(-1),
    m_range(0.0f),
    m_signal(0.0f)
{
  m_type = TypeFeedingTube;
  //m_front.Set(0,0,1);

  SetShape(g_app->m_resource->GetShapeStatic("feedingtube.shp"));
  m_focusMarker = m_shape->GetMarkerData("MarkerFocus");
}

// *** Initialise
void FeedingTube::Initialise(Building* _template)
{
  Building::Initialise(_template);
  DEBUG_ASSERT(_template->m_type == Building::TypeFeedingTube);
  m_receiverId = static_cast<FeedingTube*>(_template)->m_receiverId;
}

bool FeedingTube::Advance()
{
  Matrix34 rootMat(m_front, g_upVector, m_pos);
  Matrix34 worldMat = m_shape->GetMarkerWorldMatrix(m_focusMarker, rootMat);
  LegacyVector3 dishPos = worldMat.pos;

  auto ft = static_cast<FeedingTube*>(g_app->m_location->GetBuilding(m_receiverId));
  if (ft && ft->m_type == TypeFeedingTube)
    m_range = (ft->GetDishPos(0.0f) - dishPos).Mag();
  else
    m_range = 0.0f;

  return Building::Advance();
}

LegacyVector3 FeedingTube::GetDishPos(float _predictionTime)
{
  Matrix34 rootMat(m_front, g_upVector, m_pos);
  Matrix34 worldMat = m_shape->GetMarkerWorldMatrix(m_focusMarker, rootMat);
  return worldMat.pos;
}

LegacyVector3 FeedingTube::GetDishFront(float _predictionTime)
{
  if (m_receiverId != -1)
  {
    auto receiver = static_cast<FeedingTube*>(g_app->m_location->GetBuilding(m_receiverId));
    if (receiver)
    {
      LegacyVector3 ourDishPos = GetDishPos(_predictionTime);
      LegacyVector3 receiverDishPos = receiver->GetDishPos(_predictionTime);
      return (receiverDishPos - ourDishPos).Normalise();
    }
  }

  Matrix34 rootMat(m_front, g_upVector, m_pos);
  Matrix34 worldMat = m_shape->GetMarkerWorldMatrix(m_focusMarker, rootMat);
  return worldMat.f;
}

LegacyVector3 FeedingTube::GetForwardsClippingDir(float _predictionTime, FeedingTube* _sender)
{
  if (_sender == nullptr)
    return GetDishFront(_predictionTime);

  LegacyVector3 senderDishFront = _sender->GetDishFront(_predictionTime);

  LegacyVector3 dishFront = GetDishFront(_predictionTime);

  // Make the two dishFronts point at each other.

  LegacyVector3 SR = GetDishPos(_predictionTime) - _sender->GetDishPos(_predictionTime);
  SR.Normalise();

  if (SR * senderDishFront < 0)
    senderDishFront *= -1;

  if (SR * dishFront > 0)
    dishFront *= -1;

  LegacyVector3 combinedDirection = -senderDishFront + dishFront;

  if (combinedDirection.MagSquared() < 0.5f)
    return -dishFront;

  return combinedDirection.Normalise();
}

LegacyVector3 FeedingTube::GetStartPoint() { return GetDishPos(0.0f); }

LegacyVector3 FeedingTube::GetEndPoint() { return GetDishPos(0.0f) + (GetDishFront(0.0f) * m_range); }

bool FeedingTube::DoesSphereHit(const LegacyVector3& _pos, float _radius)
{
  // This method is overridden for Radar Dish in order to expand the number
  // of cells the Radar Dish is placed into.  We were having problems where
  // entities couldn't get into the radar dish because its door was right on
  // the edge of an obstruction grid cell, so the entity didn't see the
  // building at all

  SpherePackage sphere(_pos, _radius * 1.5f);
  Matrix34 transform(m_front, m_up, m_pos);
  return m_shape->SphereHit(&sphere, transform);
}

int FeedingTube::GetBuildingLink() { return m_receiverId; }

void FeedingTube::SetBuildingLink(int _buildingId)
{
  Building* b = g_app->m_location->GetBuilding(_buildingId);
  if (b && b->m_type == TypeFeedingTube)
  {
    m_receiverId = _buildingId;

    auto p = static_cast<FeedingTube*>(b);
  }
}

// *** Read
void FeedingTube::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_receiverId = atoi(_in->GetNextToken());
}

void FeedingTube::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%-8d", m_receiverId);
}

bool FeedingTube::IsInView()
{
  LegacyVector3 startPoint = GetStartPoint();
  LegacyVector3 endPoint = GetEndPoint();

  LegacyVector3 midPoint = (startPoint + endPoint) / 2.0f;
  float radius = (startPoint - endPoint).Mag() / 2.0f;
  radius += m_radius;

  if (g_app->m_camera->SphereInViewFrustum(midPoint, radius))
    return true;

  return Building::IsInView();
}

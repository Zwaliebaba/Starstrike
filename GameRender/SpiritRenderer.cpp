#include "pch.h"

#include "SpiritRenderer.h"
#include "QuadBatcher.h"
#include "spirit.h"
#include "camera.h"
#include "GameApp.h"
#include "location.h"
#include "team.h"


void SpiritRenderer::RenderSpirit(const Spirit& _spirit, float _predictionTime)
{
  int innerAlpha = 255;
  int outerAlpha = 100;
  float spiritInnerSize = 1.0f;
  float spiritOuterSize = 3.0f;

  if (_spirit.m_state == Spirit::StateBirth)
  {
    float fractionBorn = 1.2f - (_spirit.m_timeSync / 6.0f);
    if (fractionBorn > 1.0f)
      fractionBorn = 1.0f;
    spiritInnerSize *= fractionBorn;
    spiritOuterSize *= fractionBorn;
  }

  if (_spirit.m_state == Spirit::StateDeath)
  {
    float fractionAlive = _spirit.m_timeSync / 180.0f;
    innerAlpha *= fractionAlive;
    outerAlpha *= fractionAlive;
    spiritInnerSize *= fractionAlive;
    spiritOuterSize *= fractionAlive;
  }

  RGBAColour colour;
  if (_spirit.m_teamId != 255)
    colour = g_context->m_location->m_teams[_spirit.m_teamId].m_colour;
  else
    colour.Set(255, 255, 255);

  float predTime = _predictionTime - SERVER_ADVANCE_PERIOD;

  LegacyVector3 predictedPos = _spirit.m_pos + predTime * _spirit.m_vel;
  predictedPos += predTime * _spirit.m_hover;

  LegacyVector3 camUp = g_context->m_camera->GetUp();
  LegacyVector3 camRight = g_context->m_camera->GetRight();

  auto& batcher = QuadBatcher::Get();

  // Inner quad
  {
    float size = spiritInnerSize;
    uint32_t color = QuadBatcher::PackColorBGRA(colour.r, colour.g, colour.b,
                                                static_cast<unsigned char>(innerAlpha));
    LegacyVector3 p0 = predictedPos - camUp * size;
    LegacyVector3 p1 = predictedPos + camRight * size;
    LegacyVector3 p2 = predictedPos + camUp * size;
    LegacyVector3 p3 = predictedPos - camRight * size;
    batcher.Emit(
      QuadBatcher::MakeVertex(p0.x, p0.y, p0.z, color),
      QuadBatcher::MakeVertex(p1.x, p1.y, p1.z, color),
      QuadBatcher::MakeVertex(p2.x, p2.y, p2.z, color),
      QuadBatcher::MakeVertex(p3.x, p3.y, p3.z, color)
    );
  }

  // Outer quad
  {
    float size = spiritOuterSize;
    uint32_t color = QuadBatcher::PackColorBGRA(colour.r, colour.g, colour.b,
                                                static_cast<unsigned char>(outerAlpha));
    LegacyVector3 p0 = predictedPos - camUp * size;
    LegacyVector3 p1 = predictedPos + camRight * size;
    LegacyVector3 p2 = predictedPos + camUp * size;
    LegacyVector3 p3 = predictedPos - camRight * size;
    batcher.Emit(
      QuadBatcher::MakeVertex(p0.x, p0.y, p0.z, color),
      QuadBatcher::MakeVertex(p1.x, p1.y, p1.z, color),
      QuadBatcher::MakeVertex(p2.x, p2.y, p2.z, color),
      QuadBatcher::MakeVertex(p3.x, p3.y, p3.z, color)
    );
  }
}

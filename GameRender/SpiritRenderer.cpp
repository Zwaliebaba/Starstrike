#include "pch.h"

#include "SpiritRenderer.h"
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
    colour = g_app->m_location->m_teams[_spirit.m_teamId].m_colour;
  else
    colour.Set(255, 255, 255);

  float predTime = _predictionTime - SERVER_ADVANCE_PERIOD;

  LegacyVector3 predictedPos = _spirit.m_pos + predTime * _spirit.m_vel;
  predictedPos += predTime * _spirit.m_hover;

  float size = spiritInnerSize;
  glColor4ub(colour.r, colour.g, colour.b, innerAlpha);

  glBegin(GL_QUADS);
  glVertex3fv((predictedPos - g_app->m_camera->GetUp() * size).GetData());
  glVertex3fv((predictedPos + g_app->m_camera->GetRight() * size).GetData());
  glVertex3fv((predictedPos + g_app->m_camera->GetUp() * size).GetData());
  glVertex3fv((predictedPos - g_app->m_camera->GetRight() * size).GetData());
  glEnd();

  size = spiritOuterSize;
  glColor4ub(colour.r, colour.g, colour.b, outerAlpha);
  glBegin(GL_QUADS);
  glVertex3fv((predictedPos - g_app->m_camera->GetUp() * size).GetData());
  glVertex3fv((predictedPos + g_app->m_camera->GetRight() * size).GetData());
  glVertex3fv((predictedPos + g_app->m_camera->GetUp() * size).GetData());
  glVertex3fv((predictedPos - g_app->m_camera->GetRight() * size).GetData());
  glEnd();
}

#include "pch.h"

#include "MiscEffectRenderer.h"
#include "officer.h"
#include "spam.h"
#include "souldestroyer.h"
#include "camera.h"
#include "resource.h"
#include "GameApp.h"
#include "location.h"
#include "team.h"


void MiscEffectRenderer::Render(const WorldObject& _object, const WorldObjectRenderContext& _ctx)
{
  switch (_object.m_type)
  {
  case WorldObject::EffectOfficerOrders:
    RenderOfficerOrders(static_cast<const OfficerOrders&>(_object), _ctx.predictionTime);
    break;

  case WorldObject::EffectSpamInfection:
    RenderSpamInfection(static_cast<const SpamInfection&>(_object), _ctx.predictionTime);
    break;

  case WorldObject::EffectZombie:
    RenderZombie(static_cast<const Zombie&>(_object), _ctx.predictionTime);
    break;

  default:
    // EffectGunTurretTarget — intentionally empty (debug-only RenderSphere was removed)
    break;
  }
}

// ---------------------------------------------------------------------------
// OfficerOrders — billboard starburst at waypoint position
// ---------------------------------------------------------------------------

void MiscEffectRenderer::RenderOfficerOrders(const OfficerOrders& _orders, float _predictionTime)
{
  float size = 15.0f;
  LegacyVector3 predictedPos = _orders.m_pos + _orders.m_vel * _predictionTime;

  float alpha = 0.7f;
  if (_orders.m_arrivedTimer >= 0.0f)
  {
    float fraction = 1.0f - (_orders.m_arrivedTimer + _predictionTime);
    fraction = std::max(fraction, 0.0f);
    fraction = std::min(fraction, 1.0f);
    size *= fraction;
  }

  LegacyVector3 camUp = g_context->m_camera->GetUp() * size;
  LegacyVector3 camRight = g_context->m_camera->GetRight() * size;

  glColor4f(1.0f, 0.3f, 1.0f, alpha);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));
  glDisable(GL_DEPTH_TEST);

  glBegin(GL_QUADS);
  glTexCoord2i(0, 0);
  glVertex3fv((predictedPos - camRight + camUp).GetData());
  glTexCoord2i(1, 0);
  glVertex3fv((predictedPos + camRight + camUp).GetData());
  glTexCoord2i(1, 1);
  glVertex3fv((predictedPos + camRight - camUp).GetData());
  glTexCoord2i(0, 1);
  glVertex3fv((predictedPos - camRight - camUp).GetData());
  glEnd();

  glEnable(GL_DEPTH_TEST);
  glDisable(GL_TEXTURE_2D);
}

// ---------------------------------------------------------------------------
// SpamInfection — glowing trail with position history
// ---------------------------------------------------------------------------

void MiscEffectRenderer::RenderSpamInfection(const SpamInfection& _infection, float _predictionTime)
{
  LegacyVector3 predictedPos = _infection.m_pos + _infection.m_vel * _predictionTime;

  glShadeModel(GL_SMOOTH);
  glEnable(GL_BLEND);
  int maxLength = SPAMINFECTION_TAILLENGTH * (_infection.m_life / SPAMINFECTION_LIFE);
  maxLength = std::max(maxLength, 2);
  maxLength = std::min(maxLength, _infection.m_positionHistory.Size());

  LegacyVector3 camPos = g_context->m_camera->GetPos();
  int numRepeats = 4;

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/laser.bmp"));
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  for (int j = 0; j < numRepeats; ++j)
  {
    float size = 0.3f * numRepeats;
    for (int i = 1; i < maxLength; ++i)
    {
      float alpha = 1.0f - i / static_cast<float>(maxLength);
      alpha *= 0.75f;
      glColor4f(1.0f, 0.1f, 0.1f, alpha);
      LegacyVector3 thisPos = *_infection.m_positionHistory.GetPointer(i);
      LegacyVector3 lastPos = *_infection.m_positionHistory.GetPointer(i - 1);
      LegacyVector3 rightAngle = (thisPos - lastPos) ^ (camPos - thisPos);
      rightAngle.SetLength(size);
      glBegin(GL_QUADS);
      glTexCoord2f(0.2f, 0.0f);
      glVertex3fv((thisPos - rightAngle).GetData());
      glTexCoord2f(0.2f, 1.0f);
      glVertex3fv((thisPos + rightAngle).GetData());
      glTexCoord2f(0.8f, 1.0f);
      glVertex3fv((lastPos + rightAngle).GetData());
      glTexCoord2f(0.8f, 0.0f);
      glVertex3fv((lastPos - rightAngle).GetData());
      glEnd();
    }
    if (_infection.m_positionHistory.Size() > 0)
    {
      glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
      LegacyVector3 lastPos = *_infection.m_positionHistory.GetPointer(0);
      LegacyVector3 thisPos = predictedPos;
      LegacyVector3 rightAngle = (thisPos - lastPos) ^ (camPos - thisPos);
      rightAngle.SetLength(size);
      glBegin(GL_QUADS);
      glTexCoord2f(0.2f, 0.0f);
      glVertex3fv((thisPos - rightAngle).GetData());
      glTexCoord2f(0.2f, 1.0f);
      glVertex3fv((thisPos + rightAngle).GetData());
      glTexCoord2f(0.8f, 1.0f);
      glVertex3fv((lastPos + rightAngle).GetData());
      glTexCoord2f(0.8f, 0.0f);
      glVertex3fv((lastPos - rightAngle).GetData());
      glEnd();
    }
  }

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_FLAT);
}

// ---------------------------------------------------------------------------
// Zombie — ghost sprite billboard with inner/outer alpha layers
// ---------------------------------------------------------------------------

void MiscEffectRenderer::RenderZombie(const Zombie& _zombie, float _predictionTime)
{
  LegacyVector3 predictedPos = _zombie.m_pos + _zombie.m_vel * _predictionTime;
  predictedPos += _zombie.m_hover * _predictionTime;

  LegacyVector3 predictedFront = _zombie.m_front;
  LegacyVector3 predictedUp = _zombie.m_up;
  LegacyVector3 predictedRight = predictedFront ^ predictedUp;

  float size = 5.0f;

  float alpha = 1.0f - (_zombie.m_life / 10.0f);
  alpha = std::max(0.1f, alpha);
  alpha = std::min(0.7f, alpha);

  float outerAlpha = (0.7f - alpha) * 0.1f;

  float timeRemaining = 600.0f - _zombie.m_life;
  if (timeRemaining < 100.0f)
  {
    alpha *= timeRemaining * 0.01f;
    outerAlpha *= timeRemaining * 0.01f;
  }

  glDisable(GL_CULL_FACE);
  glColor4f(0.9f, 0.9f, 1.0f, alpha);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("sprites/ghost.bmp"));

  glBegin(GL_QUADS);
  glTexCoord2i(0, 0);
  glVertex3fv((predictedPos - size * predictedRight - size * predictedUp).GetData());
  glTexCoord2i(1, 0);
  glVertex3fv((predictedPos + size * predictedRight - size * predictedUp).GetData());
  glTexCoord2i(1, 1);
  glVertex3fv((predictedPos + size * predictedRight + size * predictedUp).GetData());
  glTexCoord2i(0, 1);
  glVertex3fv((predictedPos - size * predictedRight + size * predictedUp).GetData());
  glEnd();

  size *= 2.0f;
  glColor4f(0.9f, 0.9f, 1.0f, outerAlpha);
  glBegin(GL_QUADS);
  glTexCoord2i(0, 0);
  glVertex3fv((predictedPos - size * predictedRight - size * predictedUp).GetData());
  glTexCoord2i(1, 0);
  glVertex3fv((predictedPos + size * predictedRight - size * predictedUp).GetData());
  glTexCoord2i(1, 1);
  glVertex3fv((predictedPos + size * predictedRight + size * predictedUp).GetData());
  glTexCoord2i(0, 1);
  glVertex3fv((predictedPos - size * predictedRight + size * predictedUp).GetData());
  glEnd();

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_CULL_FACE);
}

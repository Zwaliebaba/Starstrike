#include "pch.h"

#include "WeaponRenderer.h"
#include "weapons.h"
#include "camera.h"
#include "math_utils.h"
#include "particle_system.h"
#include "renderer.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "hi_res_time.h"
#include "GameApp.h"
#include "location.h"
#include "team.h"


void WeaponRenderer::Render(const WorldObject& _object, const WorldObjectRenderContext& _ctx)
{
  switch (_object.m_type)
  {
  case WorldObject::EffectThrowableGrenade:
  case WorldObject::EffectThrowableAirstrikeMarker:
  case WorldObject::EffectThrowableAirstrikeBomb:
  case WorldObject::EffectThrowableControllerGrenade:
    RenderThrowable(static_cast<const ThrowableWeapon&>(_object), _ctx.predictionTime);
    break;

  case WorldObject::EffectRocket:
    RenderRocket(static_cast<const Rocket&>(_object), _ctx.predictionTime);
    break;

  case WorldObject::EffectShockwave:
    RenderShockwave(static_cast<const Shockwave&>(_object), _ctx.predictionTime);
    break;

  case WorldObject::EffectMuzzleFlash:
    RenderMuzzleFlash(static_cast<const MuzzleFlash&>(_object), _ctx.predictionTime);
    break;

  case WorldObject::EffectGunTurretShell:
    RenderTurretShell(static_cast<const TurretShell&>(_object), _ctx.predictionTime);
    break;

  default:
    break;
  }
}


// ============================================================================
//  ThrowableWeapon — shape + starburst flash billboard
// ============================================================================

void WeaponRenderer::RenderThrowable(const ThrowableWeapon& _weapon, float _predictionTime)
{
  _predictionTime -= SERVER_ADVANCE_PERIOD;

  LegacyVector3 predictedPos = _weapon.m_pos + _weapon.m_vel * _predictionTime;
  Matrix34 transform(_weapon.m_front, _weapon.m_up, predictedPos);

  g_context->m_renderer->SetObjectLighting();
  glEnable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_COLOR_MATERIAL);
  glDisable(GL_BLEND);

  _weapon.m_shape->Render(_predictionTime, transform);

  glEnable(GL_BLEND);
  glDisable(GL_COLOR_MATERIAL);
  g_context->m_renderer->UnsetObjectLighting();

  // Flash starburst effect (sound is handled in Advance)
  int numFlashes = static_cast<int>(GetHighResTime() - _weapon.m_birthTime);
  float flashAlpha = 1.0f - ((GetHighResTime() - _weapon.m_birthTime) - numFlashes);
  if (flashAlpha < 0.2f)
  {
    float distToThrowable = (g_context->m_camera->GetPos() - predictedPos).Mag();
    float size = 1000.0f / sqrtf(distToThrowable);
    glColor4ub(_weapon.m_colour.r, _weapon.m_colour.g, _weapon.m_colour.b, 200);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));
    glDisable(GL_CULL_FACE);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv((predictedPos - g_context->m_camera->GetUp() * size).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((predictedPos + g_context->m_camera->GetRight() * size).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((predictedPos + g_context->m_camera->GetUp() * size).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((predictedPos - g_context->m_camera->GetRight() * size).GetData());
    glEnd();

    size *= 0.4f;
    glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));
    glBegin(GL_QUADS);
    glTexCoord2i(0, 1);
    glVertex3fv((predictedPos - g_context->m_camera->GetUp() * size).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((predictedPos + g_context->m_camera->GetRight() * size).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((predictedPos + g_context->m_camera->GetUp() * size).GetData());
    glTexCoord2i(0, 0);
    glVertex3fv((predictedPos - g_context->m_camera->GetRight() * size).GetData());
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
  }
}


// ============================================================================
//  Rocket — shape + particle trail
// ============================================================================

void WeaponRenderer::RenderRocket(const Rocket& _rocket, float _predictionTime)
{
  LegacyVector3 predictedPos = _rocket.m_pos + _rocket.m_vel * _predictionTime;

  LegacyVector3 front = _rocket.m_vel;
  front.Normalise();
  LegacyVector3 right = front ^ g_upVector;
  LegacyVector3 up = right ^ front;

  Matrix34 transform(front, up, predictedPos);

  g_context->m_renderer->SetObjectLighting();
  glEnable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_COLOR_MATERIAL);
  glDisable(GL_BLEND);

  _rocket.m_shape->Render(_predictionTime, transform);

  glEnable(GL_BLEND);
  glDisable(GL_COLOR_MATERIAL);
  g_context->m_renderer->UnsetObjectLighting();

  for (int i = 0; i < 5; ++i)
  {
    LegacyVector3 vel(_rocket.m_vel / -20.0f);
    vel.x += sfrand(8.0f);
    vel.y += sfrand(8.0f);
    vel.z += sfrand(8.0f);
    LegacyVector3 pos = predictedPos - _rocket.m_vel * (0.05f + frand(0.05f));
    pos.x += sfrand(8.0f);
    pos.y += sfrand(8.0f);
    pos.z += sfrand(8.0f);
    float size = 50.0f + frand(50.0f);
    g_context->m_particleSystem->CreateParticle(pos, vel, Particle::TypeMissileFire, size);
  }
}


// ============================================================================
//  Laser — billboard quad with team colour
// ============================================================================

void WeaponRenderer::RenderLaser(const Laser& _laser, float _predictionTime)
{
  LegacyVector3 predictedPos = _laser.m_pos + _laser.m_vel * _predictionTime;

  LegacyVector3 lengthVector = _laser.m_vel;
  lengthVector.SetLength(10.0f);
  LegacyVector3 fromPos = predictedPos;
  LegacyVector3 toPos = predictedPos - lengthVector;

  LegacyVector3 midPoint = fromPos + (toPos - fromPos) / 2.0f;
  LegacyVector3 camToMidPoint = g_context->m_camera->GetPos() - midPoint;
  float camDistSqd = camToMidPoint.MagSquared();
  LegacyVector3 rightAngle = (camToMidPoint ^ (midPoint - toPos)).Normalise();

  rightAngle *= 0.8f;

  const RGBAColour& colour = g_context->m_location->m_teams[_laser.m_fromTeamId].m_colour;
  glColor4ub(colour.r, colour.g, colour.b, 255);

  glBegin(GL_QUADS);
  for (int i = 0; i < 5; ++i)
  {
    glTexCoord2i(0, 0);
    glVertex3fv((fromPos - rightAngle).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((fromPos + rightAngle).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((toPos + rightAngle).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((toPos - rightAngle).GetData());
  }
  glEnd();

  if (camDistSqd < 200.0f)
    g_context->m_camera->CreateCameraShake(0.5f);
}


// ============================================================================
//  Shockwave — ground ring + screen flash + starburst
// ============================================================================

void WeaponRenderer::RenderShockwave(const Shockwave& _shockwave, float _predictionTime)
{
  glShadeModel(GL_SMOOTH);
  glDisable(GL_DEPTH_TEST);

  glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
  int numSteps = 50;
  float predictedLife = _shockwave.m_life - _predictionTime;
  float radius = 35.0f + 40.0f * (_shockwave.m_size - predictedLife);
  float alpha = 0.6f * predictedLife / _shockwave.m_size;

  glBegin(GL_TRIANGLE_FAN);
  glColor4f(1.0f, 1.0f, 0.0f, 0.0f);
  glVertex3fv((_shockwave.m_pos + LegacyVector3(0, 5, 0)).GetData());
  glColor4f(1.0f, 0.5f, 0.5f, alpha);

  float angle = 0.0f;
  for (int i = 0; i <= numSteps; ++i)
  {
    float xDiff = radius * sinf(angle);
    float zDiff = radius * cosf(angle);
    LegacyVector3 pos = _shockwave.m_pos + LegacyVector3(xDiff, 5, zDiff);
    glVertex3fv(pos.GetData());
    angle += 2.0f * M_PI / static_cast<float>(numSteps);
  }

  glEnd();

  glShadeModel(GL_FLAT);
  glEnable(GL_DEPTH_TEST);

  //
  // Screen flash if new

  if (_shockwave.m_size - predictedLife < 0.1f)
  {
    if (g_context->m_camera->PosInViewFrustum(_shockwave.m_pos))
    {
      float distance = (g_context->m_camera->GetPos() - _shockwave.m_pos).Mag();
      float distanceFactor = 1.0f - (distance / 500.0f);
      if (distanceFactor < 0.0f)
        distanceFactor = 0.0f;

      float flashAlpha = 1.0f - (_shockwave.m_size - predictedLife) / 0.1f;
      flashAlpha *= distanceFactor;
      glColor4f(1, 1, 1, flashAlpha);
      g_context->m_renderer->SetupMatricesFor2D();
      int screenW = g_context->m_renderer->ScreenW();
      int screenH = g_context->m_renderer->ScreenH();
      glDisable(GL_CULL_FACE);
      glBegin(GL_QUADS);
      glVertex2i(0, 0);
      glVertex2i(screenW, 0);
      glVertex2i(screenW, screenH);
      glVertex2i(0, screenH);
      glEnd();
      glEnable(GL_CULL_FACE);
      g_context->m_renderer->SetupMatricesFor3D();

      g_context->m_camera->CreateCameraShake(flashAlpha);
    }
  }

  //
  // Big blast starburst

  if (_shockwave.m_size - predictedLife < 1.0f)
  {
    float distToBang = (g_context->m_camera->GetPos() - _shockwave.m_pos).Mag();
    LegacyVector3 predictedPos = _shockwave.m_pos;
    float size = (_shockwave.m_size * 2000.0f) / sqrtf(distToBang);
    float blastAlpha = 1.0f - (_shockwave.m_size - predictedLife) / 1.0f;
    glColor4f(1.0f, 0.4f, 0.4f, blastAlpha);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));
    glDisable(GL_CULL_FACE);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv((predictedPos - g_context->m_camera->GetUp() * size).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((predictedPos + g_context->m_camera->GetRight() * size).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((predictedPos + g_context->m_camera->GetUp() * size).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((predictedPos - g_context->m_camera->GetRight() * size).GetData());
    glEnd();

    size *= 0.4f;
    glColor4f(1.0f, 1.0f, 0.0f, blastAlpha);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));
    glBegin(GL_QUADS);
    glTexCoord2i(0, 1);
    glVertex3fv((predictedPos - g_context->m_camera->GetUp() * size).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((predictedPos + g_context->m_camera->GetRight() * size).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((predictedPos + g_context->m_camera->GetUp() * size).GetData());
    glTexCoord2i(0, 0);
    glVertex3fv((predictedPos - g_context->m_camera->GetRight() * size).GetData());
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
  }
}


// ============================================================================
//  MuzzleFlash — billboard texture with fading alpha
// ============================================================================

void WeaponRenderer::RenderMuzzleFlash(const MuzzleFlash& _flash, float _predictionTime)
{
  float predictedLife = _flash.m_life - _predictionTime * 10.0f;
  LegacyVector3 predictedPos = _flash.m_pos + _flash.m_front * _predictionTime * 10.0f;

  LegacyVector3 fromPos = predictedPos;
  LegacyVector3 toPos = predictedPos + _flash.m_front * _flash.m_size;

  LegacyVector3 midPoint = fromPos + (toPos - fromPos) / 2.0f;
  LegacyVector3 camToMidPoint = g_context->m_camera->GetPos() - midPoint;
  LegacyVector3 rightAngle = (camToMidPoint ^ (midPoint - toPos)).Normalise();
  LegacyVector3 toPosToFromPos = toPos - fromPos;

  rightAngle *= _flash.m_size * 0.5f;

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/muzzleflash.bmp"));
  glDepthMask(false);

  float alpha = predictedLife;
  alpha = std::min(1.0f, alpha);
  alpha = std::max(0.0f, alpha);
  glColor4f(1.0f, 1.0f, 1.0f, alpha);

  glBegin(GL_QUADS);
  for (int i = 0; i < 5; ++i)
  {
    rightAngle *= 0.8f;
    toPosToFromPos *= 0.8f;
    glTexCoord2i(0, 0);
    glVertex3fv((fromPos - rightAngle).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((fromPos + rightAngle).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((fromPos + toPosToFromPos + rightAngle).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((fromPos + toPosToFromPos - rightAngle).GetData());
  }
  glEnd();

  glDisable(GL_TEXTURE_2D);
}


// ============================================================================
//  TurretShell — cached shape render
// ============================================================================

void WeaponRenderer::RenderTurretShell(const TurretShell& _shell, float _predictionTime)
{
  float predictionTime = _predictionTime + SERVER_ADVANCE_PERIOD;

  LegacyVector3 predictedPos = _shell.m_pos + _shell.m_vel * predictionTime;
  LegacyVector3 predictedFront = _shell.m_vel;
  predictedFront.Normalise();
  LegacyVector3 right = predictedFront ^ g_upVector;
  LegacyVector3 up = right ^ predictedFront;

  static ShapeStatic* s_turretShellShape = nullptr;
  if (!s_turretShellShape)
    s_turretShellShape = Resource::GetShapeStatic("turretshell.shp");

  Matrix34 shellMat(predictedFront, up, predictedPos);

  glDisable(GL_BLEND);
  g_context->m_renderer->SetObjectLighting();
  s_turretShellShape->Render(predictionTime, shellMat);
  g_context->m_renderer->UnsetObjectLighting();
  glEnable(GL_BLEND);
}

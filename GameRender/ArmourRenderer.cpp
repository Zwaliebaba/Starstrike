#include "pch.h"
#include "ArmourRenderer.h"
#include "FlagRenderer.h"
#include "armour.h"
#include "ShapeStatic.h"
#include "matrix34.h"
#include "renderer.h"
#include "resource.h"
#include "GameApp.h"
#include "main.h"

void ArmourRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
  const auto& armour = static_cast<const Armour&>(_entity);

  if (armour.m_dead)
    return;

  //
  // Work out our predicted position

  LegacyVector3 predictedPos = armour.m_pos + armour.m_vel * _ctx.predictionTime;
  predictedPos.y += sinf(g_gameTime + armour.m_id.GetUniqueId()) * 2;
  LegacyVector3 predictedUp = armour.m_up;
  predictedUp.x += sinf((g_gameTime + armour.m_id.GetUniqueId()) * 2) * 0.05f;
  predictedUp.z += cosf(g_gameTime + armour.m_id.GetUniqueId()) * 0.05f;

  LegacyVector3 predictedFront = armour.m_front;
  LegacyVector3 right = predictedUp ^ predictedFront;
  predictedUp.Normalise();
  predictedFront = right ^ predictedUp;
  predictedFront.Normalise();

  //
  // Render the tank body

  Matrix34 bodyMat(predictedFront, predictedUp, predictedPos);

  if (armour.m_renderDamaged)
  {
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);

    if (frand() > 0.5f)
      bodyMat.r *= (1.0f + sinf(g_gameTime) * 0.5f);
    else
      bodyMat.f *= (1.0f + sinf(g_gameTime) * 0.5f);
  }

  g_context->m_renderer->SetObjectLighting();
  armour.m_shape->Render(_ctx.predictionTime, bodyMat);
  g_context->m_renderer->UnsetObjectLighting();

  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  //
  // Render the flag

  float timeIndex = g_gameTime + armour.m_id.GetUniqueId() * 10;
  Matrix34 flagMat = armour.m_shape->GetMarkerWorldMatrix(armour.m_markerFlag, bodyMat);

  // Copy the flag objects — Flag methods are mutable but we have const access.
  Flag flag = armour.m_flag;
  flag.SetPosition(flagMat.pos);
  flag.SetOrientation(predictedFront * -1, predictedUp);
  flag.SetSize(20.0f);

  switch (armour.m_state)
  {
  case 0: // StateIdle
    flag.SetTexture(Resource::GetTexture("icons/banner_none.bmp"));
    break;
  case 1: // StateUnloading
    flag.SetTexture(Resource::GetTexture("icons/banner_unload.bmp"));
    break;
  case 2: // StateLoading
    flag.SetTexture(Resource::GetTexture("icons/banner_follow.bmp"));
    break;
  }

  FlagRenderer::Render(flag);

  if (armour.m_numPassengers > 0)
  {
    auto caption = std::format("{}", armour.m_numPassengers);
    FlagRenderer::RenderText(flag, 2, 2, caption.c_str());
  }

  //
  // If we are about to deploy, render a flag at the target

  if (armour.m_conversionPoint != g_zeroVector)
  {
    LegacyVector3 front(0, 0, -1);
    front.RotateAroundY(g_gameTime * 0.5f);
    LegacyVector3 up = g_upVector;
    up.RotateAround(front * sinf(timeIndex * 3) * 0.3f);

    Flag deployFlag = armour.m_deployFlag;
    deployFlag.SetPosition(armour.m_conversionPoint);
    deployFlag.SetOrientation(front, up);
    deployFlag.SetSize(30.0f);
    deployFlag.SetTexture(Resource::GetTexture("icons/banner_deploy.bmp"));
    FlagRenderer::Render(deployFlag);
  }
}

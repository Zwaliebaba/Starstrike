#include "pch.h"
#include "SporeGeneratorRenderer.h"
#include "ShadowRenderer.h"
#include "sporegenerator.h"
#include "GameApp.h"
#include "camera.h"
#include "main.h"
#include "renderer.h"
#include "ShapeStatic.h"

static void RenderTail(const LegacyVector3& _from, const LegacyVector3& _to, float _size)
{
  LegacyVector3 camToOurPos = g_context->m_camera->GetPos() - _from;
  LegacyVector3 lineOurPos = camToOurPos ^ (_from - _to);
  lineOurPos.SetLength(_size);

  LegacyVector3 lineVector = (_to - _from).Normalise();
  LegacyVector3 right = lineVector ^ g_upVector;
  LegacyVector3 normal = right ^ lineVector;
  normal.Normalise();

  glNormal3fv(normal.GetData());

  glVertex3fv((_from - lineOurPos).GetData());
  glVertex3fv((_from + lineOurPos).GetData());
  glVertex3fv((_to - lineOurPos).GetData());
  glVertex3fv((_to + lineOurPos).GetData());
}

void SporeGeneratorRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
  auto& spore = static_cast<const SporeGenerator&>(_entity);

  if (spore.m_dead)
    return;

  auto entityFront = LegacyVector3(0, 0, 1);
  LegacyVector3 entityUp = g_upVector;
  LegacyVector3 predictedPos = spore.m_pos + spore.m_vel * _ctx.predictionTime;

  //
  // 3d Shape

  g_context->m_renderer->SetObjectLighting();
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);

  Matrix34 mat(entityFront, entityUp, predictedPos);

  if (spore.m_renderDamaged)
  {
    float timeIndex = static_cast<float>(g_gameTime) + spore.m_id.GetUniqueId() * 10;
    float thefrand = frand();
    if (thefrand > 0.7f)
      mat.f *= (1.0f - sinf(timeIndex) * 0.5f);
    else if (thefrand > 0.4f)
      mat.u *= (1.0f - sinf(timeIndex) * 0.5f);
    else
      mat.r *= (1.0f - sinf(timeIndex) * 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
  }

  spore.m_shape->Render(_ctx.predictionTime, mat);

  //
  // Tails

  glColor4f(0.2f, 0.0f, 0.0f, 1.0f);
  glEnable(GL_COLOR_MATERIAL);

  int numTailParts = 3;
  static LegacyVector3 s_vel;
  s_vel = s_vel * 0.95f + spore.m_vel * 0.05f;

  for (int i = 0; i < SPOREGENERATOR_NUMTAILS; ++i)
  {
    LegacyVector3 prevTailPos = spore.m_shape->GetMarkerWorldMatrix(spore.m_tail[i], mat).pos;
    LegacyVector3 prevTailDir = (prevTailPos - predictedPos);
    prevTailDir.HorizontalAndNormalise();

    glBegin(GL_QUAD_STRIP);

    for (int j = 0; j < numTailParts; ++j)
    {
      LegacyVector3 thisTailPos = prevTailPos + prevTailDir * 15.0f;
      prevTailDir = (thisTailPos - prevTailPos);
      prevTailDir.HorizontalAndNormalise();

      float timeIndex = static_cast<float>(g_gameTime) + i + j + spore.m_id.GetUniqueId() * 10;
      thisTailPos += LegacyVector3(sinf(timeIndex) * 10.0f, sinf(timeIndex) * 10.0f, sinf(timeIndex) * 10.0f);

      LegacyVector3 vel = s_vel;
      vel.SetLength(10.0f * static_cast<float>(j) / static_cast<float>(numTailParts));
      thisTailPos += vel;
      thisTailPos.y -= 10.0f * static_cast<float>(j) / static_cast<float>(numTailParts);

      if (spore.m_state == SporeGenerator::StatePanic)
      {
        float panicFraction = 5.0f;
        float panicTime = static_cast<float>(g_gameTime) * panicFraction + i + j;
        thisTailPos += LegacyVector3(cosf(panicTime) * 5.0f, cosf(panicTime) * 5.0f,
                                     cosf(panicTime) * 5.0f);
      }

      float size = 1.0f - static_cast<float>(j) / static_cast<float>(numTailParts);
      size *= 2.0f;
      size += sinf(static_cast<float>(g_gameTime) + i + j) * 0.3f;

      RenderTail(prevTailPos, thisTailPos, size);
      prevTailPos = thisTailPos;
    }

    glEnd();
  }

  glDisable(GL_COLOR_MATERIAL);
  glEnable(GL_CULL_FACE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  g_context->m_renderer->UnsetObjectLighting();

  //
  // Shadow

  ShadowRenderer::Begin();
  ShadowRenderer::Render(predictedPos, 20.0f);
  ShadowRenderer::End();
}

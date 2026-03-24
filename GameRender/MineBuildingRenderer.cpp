#include "pch.h"
#include "MineBuildingRenderer.h"
#include "mine.h"
#include "3d_sprite.h"
#include "camera.h"
#include "GameApp.h"
#include "location.h"
#include "preferences.h"
#include "renderer.h"
#include "resource.h"
#include "ShapeStatic.h"

// ---------------------------------------------------------------------------
// MineBuildingRenderer::Render
// ---------------------------------------------------------------------------

void MineBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
  auto& mine = static_cast<const MineBuilding&>(_building);

  float predTime = _ctx.predictionTime - 0.1f;

  for (int i = 0; i < mine.m_carts.Size(); ++i)
  {
    MineCart* cart = mine.m_carts[i];
    RenderCart(mine, cart, predTime);
  }

  DefaultBuildingRenderer::Render(_building, _ctx);
}

// ---------------------------------------------------------------------------
// MineBuildingRenderer::RenderAlphas
// ---------------------------------------------------------------------------

void MineBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
  DefaultBuildingRenderer::RenderAlphas(_building, _ctx);

  auto& mine = static_cast<const MineBuilding&>(_building);

  if (mine.m_trackLink != -1)
  {
    Building* trackLink = g_context->m_location->GetBuilding(mine.m_trackLink);
    if (trackLink)
    {
      int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);

      auto mineLink = static_cast<MineBuilding*>(trackLink);

      LegacyVector3 ourPos1 = const_cast<MineBuilding&>(mine).GetTrackMarker1();
      LegacyVector3 theirPos1 = mineLink->GetTrackMarker1();
      LegacyVector3 ourPos2 = const_cast<MineBuilding&>(mine).GetTrackMarker2();
      LegacyVector3 theirPos2 = mineLink->GetTrackMarker2();

      glColor4f(0.85f, 0.4f, 0.4f, 1.0f);

      float size = 2.0f;
      if (buildingDetail > 1)
        size = 1.0f;

      LegacyVector3 camToOurPos1 = g_context->m_camera->GetPos() - ourPos1;
      LegacyVector3 lineOurPos1 = camToOurPos1 ^ (ourPos1 - theirPos1);
      lineOurPos1.SetLength(size);

      LegacyVector3 camToTheirPos1 = g_context->m_camera->GetPos() - theirPos1;
      LegacyVector3 lineTheirPos1 = camToTheirPos1 ^ (ourPos1 - theirPos1);
      lineTheirPos1.SetLength(size);

      glDisable(GL_CULL_FACE);

      if (buildingDetail == 1)
      {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/laser.bmp"));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glEnable(GL_BLEND);
      }

      glDepthMask(false);

      glBegin(GL_QUADS);
      glTexCoord2f(0.1f, 0.0f);
      glVertex3fv((ourPos1 - lineOurPos1).GetData());
      glTexCoord2f(0.1f, 1.0f);
      glVertex3fv((ourPos1 + lineOurPos1).GetData());
      glTexCoord2f(0.9f, 1.0f);
      glVertex3fv((theirPos1 + lineTheirPos1).GetData());
      glTexCoord2f(0.9f, 0.0f);
      glVertex3fv((theirPos1 - lineTheirPos1).GetData());
      glEnd();

      glBegin(GL_QUADS);
      glTexCoord2f(0.1f, 0.0f);
      glVertex3fv((ourPos2 - lineOurPos1).GetData());
      glTexCoord2f(0.1f, 1.0f);
      glVertex3fv((ourPos2 + lineOurPos1).GetData());
      glTexCoord2f(0.9f, 1.0f);
      glVertex3fv((theirPos2 + lineTheirPos1).GetData());
      glTexCoord2f(0.9f, 0.0f);
      glVertex3fv((theirPos2 - lineTheirPos1).GetData());
      glEnd();

      glDepthMask(true);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      glDisable(GL_TEXTURE_2D);
      glEnable(GL_CULL_FACE);
    }
  }
}

// ---------------------------------------------------------------------------
// MineBuildingRenderer::RenderCart
// ---------------------------------------------------------------------------

void MineBuildingRenderer::RenderCart(const MineBuilding& _mine, MineCart* _cart, float _predictionTime)
{
  if (_mine.m_trackLink == -1)
    return;

  Building* trackLink = g_context->m_location->GetBuilding(_mine.m_trackLink);
  if (!trackLink)
    return;

  auto mineBuilding = static_cast<MineBuilding*>(trackLink);

  int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);

  LegacyVector3 ourMarker1 = const_cast<MineBuilding&>(_mine).GetTrackMarker1();
  LegacyVector3 ourMarker2 = const_cast<MineBuilding&>(_mine).GetTrackMarker2();
  LegacyVector3 theirMarker1 = mineBuilding->GetTrackMarker1();
  LegacyVector3 theirMarker2 = mineBuilding->GetTrackMarker2();

  float mineSpeed = MineBuilding::RefinerySpeed();

  float predictedProgress = _cart->m_progress;
  predictedProgress += _predictionTime * mineSpeed;
  if (predictedProgress < 0.0f)
    predictedProgress = 0.0f;
  if (predictedProgress > 1.0f)
    predictedProgress = 1.0f;

  LegacyVector3 trackLeft = ourMarker1 + (theirMarker1 - ourMarker1) * predictedProgress;
  LegacyVector3 trackRight = ourMarker2 + (theirMarker2 - ourMarker2) * predictedProgress;

  LegacyVector3 cartPos = (trackLeft + trackRight) / 2.0f;
  cartPos += LegacyVector3(0, -40, 0);

  if (g_context->m_camera->PosInViewFrustum(cartPos))
  {
    LegacyVector3 cartFront = (trackLeft - trackRight) ^ g_upVector;
    cartFront.y = 0.0f;
    cartFront.Normalise();

    Matrix34 transform(cartFront, g_upVector, cartPos);
    MineBuilding::s_cartShape->Render(0.0f, transform);

    LegacyVector3 cartLinkLeft = MineBuilding::s_cartShape->GetMarkerWorldMatrix(MineBuilding::s_cartMarker1, transform).pos;
    LegacyVector3 cartLinkRight = MineBuilding::s_cartShape->GetMarkerWorldMatrix(MineBuilding::s_cartMarker2, transform).pos;

    LegacyVector3 camRight = g_context->m_camera->GetRight() * 0.5f;
    glBegin(GL_QUADS);
    glVertex3fv((trackLeft - camRight).GetData());
    glVertex3fv((trackLeft + camRight).GetData());
    glVertex3fv((cartLinkLeft + camRight).GetData());
    glVertex3fv((cartLinkLeft - camRight).GetData());

    glVertex3fv((trackRight - camRight).GetData());
    glVertex3fv((trackRight + camRight).GetData());
    glVertex3fv((cartLinkRight + camRight).GetData());
    glVertex3fv((cartLinkRight - camRight).GetData());
    glEnd();

    for (int i = 0; i < 3; ++i)
    {
      if (_cart->m_polygons[i])
      {
        Matrix34 polyMat = MineBuilding::s_cartShape->GetMarkerWorldMatrix(MineBuilding::s_cartContents[i], transform);
        MineBuilding::s_polygon1->Render(0.0f, polyMat);
      }

      if (_cart->m_primitives[i])
      {
        Matrix34 polyMat = MineBuilding::s_cartShape->GetMarkerWorldMatrix(MineBuilding::s_cartContents[i], transform);
        MineBuilding::s_primitive1->Render(0.0f, polyMat);

        if (buildingDetail < 3)
        {
          glEnable(GL_BLEND);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE);
          glDepthMask(false);
          glDisable(GL_LIGHTING);

          glColor4f(1.0f, 0.7f, 0.0f, 0.75f);

          float nearPlaneStart = g_context->m_renderer->GetNearPlane();
          g_context->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.1f, g_context->m_renderer->GetFarPlane());

          Render3DSprite(polyMat.pos - LegacyVector3(0, 25, 0), 50.0f, 50.0f, Resource::GetTexture("textures/glow.bmp"));

          g_context->m_camera->SetupProjectionMatrix(nearPlaneStart, g_context->m_renderer->GetFarPlane());

          glEnable(GL_LIGHTING);
          glEnable(GL_DEPTH_TEST);
          glDepthMask(true);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          glDisable(GL_BLEND);
        }
      }
    }
  }
}

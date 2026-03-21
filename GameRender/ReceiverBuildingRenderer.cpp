#include "pch.h"

#include "ReceiverBuildingRenderer.h"
#include "spiritreceiver.h"
#include "camera.h"
#include "GameApp.h"
#include "location.h"
#include "preferences.h"
#include "renderer.h"
#include "resource.h"
#include "ShapeStatic.h"

// ---------------------------------------------------------------------------
// File-static state shared between Begin/End for spirit batch rendering.
// ---------------------------------------------------------------------------

static float s_nearPlaneStart;

// ---------------------------------------------------------------------------
// ReceiverBuildingRenderer::RenderAlphas
// ---------------------------------------------------------------------------

void ReceiverBuildingRenderer::RenderAlphas(const Building& _building,
                                            const BuildingRenderContext& _ctx)
{
    DefaultBuildingRenderer::RenderAlphas(_building, _ctx);

    auto& receiver = static_cast<const ReceiverBuilding&>(_building);

    float predictionTime = _ctx.predictionTime - 0.1f;

    Building* spiritLink = g_app->m_location->GetBuilding(receiver.m_spiritLink);

    int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);

    if (spiritLink)
    {
        auto receiverLink = static_cast<ReceiverBuilding*>(spiritLink);

        LegacyVector3 ourPos = const_cast<ReceiverBuilding&>(receiver).GetSpiritLocation();
        LegacyVector3 theirPos = receiverLink->GetSpiritLocation();

        LegacyVector3 camToOurPos = g_app->m_camera->GetPos() - ourPos;
        LegacyVector3 ourPosRight = camToOurPos ^ (theirPos - ourPos);

        LegacyVector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
        LegacyVector3 theirPosRight = camToTheirPos ^ (theirPos - ourPos);

        glDisable(GL_CULL_FACE);
        glDepthMask(false);
        glColor4f(0.9f, 0.9f, 0.5f, 1.0f);

        float size = 0.5f;

        if (buildingDetail == 1)
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laser.bmp"));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            size = 1.0f;
        }

        ourPosRight.SetLength(size);
        theirPosRight.SetLength(size);

        glBegin(GL_QUADS);
        glTexCoord2f(0.1f, 0);
        glVertex3fv(const_cast<float*>((ourPos - ourPosRight).GetData()));
        glTexCoord2f(0.1f, 1);
        glVertex3fv(const_cast<float*>((ourPos + ourPosRight).GetData()));
        glTexCoord2f(0.9f, 1);
        glVertex3fv(const_cast<float*>((theirPos + theirPosRight).GetData()));
        glTexCoord2f(0.9f, 0);
        glVertex3fv(const_cast<float*>((theirPos - theirPosRight).GetData()));
        glEnd();

        glDisable(GL_TEXTURE_2D);

        // Render any surges
        BeginRenderUnprocessedSpirits();
        for (int i = 0; i < receiver.m_spirits.Size(); ++i)
        {
            float thisSpirit = receiver.m_spirits[i];
            thisSpirit += predictionTime * 0.8f;
            if (thisSpirit < 0.0f) thisSpirit = 0.0f;
            if (thisSpirit > 1.0f) thisSpirit = 1.0f;
            LegacyVector3 thisSpiritPos = ourPos + (theirPos - ourPos) * thisSpirit;
            RenderUnprocessedSpirit(thisSpiritPos, 1.0f);
        }
        EndRenderUnprocessedSpirits();
    }
}

// ---------------------------------------------------------------------------
// Static spirit glyph batch helpers
// ---------------------------------------------------------------------------

void ReceiverBuildingRenderer::BeginRenderUnprocessedSpirits()
{
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(false);

    int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);
    if (buildingDetail == 1)
        glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/glow.bmp"));

    s_nearPlaneStart = g_app->m_renderer->GetNearPlane();
    g_app->m_camera->SetupProjectionMatrix(s_nearPlaneStart * 1.1f,
                                           g_app->m_renderer->GetFarPlane());
}

void ReceiverBuildingRenderer::RenderUnprocessedSpirit(const LegacyVector3& _pos, float _life)
{
    LegacyVector3 position = _pos;
    LegacyVector3 camUp = g_app->m_camera->GetUp();
    LegacyVector3 camRight = g_app->m_camera->GetRight();
    float scale = 2.0f * _life;
    float alphaValue = _life;

    glColor4f(0.6f, 0.2f, 0.1f, alphaValue);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv(const_cast<float*>((position + camUp * 3.0f * scale).GetData()));
    glTexCoord2i(1, 0);
    glVertex3fv(const_cast<float*>((position + camRight * 3.0f * scale).GetData()));
    glTexCoord2i(1, 1);
    glVertex3fv(const_cast<float*>((position - camUp * 3.0f * scale).GetData()));
    glTexCoord2i(0, 1);
    glVertex3fv(const_cast<float*>((position - camRight * 3.0f * scale).GetData()));
    glTexCoord2i(0, 0);
    glVertex3fv(const_cast<float*>((position + camUp * 3.0f * scale).GetData()));
    glTexCoord2i(1, 0);
    glVertex3fv(const_cast<float*>((position + camRight * 3.0f * scale).GetData()));
    glTexCoord2i(1, 1);
    glVertex3fv(const_cast<float*>((position - camUp * 3.0f * scale).GetData()));
    glTexCoord2i(0, 1);
    glVertex3fv(const_cast<float*>((position - camRight * 3.0f * scale).GetData()));
    glEnd();

    glColor4f(0.6f, 0.2f, 0.1f, alphaValue);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv(const_cast<float*>((position + camUp * 1.0f * scale).GetData()));
    glTexCoord2i(1, 0);
    glVertex3fv(const_cast<float*>((position + camRight * 1.0f * scale).GetData()));
    glTexCoord2i(1, 1);
    glVertex3fv(const_cast<float*>((position - camUp * 1.0f * scale).GetData()));
    glTexCoord2i(0, 1);
    glVertex3fv(const_cast<float*>((position - camRight * 1.0f * scale).GetData()));
    glTexCoord2i(0, 0);
    glVertex3fv(const_cast<float*>((position + camUp * 1.0f * scale).GetData()));
    glTexCoord2i(1, 0);
    glVertex3fv(const_cast<float*>((position + camRight * 1.0f * scale).GetData()));
    glTexCoord2i(1, 1);
    glVertex3fv(const_cast<float*>((position - camUp * 1.0f * scale).GetData()));
    glTexCoord2i(0, 1);
    glVertex3fv(const_cast<float*>((position - camRight * 1.0f * scale).GetData()));
    glEnd();

    int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);
    if (buildingDetail == 1)
    {
        glEnable(GL_TEXTURE_2D);
        glColor4f(0.6f, 0.2f, 0.1f, alphaValue / 1.5f);
        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3fv(const_cast<float*>((position + camUp * 30.0f * scale).GetData()));
        glTexCoord2i(1, 0);
        glVertex3fv(const_cast<float*>((position + camRight * 30.0f * scale).GetData()));
        glTexCoord2i(1, 1);
        glVertex3fv(const_cast<float*>((position - camUp * 30.0f * scale).GetData()));
        glTexCoord2i(0, 1);
        glVertex3fv(const_cast<float*>((position - camRight * 30.0f * scale).GetData()));
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }
}

void ReceiverBuildingRenderer::EndRenderUnprocessedSpirits()
{
    g_app->m_camera->SetupProjectionMatrix(s_nearPlaneStart,
                                           g_app->m_renderer->GetFarPlane());

    glDisable(GL_TEXTURE_2D);
    glDepthMask(true);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

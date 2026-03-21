#include "pch.h"

#include "SpawnBuildingRenderer.h"
#include "spawnpoint.h"
#include "camera.h"
#include "GameApp.h"
#include "location.h"
#include "preferences.h"
#include "resource.h"

void SpawnBuildingRenderer::RenderSpirit(const LegacyVector3& _pos)
{
    LegacyVector3 pos = _pos;

    int innerAlpha = 255;
    int outerAlpha = 150;
    int spiritInnerSize = 2;
    int spiritOuterSize = 6;

    float size = static_cast<float>(spiritInnerSize);
    glColor4ub(150, 50, 25, innerAlpha);

    glBegin(GL_QUADS);
    glVertex3fv((pos - g_app->m_camera->GetUp() * size).GetData());
    glVertex3fv((pos + g_app->m_camera->GetRight() * size).GetData());
    glVertex3fv((pos + g_app->m_camera->GetUp() * size).GetData());
    glVertex3fv((pos - g_app->m_camera->GetRight() * size).GetData());
    glEnd();

    size = static_cast<float>(spiritOuterSize);
    glColor4ub(150, 50, 25, outerAlpha);

    glBegin(GL_QUADS);
    glVertex3fv((pos - g_app->m_camera->GetUp() * size).GetData());
    glVertex3fv((pos + g_app->m_camera->GetRight() * size).GetData());
    glVertex3fv((pos + g_app->m_camera->GetUp() * size).GetData());
    glVertex3fv((pos - g_app->m_camera->GetRight() * size).GetData());
    glEnd();
}

void SpawnBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    const auto& spawn = static_cast<const SpawnBuilding&>(_building);

    LegacyVector3 ourPos = const_cast<SpawnBuilding&>(spawn).GetSpiritLink();

    int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);

    for (int i = 0; i < spawn.m_links.Size(); ++i)
    {
        SpawnBuildingLink* link = spawn.m_links[i];
        SpawnBuilding* building = static_cast<SpawnBuilding*>(g_app->m_location->GetBuilding(link->m_targetBuildingId));
        if (building)
        {
            LegacyVector3 theirPos = building->GetSpiritLink();

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
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);

                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laser.bmp"));
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

                size = 1.0f;
            }

            theirPosRight.SetLength(size);
            ourPosRight.SetLength(size);

            glBegin(GL_QUADS);
            glTexCoord2f(0.1f, 0);
            glVertex3fv((ourPos - ourPosRight).GetData());
            glTexCoord2f(0.1f, 1);
            glVertex3fv((ourPos + ourPosRight).GetData());
            glTexCoord2f(0.9f, 1);
            glVertex3fv((theirPos + theirPosRight).GetData());
            glTexCoord2f(0.9f, 0);
            glVertex3fv((theirPos - theirPosRight).GetData());
            glEnd();

            glDisable(GL_TEXTURE_2D);

            //
            // Render spirits in transit

            for (int j = 0; j < link->m_spirits.Size(); ++j)
            {
                SpawnBuildingSpirit* spirit = link->m_spirits[j];
                float predictedProgress = spirit->m_currentProgress + _ctx.predictionTime;
                if (predictedProgress >= 0.0f && predictedProgress <= 1.0f)
                {
                    LegacyVector3 position = ourPos + (theirPos - ourPos) * predictedProgress;
                    RenderSpirit(position);
                }
            }
        }
    }

    DefaultBuildingRenderer::RenderAlphas(_building, _ctx);
}

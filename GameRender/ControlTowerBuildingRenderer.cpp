#include "pch.h"
#include "ControlTowerBuildingRenderer.h"

#include "controltower.h"
#include "camera.h"
#include "clienttoserver.h"
#include "GameApp.h"
#include "globals.h"
#include "hi_res_time.h"
#include "location.h"
#include "main.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "team.h"

void ControlTowerBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    DefaultBuildingRenderer::Render(_building, _ctx);

    auto& tower = static_cast<const ControlTower&>(_building);
    ControlTower::s_dishShape->Render(_ctx.predictionTime, tower.m_dishMatrix);
}

void ControlTowerBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    DefaultBuildingRenderer::RenderAlphas(_building, _ctx);

    auto& tower = static_cast<const ControlTower&>(_building);

    //
    // Control lines will be bright when we are near a control tower
    // And dim when we are not
    // Recalculate our distance to the nearest control tower once per second

    static int s_lastRecalculation = 0.0f;
    static float s_distanceScale = 0.0f;
    static float s_desiredDistanceScale = 0.0f;

    if (static_cast<int>(GetHighResTime()) > s_lastRecalculation)
    {
        s_lastRecalculation = static_cast<int>(GetHighResTime());

        float nearest = 99999.9f;
        for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
        {
            if (g_app->m_location->m_buildings.ValidIndex(i))
            {
                Building* building = g_app->m_location->m_buildings[i];
                if (building && building->m_type == Building::TypeControlTower)
                {
                    float camDist = (building->m_pos - g_app->m_camera->GetPos()).Mag();
                    if (camDist < nearest)
                        nearest = camDist;
                }
            }
        }

        if (nearest < 200.0f)
            s_desiredDistanceScale = 1.0f;
        else
            s_desiredDistanceScale = 0.1f;
    }

    if (s_desiredDistanceScale > s_distanceScale)
    {
        s_distanceScale = (s_desiredDistanceScale * SERVER_ADVANCE_PERIOD * 0.1f) + (s_distanceScale * (1.0f - SERVER_ADVANCE_PERIOD * 0.1f));
    }
    else
    {
        s_distanceScale = (s_desiredDistanceScale * SERVER_ADVANCE_PERIOD * 0.03f) + (s_distanceScale * (1.0f - SERVER_ADVANCE_PERIOD * 0.03f));
    }

    //
    // Pre-compute some positions

    Matrix34 rootMat(tower.m_front, g_upVector, tower.m_pos);
    Matrix34 worldMat = tower.m_shape->GetMarkerWorldMatrix(tower.m_lightPos, rootMat);
    LegacyVector3 lightPos = worldMat.pos;

    LegacyVector3 camR = g_app->m_camera->GetRight();
    LegacyVector3 camU = g_app->m_camera->GetUp();

    RGBAColour colour;
    if (tower.m_id.GetTeamId() == 255)
        colour.Set(128, 128, 128, 255);
    else
        colour = g_app->m_location->m_teams[tower.m_id.GetTeamId()].m_colour;

    //
    // Draw control line to heaven

    if (!g_app->m_editing)
    {
        LegacyVector3 controlUp(0, 50.0f + (tower.m_id.GetUniqueId() % 50), 0);

        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glShadeModel(GL_SMOOTH);
        glDepthMask(false);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laser.bmp"));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        float w = (lightPos - g_app->m_camera->GetPos()).Mag() * 0.002f;
        w = max(0.5f, w);

        for (int i = 0; i < 10; ++i)
        {
            LegacyVector3 thisUp1 = LegacyVector3(0, -20, 0) + controlUp * static_cast<float>(i);
            LegacyVector3 thisUp2 = LegacyVector3(0, -20, 0) + controlUp * static_cast<float>(i + 1);

            int alpha = 255 - 255 * static_cast<float>(i) / 10.0f;
            int alpha2 = 255 - 255 * static_cast<float>(i + 1) / 10.0f;

            alpha *= fabs(sinf(g_gameTime * 2 + static_cast<float>(i) / 5.0f));
            alpha2 *= fabs(sinf(g_gameTime * 2 + static_cast<float>(i + 1) / 5.0f));

            alpha *= s_distanceScale;
            alpha2 *= s_distanceScale;

            float y = static_cast<float>(i) / 10.0f;
            float h = 1.0f / 10.0f;

            glBegin(GL_QUADS);
            glColor4ub(colour.r, colour.g, colour.b, alpha);

            glTexCoord2f(y, 0);
            glVertex3fv((lightPos - camR * w + thisUp1).GetData());
            glTexCoord2f(y, 1);
            glVertex3fv((lightPos + camR * w + thisUp1).GetData());

            glColor4ub(colour.r, colour.g, colour.b, alpha2);

            glTexCoord2f(y + h, 1);
            glVertex3fv((lightPos + camR * w + thisUp2).GetData());
            glTexCoord2f(y + h, 0);
            glVertex3fv((lightPos - camR * w + thisUp2).GetData());
            glEnd();
        }

        glDisable(GL_TEXTURE_2D);

        glDepthMask(true);
        glShadeModel(GL_FLAT);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }

    //
    // Draw our signal flash

    int lastSeqId = g_app->m_clientToServer->m_lastValidSequenceIdFromServer;

    if ((tower.m_id.GetTeamId() != 255 && (lastSeqId % 10) / 2 == tower.m_id.GetTeamId()) || tower.m_beingReprogrammed[lastSeqId % 3] || g_app->m_editing)
    {
        Matrix34 rootMat2(tower.m_front, g_upVector, tower.m_pos);
        Matrix34 worldMat2 = tower.m_shape->GetMarkerWorldMatrix(tower.m_lightPos, rootMat2);
        LegacyVector3 flashPos = worldMat2.pos;

        float signalSize = tower.m_ownership / 5.0f;

        glColor4ub(colour.r, colour.g, colour.b, 255);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/starburst.bmp"));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glDisable(GL_CULL_FACE);
        glDepthMask(false);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        for (int i = 0; i < 10; ++i)
        {
            float size = signalSize * static_cast<float>(i) / 10.0f;
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3fv((flashPos - camR * size - camU * size).GetData());
            glTexCoord2f(1.0f, 0.0f);
            glVertex3fv((flashPos + camR * size - camU * size).GetData());
            glTexCoord2f(1.0f, 1.0f);
            glVertex3fv((flashPos + camR * size + camU * size).GetData());
            glTexCoord2f(0.0f, 1.0f);
            glVertex3fv((flashPos - camR * size + camU * size).GetData());
            glEnd();
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);

        glDepthMask(true);
        glEnable(GL_CULL_FACE);
        glDisable(GL_TEXTURE_2D);
    }
}

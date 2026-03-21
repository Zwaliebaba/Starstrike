#include "pch.h"

#include "FuelStationBuildingRenderer.h"
#include "rocket.h"
#include "building.h"
#include "main.h"
#include "camera.h"
#include "resource.h"
#include "text_renderer.h"
#include "GameApp.h"
#include "location.h"

void FuelStationBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    // Intentionally skip FuelBuilding::RenderAlphas — the original
    // FuelStation::RenderAlphas prevented the fuel-flow quad from rendering.

    auto& station = static_cast<const FuelStation&>(_building);

    Building* building = g_app->m_location->GetBuilding(station.m_fuelLink);
    if (building && building->m_type == Building::TypeEscapeRocket)
    {
        auto rocket = static_cast<EscapeRocket*>(building);
        if ((rocket->m_state == EscapeRocket::StateCountdown && rocket->m_countdown <= 10.0f) ||
            rocket->m_state == EscapeRocket::StateFlight)
        {
            float screenSize = 60.0f;
            LegacyVector3 screenFront = _building.m_front;
            LegacyVector3 screenRight = screenFront ^ g_upVector;
            LegacyVector3 screenPos = _building.m_pos + LegacyVector3(0, 150, 0);
            screenPos -= screenRight * screenSize * 0.5f;
            screenPos += screenFront * 30;
            LegacyVector3 screenUp = g_upVector;

            //
            // Render lines for over effect

            glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/interface_grey.bmp"));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);
            glDepthMask(false);
            glShadeModel(GL_SMOOTH);
            glDisable(GL_CULL_FACE);

            float texX = 0.0f;
            float texW = 3.0f;
            float texY = g_gameTime * 0.01f;
            float texH = 0.3f;

            glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glBegin(GL_QUADS);
            glVertex3fv(screenPos.GetData());
            glVertex3fv((screenPos + screenRight * screenSize).GetData());
            glVertex3fv((screenPos + screenRight * screenSize + screenUp * screenSize).GetData());
            glVertex3fv((screenPos + screenUp * screenSize).GetData());
            glEnd();

            glColor4f(1.0f, 0.4f, 0.2f, 1.0f);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            for (int i = 0; i < 2; ++i)
            {
                glBegin(GL_QUADS);
                glTexCoord2f(texX, texY);
                glVertex3fv(screenPos.GetData());

                glTexCoord2f(texX + texW, texY);
                glVertex3fv((screenPos + screenRight * screenSize).GetData());

                glTexCoord2f(texX + texW, texY + texH);
                glVertex3fv((screenPos + screenRight * screenSize + screenUp * screenSize).GetData());

                glTexCoord2f(texX, texY + texH);
                glVertex3fv((screenPos + screenUp * screenSize).GetData());
                glEnd();

                texY *= 1.5f;
                texH = 0.1f;
            }

            glDepthMask(false);

            //
            // Render countdown

            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            LegacyVector3 textPos = screenPos + screenRight * screenSize * 0.5f + screenUp * screenSize * 0.5f;

            if (rocket->m_state == EscapeRocket::StateCountdown)
            {
                int countdown = static_cast<int>(rocket->m_countdown) + 1;
                g_gameFont.DrawText3D(textPos, screenFront, g_upVector, 50, "%d", countdown);
            }
            else
            {
                if (fmod(g_gameTime, 2) < 1)
                    g_gameFont.DrawText3D(textPos, screenFront, g_upVector, 50, "0");
            }

            //
            // Render projection effect

            glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laser.bmp"));

            LegacyVector3 ourPos = _building.m_pos + LegacyVector3(0, 90, 0);
            LegacyVector3 theirPos = _building.m_pos + LegacyVector3(0, 200, 0);
            theirPos += screenFront * 30.0f;

            LegacyVector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
            LegacyVector3 lineTheirPos = camToTheirPos ^ (ourPos - theirPos);
            lineTheirPos.SetLength(_building.m_radius * 0.5f);

            for (int i = 0; i < 3; ++i)
            {
                LegacyVector3 pos = theirPos;
                pos.x += sinf(g_gameTime + i) * 15;
                pos.y += sinf(g_gameTime + i) * 15;
                pos.z += cosf(g_gameTime + i) * 15;

                float blue = 0.5f + fabs(sinf(g_gameTime * i)) * 0.5f;

                glBegin(GL_QUADS);
                glColor4f(1.0f, 0.4f, 0.2f, 0.4f);
                glTexCoord2f(1, 0);
                glVertex3fv((ourPos - lineTheirPos).GetData());
                glTexCoord2f(1, 1);
                glVertex3fv((ourPos + lineTheirPos).GetData());
                glColor4f(1.0f, 0.4f, 0.2f, 0.2f);
                glTexCoord2f(0, 1);
                glVertex3fv((pos + lineTheirPos).GetData());
                glTexCoord2f(0, 0);
                glVertex3fv((pos - lineTheirPos).GetData());
                glEnd();
            }

            glDepthMask(true);
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_TEXTURE_2D);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_BLEND);
            glShadeModel(GL_FLAT);
        }
    }
}

#include "pch.h"

#include "FenceSwitchBuildingRenderer.h"
#include "switch.h"
#include "laserfence.h"
#include "matrix34.h"
#include "ShapeStatic.h"
#include "resource.h"
#include "camera.h"
#include "GameApp.h"
#include "location.h"
#include "team.h"
#include "clienttoserver.h"

void FenceSwitchBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    // No base-class RenderAlphas — FenceSwitch renders its own connections
    // instead of lights/ports.

    const FenceSwitch& sw = static_cast<const FenceSwitch&>(_building);

    // Connection to first linked fence
    Building* b = g_app->m_location->GetBuilding(sw.m_linkedBuildingId);
    if (b && b->m_type == Building::TypeLaserFence)
    {
        auto fence = static_cast<LaserFence*>(b);
        LegacyVector3 ourPos = const_cast<FenceSwitch&>(sw).GetConnectionLocation();
        RenderConnection(ourPos, fence->GetTopPosition(), sw.m_switchValue == sw.m_linkedBuildingId);
    }

    // Connection to second linked fence
    b = g_app->m_location->GetBuilding(sw.m_linkedBuildingId2);
    if (b && b->m_type == Building::TypeLaserFence)
    {
        auto fence = static_cast<LaserFence*>(b);
        LegacyVector3 ourPos = const_cast<FenceSwitch&>(sw).GetConnectionLocation();
        RenderConnection(ourPos, fence->GetTopPosition(), sw.m_switchValue == sw.m_linkedBuildingId2);
    }
}

void FenceSwitchBuildingRenderer::RenderConnection(const LegacyVector3& _ourPos, const LegacyVector3& _targetPos, bool _active)
{
    LegacyVector3 ourPos = _ourPos;
    LegacyVector3 theirPos = _targetPos;

    LegacyVector3 camToOurPos = g_app->m_camera->GetPos() - ourPos;
    LegacyVector3 ourPosRight = camToOurPos ^ (theirPos - ourPos);
    ourPosRight.SetLength(2.0f);

    LegacyVector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
    LegacyVector3 theirPosRight = camToTheirPos ^ (theirPos - ourPos);
    theirPosRight.SetLength(2.0f);

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(false);

    if (_active)
        glColor4f(0.9f, 0.9f, 0.5f, 1.0f);
    else
        glColor4f(0.5f, 0.5f, 0.5f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laser.bmp"));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

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
}

void FenceSwitchBuildingRenderer::RenderLights(const Building& _building)
{
    const FenceSwitch& sw = static_cast<const FenceSwitch&>(_building);

    if (sw.m_id.GetTeamId() != 255 && sw.m_lights.Size() > 0)
    {
        if ((g_app->m_clientToServer->m_lastValidSequenceIdFromServer % 10) / 2 == sw.m_id.GetTeamId() || g_app->m_editing)
        {
            for (int i = 0; i < sw.m_lights.Size(); ++i)
            {
                ShapeMarkerData* marker = *const_cast<LList<ShapeMarkerData*>&>(sw.m_lights).GetPointer(i);
                Matrix34 rootMat(sw.m_front, sw.m_up, sw.m_pos);
                Matrix34 worldMat = sw.m_shape->GetMarkerWorldMatrix(marker, rootMat);
                LegacyVector3 lightPos = worldMat.pos;

                float signalSize = 6.0f;
                LegacyVector3 camR = g_app->m_camera->GetRight();
                LegacyVector3 camU = g_app->m_camera->GetUp();

                if (sw.m_switchValue == sw.m_linkedBuildingId)
                    glColor3f(0.0f, 1.0f, 0.0f);
                else
                    glColor3f(1.0f, 0.0f, 0.0f);

                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/starburst.bmp"));
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glDisable(GL_CULL_FACE);
                glDepthMask(false);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);

                for (int j = 0; j < 10; ++j)
                {
                    float size = signalSize * static_cast<float>(j) / 10.0f;
                    glBegin(GL_QUADS);
                    glTexCoord2f(0.0f, 0.0f);
                    glVertex3fv((lightPos - camR * size - camU * size).GetData());
                    glTexCoord2f(1.0f, 0.0f);
                    glVertex3fv((lightPos + camR * size - camU * size).GetData());
                    glTexCoord2f(1.0f, 1.0f);
                    glVertex3fv((lightPos + camR * size + camU * size).GetData());
                    glTexCoord2f(0.0f, 1.0f);
                    glVertex3fv((lightPos - camR * size + camU * size).GetData());
                    glEnd();
                }

                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable(GL_BLEND);

                glDepthMask(true);
                glEnable(GL_CULL_FACE);
                glDisable(GL_TEXTURE_2D);
            }
        }
    }
}

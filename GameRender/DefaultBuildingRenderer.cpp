#include "pch.h"

#include "DefaultBuildingRenderer.h"
#include "building.h"
#include "camera.h"
#include "clienttoserver.h"
#include "GameApp.h"
#include "globals.h"
#include "landscape.h"
#include "location.h"
#include "matrix34.h"
#include "preferences.h"
#include "profiler.h"
#include "renderer.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "team.h"

void DefaultBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    if (_building.m_shape)
    {
        Matrix34 mat(_building.m_front, _building.m_up, _building.m_pos);
        _building.m_shape->Render(_ctx.predictionTime, mat);
    }
}

void DefaultBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    RenderLights(_building);
    RenderPorts(_building);
}

void DefaultBuildingRenderer::RenderLights(const Building& _building)
{
    if (_building.m_id.GetTeamId() != 255 && _building.m_lights.Size() > 0)
    {
        if ((g_context->m_clientToServer->m_lastValidSequenceIdFromServer % 10) / 2 == _building.m_id.GetTeamId()
            || g_context->m_editing)
        {
            for (int i = 0; i < _building.m_lights.Size(); ++i)
            {
                ShapeMarkerData* marker = _building.m_lights[i];
                Matrix34 rootMat(_building.m_front, _building.m_up, _building.m_pos);
                Matrix34 worldMat = _building.m_shape->GetMarkerWorldMatrix(marker, rootMat);
                LegacyVector3 lightPos = worldMat.pos;

                float signalSize = 6.0f;
                LegacyVector3 camR = g_context->m_camera->GetRight();
                LegacyVector3 camU = g_context->m_camera->GetUp();

                if (_building.m_id.GetTeamId() == 255)
                    glColor3f(0.5f, 0.5f, 0.5f);
                else
                    glColor3ubv(g_context->m_location->m_teams[_building.m_id.GetTeamId()].m_colour.GetData());

                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));
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

void DefaultBuildingRenderer::RenderPorts(const Building& _building)
{
    if (_building.m_ports.Size() == 0)
        return;

    START_PROFILE(g_context->m_profiler, "RenderPorts");

    int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail");

    for (int i = 0; i < _building.m_ports.Size(); ++i)
    {
        BuildingPort* port = _building.m_ports[i];
        LegacyVector3 portPos = port->m_mat.pos;
        LegacyVector3 portFront = port->m_mat.f;

        //
        // Render the port shape

        portPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(portPos.x, portPos.z) + 0.5f;
        LegacyVector3 portUp = g_upVector;
        Matrix34 mat(portFront, portUp, portPos);

        if (buildingDetail < 3)
        {
            g_context->m_renderer->SetObjectLighting();
            Building::s_controlPad->Render(0.0f, mat);
            g_context->m_renderer->UnsetObjectLighting();
        }

        //
        // Render the status light

        float size = 6.0f;

        LegacyVector3 camR = g_context->m_camera->GetRight() * size;
        LegacyVector3 camU = g_context->m_camera->GetUp() * size;

        LegacyVector3 statusPos = Building::s_controlPad->GetMarkerWorldMatrix(Building::s_controlPadStatus, mat).pos;

        if (port->m_occupant.IsValid())
            glColor4f(0.3f, 1.0f, 0.3f, 1.0f);
        else
            glColor4f(1.0f, 0.3f, 0.3f, 1.0f);

        glDisable(GL_CULL_FACE);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));
        glDepthMask(false);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3fv((statusPos - camR - camU).GetData());
        glTexCoord2i(1, 0);
        glVertex3fv((statusPos + camR - camU).GetData());
        glTexCoord2i(1, 1);
        glVertex3fv((statusPos + camR + camU).GetData());
        glTexCoord2i(0, 1);
        glVertex3fv((statusPos - camR + camU).GetData());
        glEnd();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
        glDepthMask(true);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_CULL_FACE);
    }

    END_PROFILE(g_context->m_profiler, "RenderPorts");
}

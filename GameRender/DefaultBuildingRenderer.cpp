#include "pch.h"

#include "DefaultBuildingRenderer.h"
#include "QuadBatcher.h"
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

void DefaultBuildingRenderer::RenderAlphas(const Building& _building, [[maybe_unused]] const BuildingRenderContext& _ctx)
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
            // Determine team color (constant for all lights on this building)
            uint32_t color;
            if (_building.m_id.GetTeamId() == 255)
                color = QuadBatcher::PackColorF(0.5f, 0.5f, 0.5f);
            else
            {
                const unsigned char* c = g_context->m_location->m_teams[_building.m_id.GetTeamId()].m_colour.GetData();
                color = QuadBatcher::PackColorBGRA(c[0], c[1], c[2]);
            }

            // GL state — set once for all lights
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

            LegacyVector3 camR = g_context->m_camera->GetRight();
            LegacyVector3 camU = g_context->m_camera->GetUp();

            auto& batcher = QuadBatcher::Get();

            for (int i = 0; i < _building.m_lights.Size(); ++i)
            {
                ShapeMarkerData* marker = _building.m_lights[i];
                Matrix34 rootMat(_building.m_front, _building.m_up, _building.m_pos);
                Matrix34 worldMat = _building.m_shape->GetMarkerWorldMatrix(marker, rootMat);
                LegacyVector3 lightPos = worldMat.pos;

                float signalSize = 6.0f;

                for (int j = 0; j < 10; ++j)
                {
                    float size = signalSize * static_cast<float>(j) / 10.0f;
                    LegacyVector3 r = camR * size;
                    LegacyVector3 u = camU * size;
                    LegacyVector3 p0 = lightPos - r - u;
                    LegacyVector3 p1 = lightPos + r - u;
                    LegacyVector3 p2 = lightPos + r + u;
                    LegacyVector3 p3 = lightPos - r + u;
                    batcher.Emit(
                        QuadBatcher::MakeVertex(p0.x, p0.y, p0.z, color, 0.f, 0.f),
                        QuadBatcher::MakeVertex(p1.x, p1.y, p1.z, color, 1.f, 0.f),
                        QuadBatcher::MakeVertex(p2.x, p2.y, p2.z, color, 1.f, 1.f),
                        QuadBatcher::MakeVertex(p3.x, p3.y, p3.z, color, 0.f, 1.f)
                    );
                }
            }

            batcher.Flush();

            // Restore GL state once
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_BLEND);
            glDepthMask(true);
            glEnable(GL_CULL_FACE);
            glDisable(GL_TEXTURE_2D);
        }
    }
}

void DefaultBuildingRenderer::RenderPorts(const Building& _building)
{
    if (_building.m_ports.Size() == 0)
        return;

    START_PROFILE(g_context->m_profiler, "RenderPorts");

    int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail");

    // Pass 1: Render all control pad shapes (opaque, with lighting)
    if (buildingDetail < 3)
    {
        for (int i = 0; i < _building.m_ports.Size(); ++i)
        {
            BuildingPort* port = _building.m_ports[i];
            LegacyVector3 portPos = port->m_mat.pos;
            portPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(portPos.x, portPos.z) + 0.5f;
            LegacyVector3 portUp = g_upVector;
            Matrix34 mat(port->m_mat.f, portUp, portPos);

            g_context->m_renderer->SetObjectLighting();
            Building::s_controlPad->Render(0.0f, mat);
            g_context->m_renderer->UnsetObjectLighting();
        }
    }

    // Pass 2: Batch all status light quads
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));
    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    auto& batcher = QuadBatcher::Get();

    for (int i = 0; i < _building.m_ports.Size(); ++i)
    {
        BuildingPort* port = _building.m_ports[i];
        LegacyVector3 portPos = port->m_mat.pos;
        portPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(portPos.x, portPos.z) + 0.5f;
        LegacyVector3 portUp = g_upVector;
        Matrix34 mat(port->m_mat.f, portUp, portPos);

        float size = 6.0f;
        LegacyVector3 camR = g_context->m_camera->GetRight() * size;
        LegacyVector3 camU = g_context->m_camera->GetUp() * size;

        LegacyVector3 statusPos = Building::s_controlPad->GetMarkerWorldMatrix(Building::s_controlPadStatus, mat).pos;

        uint32_t color;
        if (port->m_occupant.IsValid())
            color = QuadBatcher::PackColorF(0.3f, 1.0f, 0.3f, 1.0f);
        else
            color = QuadBatcher::PackColorF(1.0f, 0.3f, 0.3f, 1.0f);

        LegacyVector3 p0 = statusPos - camR - camU;
        LegacyVector3 p1 = statusPos + camR - camU;
        LegacyVector3 p2 = statusPos + camR + camU;
        LegacyVector3 p3 = statusPos - camR + camU;

        batcher.Emit(
            QuadBatcher::MakeVertex(p0.x, p0.y, p0.z, color, 0.f, 0.f),
            QuadBatcher::MakeVertex(p1.x, p1.y, p1.z, color, 1.f, 0.f),
            QuadBatcher::MakeVertex(p2.x, p2.y, p2.z, color, 1.f, 1.f),
            QuadBatcher::MakeVertex(p3.x, p3.y, p3.z, color, 0.f, 1.f)
        );
    }

    batcher.Flush();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glDepthMask(true);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);

    END_PROFILE(g_context->m_profiler, "RenderPorts");
}

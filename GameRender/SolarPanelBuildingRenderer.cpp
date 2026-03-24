#include "pch.h"

#include "SolarPanelBuildingRenderer.h"
#include "generator.h"
#include "camera.h"
#include "GameApp.h"
#include "location.h"
#include "landscape.h"
#include "main.h"
#include "matrix34.h"
#include "resource.h"
#include "ShapeStatic.h"

void SolarPanelBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    const auto& panel = static_cast<const SolarPanel&>(_building);

    if (g_context->m_editing)
    {
        // Editor live-update: keep orientation aligned to landscape normal.
        auto& mutablePanel = const_cast<SolarPanel&>(panel);
        mutablePanel.m_up = g_context->m_location->m_landscape.m_normalMap->GetValue(panel.m_pos.x, panel.m_pos.z);
        LegacyVector3 right(1, 0, 0);
        mutablePanel.m_front = right ^ mutablePanel.m_up;
    }

    glShadeModel(GL_SMOOTH);
    PowerBuildingRenderer::Render(_building, _ctx);
    glShadeModel(GL_FLAT);
}

void SolarPanelBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    PowerBuildingRenderer::RenderAlphas(_building, _ctx);

    const auto& panel = static_cast<const SolarPanel&>(_building);

    float fractionOccupied = static_cast<float>(const_cast<SolarPanel&>(panel).GetNumPortsOccupied())
                           / static_cast<float>(const_cast<SolarPanel&>(panel).GetNumPorts());

    if (fractionOccupied > 0.0f)
    {
        Matrix34 mat(panel.m_front, panel.m_up, panel.m_pos);
        float glowWidth = 60.0f;
        float glowHeight = 40.0f;
        float alphaValue = fabs(sinf(static_cast<float>(g_gameTime))) * fractionOccupied;

        glColor4f(0.2f, 0.4f, 0.9f, alphaValue);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/glow.bmp"));
        glDepthMask(false);
        glDisable(GL_CULL_FACE);

        for (int i = 0; i < SOLARPANEL_NUMGLOWS; ++i)
        {
            Matrix34 thisGlow = panel.m_shape->GetMarkerWorldMatrix(panel.m_glowMarker[i], mat);

            glBegin(GL_QUADS);
            glTexCoord2i(0, 0);
            glVertex3fv((thisGlow.pos - thisGlow.r * glowHeight + thisGlow.f * glowWidth).GetData());
            glTexCoord2i(0, 1);
            glVertex3fv((thisGlow.pos + thisGlow.r * glowHeight + thisGlow.f * glowWidth).GetData());
            glTexCoord2i(1, 1);
            glVertex3fv((thisGlow.pos + thisGlow.r * glowHeight - thisGlow.f * glowWidth).GetData());
            glTexCoord2i(1, 0);
            glVertex3fv((thisGlow.pos - thisGlow.r * glowHeight - thisGlow.f * glowWidth).GetData());
            glEnd();
        }

        glEnable(GL_CULL_FACE);
        glDepthMask(true);
        glDisable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
    }
}

void SolarPanelBuildingRenderer::RenderPorts(const Building& _building)
{
    const auto& panel = static_cast<const SolarPanel&>(_building);

    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));
    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    for (int i = 0; i < const_cast<SolarPanel&>(panel).GetNumPorts(); ++i)
    {
        Matrix34 rootMat(panel.m_front, panel.m_up, panel.m_pos);
        Matrix34 worldMat = panel.m_shape->GetMarkerWorldMatrix(panel.m_statusMarkers[i], rootMat);

        //
        // Render the status light

        float size = 6.0f;

        LegacyVector3 camR = g_context->m_camera->GetRight() * size;
        LegacyVector3 camU = g_context->m_camera->GetUp() * size;

        LegacyVector3 statusPos = worldMat.pos;

        if (const_cast<SolarPanel&>(panel).GetPortOccupant(i).IsValid())
            glColor4f(0.3f, 1.0f, 0.3f, 1.0f);
        else
            glColor4f(1.0f, 0.3f, 0.3f, 1.0f);

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
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glDepthMask(true);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
}

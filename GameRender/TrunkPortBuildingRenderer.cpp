#include "pch.h"

#include "TrunkPortBuildingRenderer.h"
#include "trunkport.h"
#include "matrix34.h"
#include "ShapeStatic.h"
#include "resource.h"
#include "hi_res_time.h"
#include "camera.h"
#include "GameApp.h"
#include "global_world.h"
#include "profiler.h"
#include "language_table.h"
#include "main.h"
#include "text_renderer.h"

void TrunkPortBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    DefaultBuildingRenderer::Render(_building, _ctx);

    const TrunkPort& port = static_cast<const TrunkPort&>(_building);

    const char* locationName = g_context->m_globalWorld->GetLocationNameTranslated(port.m_targetLocationId);
    std::string caption;
    if (locationName)
        caption = locationName;
    else
        caption = std::format("[{}]", LANGUAGEPHRASE("location_unknown"));

    START_PROFILE(g_context->m_profiler, "RenderDestination");

    float fontSize = 70.0f / caption.size();
    fontSize = std::min(fontSize, 10.0f);

    Matrix34 portMat(port.m_front, g_upVector, port.m_pos);

    Matrix34 destMat = port.m_shape->GetMarkerWorldMatrix(port.m_destination1, portMat);
    glColor4f(0.9f, 0.8f, 0.8f, 1.0f);
    g_gameFont.DrawText3D(destMat.pos, destMat.f, destMat.u, fontSize, "%s", caption.c_str());
    g_gameFont.SetRenderShadow(true);
    destMat.pos += destMat.f * 0.1f;
    destMat.pos += (destMat.f ^ destMat.u) * 0.2f;
    destMat.pos += destMat.u * 0.2f;
    glColor4f(0.9f, 0.8f, 0.8f, 0.0f);
    g_gameFont.DrawText3D(destMat.pos, destMat.f, destMat.u, fontSize, "%s", caption.c_str());

    g_gameFont.SetRenderShadow(false);
    glColor4f(0.9f, 0.8f, 0.8f, 1.0f);
    destMat = port.m_shape->GetMarkerWorldMatrix(port.m_destination2, portMat);
    g_gameFont.DrawText3D(destMat.pos, destMat.f, destMat.u, fontSize, "%s", caption.c_str());
    g_gameFont.SetRenderShadow(true);
    destMat.pos += destMat.f * 0.1f;
    destMat.pos += (destMat.f ^ destMat.u) * 0.2f;
    destMat.pos += destMat.u * 0.2f;
    glColor4f(0.9f, 0.8f, 0.8f, 0.0f);
    g_gameFont.DrawText3D(destMat.pos, destMat.f, destMat.u, fontSize, "%s", caption.c_str());

    g_gameFont.SetRenderShadow(false);

    END_PROFILE(g_context->m_profiler, "RenderDestination");
}

void TrunkPortBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    DefaultBuildingRenderer::RenderAlphas(_building, _ctx);

    const TrunkPort& port = static_cast<const TrunkPort&>(_building);

    if (port.m_openTimer > 0.0f)
    {
        ShapeMarkerData* marker = port.m_shape->GetMarkerData("MarkerSurface");
        DEBUG_ASSERT(marker);

        Matrix34 transform(port.m_front, g_upVector, port.m_pos);
        LegacyVector3 markerPos = port.m_shape->GetMarkerWorldMatrix(marker, transform).pos;
        float maxDistance = 40.0f;

        float timeOpen = GetHighResTime() - port.m_openTimer;
        float timeScale = (5 - timeOpen);
        if (timeScale < 1.0f)
            timeScale = 1.0f;

        //
        // Calculate dif map based on sine curves

        START_PROFILE(g_context->m_profiler, "Advance Heightmap");

        LegacyVector3 difMap[TRUNKPORT_HEIGHTMAP_MAXSIZE][TRUNKPORT_HEIGHTMAP_MAXSIZE];

        for (int x = 0; x < port.m_heightMapSize; ++x)
        {
            for (int z = 0; z < port.m_heightMapSize; ++z)
            {
                float centerDif = (port.m_heightMap[z * port.m_heightMapSize + x] - markerPos).Mag();
                float fractionOut = centerDif / maxDistance;
                if (fractionOut > 1.0f)
                    fractionOut = 1.0f;

                float wave1 = cosf(centerDif * 0.15f);
                float wave2 = cosf(centerDif * 0.05f);

                LegacyVector3 thisDif = port.m_front * sinf(static_cast<float>(g_gameTime) * 2.0f) * wave1 * (1.0f - fractionOut) * 15.0f * timeScale;
                thisDif += port.m_front * sinf(static_cast<float>(g_gameTime) * 2.5f) * wave2 * (1.0f - fractionOut) * 15.0f * timeScale;
                thisDif += g_upVector * cosf(static_cast<float>(g_gameTime)) * wave1 * timeScale * 10.0f * (1.0f - fractionOut);
                difMap[x][z] = thisDif;
            }
        }

        END_PROFILE(g_context->m_profiler, "Advance Heightmap");

        //
        // Render heightmap with dif map

        START_PROFILE(g_context->m_profiler, "Render Heightmap");

        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(false);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/laserfence.bmp"));

        float alphaValue = timeOpen;
        if (alphaValue > 0.7f)
            alphaValue = 0.7f;
        glColor4f(0.8f, 0.8f, 1.0f, alphaValue);

        for (int x = 0; x < port.m_heightMapSize - 1; ++x)
        {
            for (int z = 0; z < port.m_heightMapSize - 1; ++z)
            {
                LegacyVector3 thisPos1 = port.m_heightMap[z * port.m_heightMapSize + x] + difMap[x][z];
                LegacyVector3 thisPos2 = port.m_heightMap[z * port.m_heightMapSize + x + 1] + difMap[x + 1][z];
                LegacyVector3 thisPos3 = port.m_heightMap[(z + 1) * port.m_heightMapSize + x + 1] + difMap[x + 1][z + 1];
                LegacyVector3 thisPos4 = port.m_heightMap[(z + 1) * port.m_heightMapSize + x] + difMap[x][z + 1];

                float fractionX = static_cast<float>(x) / static_cast<float>(port.m_heightMapSize);
                float fractionZ = static_cast<float>(z) / static_cast<float>(port.m_heightMapSize);
                float width = 1.0f / port.m_heightMapSize;

                glBegin(GL_QUADS);
                glTexCoord2f(fractionX, fractionZ);
                glVertex3fv(thisPos1.GetData());
                glTexCoord2f(fractionX + width, fractionZ);
                glVertex3fv(thisPos2.GetData());
                glTexCoord2f(fractionX + width, fractionZ + width);
                glVertex3fv(thisPos3.GetData());
                glTexCoord2f(fractionX, fractionZ + width);
                glVertex3fv(thisPos4.GetData());
                glEnd();
            }
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/glow.bmp"));
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        for (int x = 0; x < port.m_heightMapSize - 1; ++x)
        {
            for (int z = 0; z < port.m_heightMapSize - 1; ++z)
            {
                LegacyVector3 thisPos1 = port.m_heightMap[z * port.m_heightMapSize + x] + difMap[x][z];
                LegacyVector3 thisPos2 = port.m_heightMap[z * port.m_heightMapSize + x + 1] + difMap[x + 1][z];
                LegacyVector3 thisPos3 = port.m_heightMap[(z + 1) * port.m_heightMapSize + x + 1] + difMap[x + 1][z + 1];
                LegacyVector3 thisPos4 = port.m_heightMap[(z + 1) * port.m_heightMapSize + x] + difMap[x][z + 1];

                float fractionX = static_cast<float>(x) / static_cast<float>(port.m_heightMapSize);
                float fractionZ = static_cast<float>(z) / static_cast<float>(port.m_heightMapSize);
                float width = 1.0f / port.m_heightMapSize;

                glBegin(GL_QUADS);
                glTexCoord2f(fractionX, fractionZ);
                glVertex3fv(thisPos1.GetData());
                glTexCoord2f(fractionX + width, fractionZ);
                glVertex3fv(thisPos2.GetData());
                glTexCoord2f(fractionX + width, fractionZ + width);
                glVertex3fv(thisPos3.GetData());
                glTexCoord2f(fractionX, fractionZ + width);
                glVertex3fv(thisPos4.GetData());
                glEnd();
            }
        }

        glDisable(GL_TEXTURE_2D);

        glDepthMask(true);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);

        END_PROFILE(g_context->m_profiler, "Render Heightmap");
    }
}

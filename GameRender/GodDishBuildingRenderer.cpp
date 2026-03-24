#include "pch.h"
#include "GodDishBuildingRenderer.h"

#include "goddish.h"
#include "camera.h"
#include "GameApp.h"
#include "globals.h"
#include "main.h"
#include "preferences.h"
#include "resource.h"

void GodDishBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    DefaultBuildingRenderer::RenderAlphas(_building, _ctx);

    auto& dish = static_cast<const GodDish&>(_building);

    LegacyVector3 camUp = g_context->m_camera->GetUp();
    LegacyVector3 camRight = g_context->m_camera->GetRight();

    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/cloudyglow.bmp"));
    glDisable(GL_DEPTH_TEST);

    float timeIndex = g_gameTime * 2;

    int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);
    int maxBlobs = 50;
    if (buildingDetail == 2)
        maxBlobs = 25;
    if (buildingDetail == 3)
        maxBlobs = 10;

    //
    // Calculate alpha value

    float alpha = dish.m_timer * 0.1f;
    alpha = std::min(alpha, 1.0f);

    //
    // Central glow effect

    for (int i = 0; i < maxBlobs; ++i)
    {
        LegacyVector3 pos = dish.m_centerPos;
        pos.x += sinf(timeIndex + i) * i * 1.7f;
        pos.y += fabs(cosf(timeIndex + i) * cosf(i * 20) * 64);
        pos.z += cosf(timeIndex + i) * i * 1.7f;

        float size = 20.0f * sinf(timeIndex + i * 13);
        size = std::max(size, 5.0f);

        glColor4f(0.6f, 0.2f, 0.1f, alpha);
        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3fv((pos - camRight * size + camUp * size).GetData());
        glTexCoord2i(1, 0);
        glVertex3fv((pos + camRight * size + camUp * size).GetData());
        glTexCoord2i(1, 1);
        glVertex3fv((pos + camRight * size - camUp * size).GetData());
        glTexCoord2i(0, 1);
        glVertex3fv((pos - camRight * size - camUp * size).GetData());
        glEnd();
    }

    //
    // Central starbursts

    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));

    int numStars = 10;
    if (buildingDetail == 2)
        numStars = 5;
    if (buildingDetail == 3)
        numStars = 2;

    for (int i = 0; i < numStars; ++i)
    {
        LegacyVector3 pos = dish.m_centerPos;
        pos.x += sinf(timeIndex + i) * i * 1.7f;
        pos.y += fabs(cosf(timeIndex + i) * cosf(i * 20) * 64);
        pos.z += cosf(timeIndex + i) * i * 1.7f;

        float size = i * 30.0f;

        glColor4f(1.0f, 0.4f, 0.2f, alpha);

        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3fv((pos - camRight * size + camUp * size).GetData());
        glTexCoord2i(1, 0);
        glVertex3fv((pos + camRight * size + camUp * size).GetData());
        glTexCoord2i(1, 1);
        glVertex3fv((pos + camRight * size - camUp * size).GetData());
        glTexCoord2i(0, 1);
        glVertex3fv((pos - camRight * size - camUp * size).GetData());
        glEnd();
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
}

#include "pch.h"

#include "ResearchItemBuildingRenderer.h"
#include "researchitem.h"
#include "matrix34.h"
#include "ShapeStatic.h"
#include "resource.h"
#include "preferences.h"
#include "camera.h"
#include "GameApp.h"
#include "global_world.h"
#include "main.h"
#include "text_renderer.h"

void ResearchItemBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    const ResearchItem& item = static_cast<const ResearchItem&>(_building);

    if (item.m_reprogrammed <= 0.0f)
        return;

    // Legacy rotation — mutates m_front/m_up each frame (same as original).
    LegacyVector3 rotateAround = g_upVector;
    rotateAround.RotateAroundX(g_gameTime * 1.0f);
    rotateAround.RotateAroundZ(g_gameTime * 0.7f);
    rotateAround.Normalise();

    const_cast<ResearchItem&>(item).m_front.RotateAround(rotateAround * g_advanceTime);
    const_cast<ResearchItem&>(item).m_up.RotateAround(rotateAround * g_advanceTime);

    LegacyVector3 predictedPos = item.m_pos + item.m_vel * _ctx.predictionTime;
    Matrix34 mat(item.m_front, item.m_up, predictedPos);

    item.m_shape->Render(0.0f, mat);

    if (g_app->m_editing && item.m_researchType != -1)
    {
        g_gameFont.DrawText3DCenter(predictedPos + LegacyVector3(0, 25, 0), 5, GlobalResearch::GetTypeName(item.m_researchType));
        g_gameFont.DrawText3DCenter(predictedPos + LegacyVector3(0, 20, 0), 5, "%2.2f", item.m_reprogrammed);
    }
}

void ResearchItemBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    DefaultBuildingRenderer::RenderAlphas(_building, _ctx);

    const ResearchItem& item = static_cast<const ResearchItem&>(_building);

    LegacyVector3 camUp = g_app->m_camera->GetUp();
    LegacyVector3 camRight = g_app->m_camera->GetRight();

    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/cloudyglow.bmp"));

    float timeIndex = g_gameTime + item.m_id.GetUniqueId() * 10.0f;

    int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);
    int maxBlobs = 20;
    if (buildingDetail == 2)
        maxBlobs = 10;
    if (buildingDetail == 3)
        maxBlobs = 0;

    float alpha = 1.0f;

    for (int i = 0; i < maxBlobs; ++i)
    {
        LegacyVector3 pos = item.m_centerPos;
        pos.x += sinf(timeIndex + i) * i * 0.3f;
        pos.y += cosf(timeIndex + i) * sinf(i * 10) * 5;
        pos.z += cosf(timeIndex + i) * i * 0.3f;

        float size = 5.0f + sinf(timeIndex + i * 10) * 7.0f;
        size = max(size, 2.0f);

        glColor4f(0.1f, 0.2f, 0.8f, alpha);

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
    // Starbursts

    alpha = 1.0f - item.m_reprogrammed / 100.0f;
    alpha *= 0.3f;

    glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/starburst.bmp"));

    if (alpha > 0.0f)
    {
        int numStars = 10;
        if (buildingDetail == 2)
            numStars = 5;
        if (buildingDetail == 3)
            numStars = 2;

        for (int i = 0; i < numStars; ++i)
        {
            LegacyVector3 pos = item.m_centerPos;
            pos.x += sinf(timeIndex + i) * i * 0.3f;
            pos.y += (cosf(timeIndex + i) * cosf(i * 10) * 2);
            pos.z += cosf(timeIndex + i) * i * 0.3f;

            float size = i * 10 * alpha;
            if (i > numStars - 2)
                size = i * 20 * alpha;

            glColor4f(0.1f, 0.2f, 0.8f, alpha);

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
    }

    //
    // Draw control line to heaven

    alpha = 1.0f - item.m_reprogrammed / 100.0f;
    alpha *= (0.5f + fabsf(cosf(g_gameTime)) * 0.5f);

    if (alpha > 0.0f)
    {
        glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laser.bmp"));
        glDisable(GL_CULL_FACE);
        glShadeModel(GL_SMOOTH);

        float w = 10.0f * alpha;

        glBegin(GL_QUADS);
        glColor4f(0.1f, 0.2f, 0.8f, alpha);
        glTexCoord2i(0, 0);
        glVertex3fv((item.m_pos + LegacyVector3(0, -50, 0) - camRight * w).GetData());
        glTexCoord2i(0, 1);
        glVertex3fv((item.m_pos + LegacyVector3(0, -50, 0) + camRight * w).GetData());

        glColor4f(0.1f, 0.2f, 0.8f, 0.0f);
        glTexCoord2i(1, 1);
        glVertex3fv((item.m_pos + LegacyVector3(0, 1000, 0) + camRight * w).GetData());
        glTexCoord2i(1, 0);
        glVertex3fv((item.m_pos + LegacyVector3(0, 1000, 0) - camRight * w).GetData());
        glEnd();

        w *= 0.3f;

        glBegin(GL_QUADS);
        glColor4f(0.1f, 0.2f, 0.8f, alpha);
        glTexCoord2i(0, 0);
        glVertex3fv((item.m_pos + LegacyVector3(0, -50, 0) - camRight * w).GetData());
        glTexCoord2i(0, 1);
        glVertex3fv((item.m_pos + LegacyVector3(0, -50, 0) + camRight * w).GetData());

        glColor4f(0.1f, 0.2f, 0.8f, 0.0f);
        glTexCoord2i(1, 1);
        glVertex3fv((item.m_pos + LegacyVector3(0, 1000, 0) + camRight * w).GetData());
        glTexCoord2i(1, 0);
        glVertex3fv((item.m_pos + LegacyVector3(0, 1000, 0) - camRight * w).GetData());
        glEnd();
    }

    glShadeModel(GL_FLAT);
    glDisable(GL_TEXTURE_2D);
    glDepthMask(true);
}

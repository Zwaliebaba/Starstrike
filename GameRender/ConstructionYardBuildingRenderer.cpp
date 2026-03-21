#include "pch.h"

#include "ConstructionYardBuildingRenderer.h"
#include "constructionyard.h"
#include "main.h"
#include "camera.h"
#include "resource.h"
#include "preferences.h"
#include "ShapeStatic.h"
#include "GameApp.h"

void ConstructionYardBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    DefaultBuildingRenderer::Render(_building, _ctx);

    auto& yard = static_cast<const ConstructionYard&>(_building);

    // Rung 1
    Matrix34 mat = const_cast<ConstructionYard&>(yard).GetRungMatrix1();
    yard.m_rung->Render(_ctx.predictionTime, mat);

    // Rung 2
    mat = const_cast<ConstructionYard&>(yard).GetRungMatrix2();
    yard.m_rung->Render(_ctx.predictionTime, mat);

    //
    // Primitives

    for (int i = 0; i < yard.m_numPrimitives; ++i)
    {
        Matrix34 buildingMat(yard.m_front, g_upVector, yard.m_pos);
        Matrix34 prim = yard.m_shape->GetMarkerWorldMatrix(yard.m_primitives[i], buildingMat);
        prim.pos.y += sinf(g_gameTime + i) * 5.0f;

        yard.m_primitive->Render(_ctx.predictionTime, prim);
    }
}

void ConstructionYardBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    DefaultBuildingRenderer::RenderAlphas(_building, _ctx);

    auto& yard = static_cast<const ConstructionYard&>(_building);

    LegacyVector3 camUp = g_app->m_camera->GetUp();
    LegacyVector3 camRight = g_app->m_camera->GetRight();

    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/cloudyglow.bmp"));

    float timeIndex = g_gameTime * 2;

    int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);
    int maxBlobs = 50;
    if (buildingDetail == 2)
        maxBlobs = 25;
    if (buildingDetail == 3)
        maxBlobs = 10;

    //
    // Calculate alpha value — smoothed over time.
    // NOTE: this mutates m_alpha on the simulation object (legacy coupling).

    float targetAlpha = 0.0f;
    if (yard.m_timer > 0.0f)
        targetAlpha = 1.0f - (yard.m_timer / 60.0f);
    else if (yard.m_numPrimitives > 0)
        targetAlpha = (yard.m_numPrimitives / 9.0f) * 0.5f;
    if (yard.m_numSurges > 0)
        targetAlpha = max(targetAlpha, 0.2f);

    float factor1 = g_advanceTime;
    float factor2 = 1.0f - factor1;
    const_cast<ConstructionYard&>(yard).m_alpha = yard.m_alpha * factor2 + targetAlpha * factor1;

    float alpha = yard.m_alpha;

    //
    // Central glow effect

    for (int i = 0; i < maxBlobs; ++i)
    {
        LegacyVector3 pos = _building.m_centerPos;
        pos.x += sinf(timeIndex + i) * i * 1.7f;
        pos.y += fabs(cosf(timeIndex + i) * cosf(i * 20) * 64);
        pos.z += cosf(timeIndex + i) * i * 1.7f;

        float size = 30.0f * sinf(timeIndex + i * 13);
        size = max(size, 5.0f);

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

    glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/starburst.bmp"));

    int numStars = 10;
    if (buildingDetail == 2)
        numStars = 5;
    if (buildingDetail == 3)
        numStars = 2;

    for (int i = 0; i < numStars; ++i)
    {
        LegacyVector3 pos = _building.m_centerPos;
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

    //
    // Starbursts on rungs

    if (yard.m_timer > 0.0f)
    {
        glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/starburst.bmp"));

        for (int r = 0; r < 2; ++r)
        {
            Matrix34 rungMat;
            if (r == 0)
                rungMat = const_cast<ConstructionYard&>(yard).GetRungMatrix1();
            else
                rungMat = const_cast<ConstructionYard&>(yard).GetRungMatrix2();

            for (int i = 0; i < YARD_NUMRUNGSPIKES; ++i)
            {
                Matrix34 spikeMat = yard.m_rung->GetMarkerWorldMatrix(yard.m_rungSpikes[i], rungMat);
                LegacyVector3 pos = spikeMat.pos;

                for (int j = 0; j < numStars; ++j)
                {
                    float size = sinf(timeIndex + r + i) * j * 5.0f;
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
            }
        }
    }

    glDisable(GL_TEXTURE_2D);
    glDepthMask(true);
}

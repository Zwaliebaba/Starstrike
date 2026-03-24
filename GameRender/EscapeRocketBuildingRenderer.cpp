#include "pch.h"

#include "EscapeRocketBuildingRenderer.h"
#include "rocket.h"
#include "main.h"
#include "camera.h"
#include "resource.h"
#include "preferences.h"
#include "ShapeStatic.h"
#include "GameApp.h"

void EscapeRocketBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    auto& rocket = static_cast<const EscapeRocket&>(_building);

    LegacyVector3 predictedPos = rocket.m_pos + rocket.m_vel * _ctx.predictionTime;
    Matrix34 mat(rocket.m_front, rocket.m_up, predictedPos);

    rocket.m_shape->Render(0.0f, mat);
}

void EscapeRocketBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    FuelBuildingRenderer::RenderAlphas(_building, _ctx);

    auto& rocket = static_cast<const EscapeRocket&>(_building);

    LegacyVector3 predictedPos = rocket.m_pos + rocket.m_vel * _ctx.predictionTime;

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    //
    // Render rocket glow

    float alpha = rocket.m_fuel / 100.0f;
    alpha = std::min(alpha, 1.0f);
    alpha = std::max(alpha, 0.0f);

    if (alpha > 0.0f)
    {
        LegacyVector3 camUp = g_context->m_camera->GetUp();
        LegacyVector3 camRight = g_context->m_camera->GetRight() * 0.75f;

        glDepthMask(false);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));

        float timeIndex = g_gameTime * 2;

        int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);
        int maxBlobs = 50;
        if (buildingDetail == 2)
            maxBlobs = 20;
        if (buildingDetail == 3)
            maxBlobs = 10;

        LegacyVector3 boosterPos = predictedPos;
        boosterPos.y += 100;

        //
        // Central glow effect

        for (int i = 30; i < maxBlobs; ++i)
        {
            LegacyVector3 pos = boosterPos;
            pos.x += sinf(timeIndex * 0.5f + i) * i * 3.7f;
            pos.y += cosf(timeIndex * 0.5f + i) * cosf(i * 20) * 50;
            pos.y += 40;
            pos.z += cosf(timeIndex * 0.3f + i) * i * 3.7f;

            float size = 20.0f * sinf(timeIndex + i * 2);
            size = std::max(size, 5.0f);

            for (int j = 0; j < 2; ++j)
            {
                size *= 0.75f;
                glColor4f(1.0f, 0.6f, 0.2f, alpha);
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
        // Central starbursts

        glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));

        int numStars = 10;
        if (buildingDetail == 2)
            numStars = 5;
        if (buildingDetail == 3)
            numStars = 2;

        for (int i = 8; i < numStars; ++i)
        {
            LegacyVector3 pos = boosterPos;
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
}

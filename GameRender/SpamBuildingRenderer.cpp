#include "pch.h"

#include "SpamBuildingRenderer.h"
#include "spam.h"
#include "matrix34.h"
#include "ShapeStatic.h"
#include "resource.h"
#include "preferences.h"
#include "camera.h"
#include "GameApp.h"
#include "main.h"

void SpamBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    const Spam& spam = static_cast<const Spam&>(_building);

    // Legacy rotation — mutates m_front/m_up each frame (same as original).
    LegacyVector3 rotateAround = g_upVector;
    rotateAround.RotateAroundX(g_gameTime * 1.0f);
    rotateAround.RotateAroundZ(g_gameTime * 0.7f);
    rotateAround.Normalise();

    const_cast<Spam&>(spam).m_front.RotateAround(rotateAround * g_advanceTime);
    const_cast<Spam&>(spam).m_up.RotateAround(rotateAround * g_advanceTime);

    LegacyVector3 predictedPos = spam.m_pos + spam.m_vel * _ctx.predictionTime;
    Matrix34 mat(spam.m_front, spam.m_up, predictedPos);

    spam.m_shape->Render(0.0f, mat);
}

void SpamBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    const Spam& spam = static_cast<const Spam&>(_building);

    LegacyVector3 camUp = g_app->m_camera->GetUp();
    LegacyVector3 camRight = g_app->m_camera->GetRight();

    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/cloudyglow.bmp"));

    float timeIndex = g_gameTime + spam.m_id.GetUniqueId() * 10.0f;

    int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);
    int maxBlobs = 20;
    if (buildingDetail == 2)
        maxBlobs = 10;
    if (buildingDetail == 3)
        maxBlobs = 0;

    float alpha = 1.0f;

    LegacyVector3 predictedPos = spam.m_pos + spam.m_vel * _ctx.predictionTime;
    LegacyVector3 centerToMpos = spam.m_pos - spam.m_centerPos;

    for (int i = 0; i < maxBlobs; ++i)
    {
        LegacyVector3 pos = predictedPos + centerToMpos;
        pos.x += sinf(timeIndex + i) * i * 0.3f;
        pos.y += cosf(timeIndex + i) * sinf(i * 10) * 5;
        pos.z += cosf(timeIndex + i) * i * 0.3f;

        float size = 5.0f + sinf(timeIndex + i * 10) * 7.0f;
        size = max(size, 2.0f);

        glColor4f(0.9f, 0.2f, 0.2f, alpha);

        if (spam.m_research)
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

    alpha = 1.0f - spam.m_timer / (SPAM_RELOADTIME - (SPAM_RELOADTIME / 2.0f) * g_app->m_difficultyLevel / 10.0f);
    alpha *= 0.3f;

    glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/starburst.bmp"));

    int numStars = 10;
    if (buildingDetail == 2)
        numStars = 5;
    if (buildingDetail == 3)
        numStars = 2;

    for (int i = 0; i < numStars; ++i)
    {
        LegacyVector3 pos = predictedPos + centerToMpos;
        pos.x += sinf(timeIndex + i) * i * 0.3f;
        pos.y += (cosf(timeIndex + i) * cosf(i * 10) * 2);
        pos.z += cosf(timeIndex + i) * i * 0.3f;

        float size = i * 10 * alpha;
        if (i > numStars - 2)
            size = i * 20 * alpha;

        glColor4f(0.8f, 0.2f, 0.2f, alpha);

        if (spam.m_research)
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

    glDisable(GL_TEXTURE_2D);
    glDepthMask(true);
}

#include "pch.h"

#include "PowerBuildingRenderer.h"
#include "generator.h"
#include "camera.h"
#include "GameApp.h"
#include "location.h"
#include "resource.h"

void PowerBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    DefaultBuildingRenderer::RenderAlphas(_building, _ctx);

    const auto& power = static_cast<const PowerBuilding&>(_building);

    Building* powerLink = g_context->m_location->GetBuilding(power.m_powerLink);
    if (powerLink)
    {
        //
        // Render the power line itself
        auto* powerBuilding = static_cast<PowerBuilding*>(powerLink);

        LegacyVector3 ourPos = const_cast<PowerBuilding&>(power).GetPowerLocation();
        LegacyVector3 theirPos = powerBuilding->GetPowerLocation();

        LegacyVector3 camToOurPos = g_context->m_camera->GetPos() - ourPos;
        LegacyVector3 ourPosRight = camToOurPos ^ (theirPos - ourPos);
        ourPosRight.SetLength(2.0f);

        LegacyVector3 camToTheirPos = g_context->m_camera->GetPos() - theirPos;
        LegacyVector3 theirPosRight = camToTheirPos ^ (theirPos - ourPos);
        theirPosRight.SetLength(2.0f);

        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(false);
        glColor4f(0.9f, 0.9f, 0.5f, 1.0f);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/laser.bmp"));
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

        //
        // Render any surges

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));

        float surgeSize = 25.0f;
        glColor4f(0.5f, 0.5f, 1.0f, 1.0f);
        LegacyVector3 camUp = g_context->m_camera->GetUp() * surgeSize;
        LegacyVector3 camRight = g_context->m_camera->GetRight() * surgeSize;
        glBegin(GL_QUADS);
        for (int i = 0; i < power.m_surges.Size(); ++i)
        {
            float thisSurge = power.m_surges[i];
            thisSurge += _ctx.predictionTime * 2.0f;
            if (thisSurge < 0.0f)
                thisSurge = 0.0f;
            if (thisSurge > 1.0f)
                thisSurge = 1.0f;
            LegacyVector3 thisSurgePos = ourPos + (theirPos - ourPos) * thisSurge;
            glTexCoord2i(0, 0);
            glVertex3fv((thisSurgePos - camUp - camRight).GetData());
            glTexCoord2i(1, 0);
            glVertex3fv((thisSurgePos - camUp + camRight).GetData());
            glTexCoord2i(1, 1);
            glVertex3fv((thisSurgePos + camUp + camRight).GetData());
            glTexCoord2i(0, 1);
            glVertex3fv((thisSurgePos + camUp - camRight).GetData());
        }
        glEnd();

        glDisable(GL_TEXTURE_2D);
        glDepthMask(true);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }
}

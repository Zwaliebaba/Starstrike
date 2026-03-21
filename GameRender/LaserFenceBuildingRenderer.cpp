#include "pch.h"

#include "LaserFenceBuildingRenderer.h"
#include "laserfence.h"
#include "matrix34.h"
#include "ShapeStatic.h"
#include "resource.h"
#include "preferences.h"
#include "ogl_extensions.h"
#include "main.h"
#include "GameApp.h"
#include "location.h"
#include "team.h"

void LaserFenceBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    const LaserFence& fence = static_cast<const LaserFence&>(_building);

    if (fence.m_shape)
    {
        Matrix34 mat(fence.m_front, g_upVector, fence.m_pos);
        mat.f *= fence.m_scale;
        mat.u *= fence.m_scale;
        mat.r *= fence.m_scale;

        glEnable(GL_NORMALIZE);
        fence.m_shape->Render(_ctx.predictionTime, mat);
        glDisable(GL_NORMALIZE);
    }
}

void LaserFenceBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    // Base class handles RenderLights + RenderPorts; we override RenderLights
    // to suppress the default starburst, so this only calls RenderPorts.
    DefaultBuildingRenderer::RenderAlphas(_building, _ctx);

    const LaserFence& fence = static_cast<const LaserFence&>(_building);

    if (fence.m_mode == LaserFence::ModeDisabled || fence.m_mode == LaserFence::ModeNeverOn)
        return;

    if (fence.m_nextLaserFenceId == -1)
        return;

    //
    // Draw the laser fence connecting to the next laser fence

    Building* nextBuilding = g_app->m_location->GetBuilding(fence.m_nextLaserFenceId);
    if (!nextBuilding || nextBuilding->m_type != Building::TypeLaserFence)
    {
        // Original code mutates m_nextLaserFenceId = -1 here as a
        // render-path cleanup.  We skip that mutation to keep the
        // renderer side-effect-free; Advance() handles fence linkage.
        return;
    }
    auto nextFence = static_cast<LaserFence*>(nextBuilding);

    int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(false);

    unsigned char alpha = 150;
    if (fence.m_id.GetTeamId() == 255)
        glColor4ub(100, 100, 100, alpha);
    else
    {
        RGBAColour* colour = &g_app->m_location->m_teams[fence.m_id.GetTeamId()].m_colour;
        glColor4ub(colour->r, colour->g, colour->b, alpha);
    }

    float ourFenceMaxHeight = const_cast<LaserFence&>(fence).GetFenceFullHeight();
    float theirFenceMaxHeight = nextFence->GetFenceFullHeight();
    float predictedStatus = fence.m_status;
    if (fence.m_mode == LaserFence::ModeDisabling)
        predictedStatus -= LASERFENCE_RAISESPEED * _ctx.predictionTime;
    if (fence.m_mode == LaserFence::ModeEnabling)
        predictedStatus += LASERFENCE_RAISESPEED * _ctx.predictionTime;
    if (g_app->m_editing)
        predictedStatus = 1.0f;

    float ourFenceHeight = ourFenceMaxHeight * predictedStatus;
    float theirFenceHeight = theirFenceMaxHeight * predictedStatus;
    float distance = (fence.m_pos - nextBuilding->m_pos).Mag();
    float dx = distance / (ourFenceMaxHeight * 2.0f);
    float dz = ourFenceHeight / ourFenceMaxHeight;
    float timeOff = g_gameTime / 15.0f;

    if (buildingDetail < 3)
    {
        glEnable(GL_TEXTURE_2D);

        gglActiveTextureARB(GL_TEXTURE0_ARB);
        glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laserfence.bmp"));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
        glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE);
        glEnable(GL_TEXTURE_2D);

        gglActiveTextureARB(GL_TEXTURE1_ARB);
        glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laserfence2.bmp"));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
        glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
        glEnable(GL_TEXTURE_2D);

        glBegin(GL_QUADS);
        gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, timeOff, 0.0f);
        gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0, 0);
        glVertex3fv((fence.m_pos - LegacyVector3(0, ourFenceHeight / 3, 0)).GetData());

        gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, timeOff, dz);
        gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0, 1);
        glVertex3fv((fence.m_pos + LegacyVector3(0, ourFenceHeight, 0)).GetData());

        gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, timeOff + dx, dz);
        gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1, 1);
        glVertex3fv((nextBuilding->m_pos + LegacyVector3(0, theirFenceHeight, 0)).GetData());

        gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, timeOff + dx, 0.0f);
        gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1, 0);
        glVertex3fv((nextBuilding->m_pos - LegacyVector3(0, theirFenceHeight / 3, 0)).GetData());
        glEnd();

        gglActiveTextureARB(GL_TEXTURE1_ARB);
        glDisable(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        gglActiveTextureARB(GL_TEXTURE0_ARB);
        glDisable(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    //
    // Blend another poly over the top for burn effect

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laserfence2.bmp"));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex3fv((fence.m_pos - LegacyVector3(0, ourFenceHeight / 3, 0)).GetData());

    glTexCoord2f(0, 1);
    glVertex3fv((fence.m_pos + LegacyVector3(0, ourFenceHeight, 0)).GetData());

    glTexCoord2f(1, 1);
    glVertex3fv((nextBuilding->m_pos + LegacyVector3(0, theirFenceHeight, 0)).GetData());

    glTexCoord2f(1, 0);
    glVertex3fv((nextBuilding->m_pos - LegacyVector3(0, theirFenceHeight / 3, 0)).GetData());
    glEnd();

    glDisable(GL_TEXTURE_2D);

    //
    // Gimme a line across the top
    glEnable(GL_LINE_SMOOTH);

    glBegin(GL_LINES);
    glVertex3fv((fence.m_pos + LegacyVector3(0, ourFenceHeight, 0)).GetData());
    glVertex3fv((nextBuilding->m_pos + LegacyVector3(0, theirFenceHeight, 0)).GetData());
    glEnd();

    glDepthMask(true);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glDisable(GL_LINE_SMOOTH);
}

void LaserFenceBuildingRenderer::RenderLights(const Building& /*_building*/)
{
    // Intentionally empty — LaserFence suppresses the default team-ownership
    // starburst lights rendered by DefaultBuildingRenderer::RenderLights.
}

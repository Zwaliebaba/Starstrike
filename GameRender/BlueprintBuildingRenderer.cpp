#include "pch.h"

#include "BlueprintBuildingRenderer.h"
#include "blueprintstore.h"
#include "camera.h"
#include "GameApp.h"
#include "location.h"
#include "main.h"
#include "resource.h"
#include "ShapeStatic.h"

// ---------------------------------------------------------------------------
// BlueprintBuildingRenderer::Render
// ---------------------------------------------------------------------------

void BlueprintBuildingRenderer::Render(const Building& _building,
                                       const BuildingRenderContext& _ctx)
{
    auto& bp = static_cast<const BlueprintBuilding&>(_building);

    // Velocity-predicted position with forced g_upVector orientation.
    LegacyVector3 pos = bp.m_pos + bp.m_vel * _ctx.predictionTime;
    Matrix34 mat(bp.m_front, g_upVector, pos);
    bp.m_shape->Render(_ctx.predictionTime, mat);
}

// ---------------------------------------------------------------------------
// BlueprintBuildingRenderer::RenderAlphas
// ---------------------------------------------------------------------------

void BlueprintBuildingRenderer::RenderAlphas(const Building& _building,
                                              const BuildingRenderContext& _ctx)
{
    // Lights + ports via default path.
    DefaultBuildingRenderer::RenderAlphas(_building, _ctx);

    auto& bp = static_cast<const BlueprintBuilding&>(_building);

    auto link = static_cast<BlueprintBuilding*>(
        g_context->m_location->GetBuilding(bp.m_buildingLink));
    if (link)
    {
        float infected = bp.m_infected / 100.0f;
        float linkInfected = link->m_infected / 100.0f;
        float ourTime = g_gameTime + bp.m_id.GetUniqueId() + bp.m_id.GetIndex();
        if (fabs(infected - linkInfected) < 0.01f)
            glColor4f(infected, 0.7f - infected * 0.7f, 0.0f,
                      0.1f + fabs(sinf(ourTime)) * 0.2f);
        else
            glColor4f(infected, 0.7f - infected * 0.7f, 0.0f,
                      0.5f + fabs(sinf(ourTime)) * 0.5f);

        LegacyVector3 ourPos =
            const_cast<BlueprintBuilding&>(bp).GetMarker(_ctx.predictionTime).pos;
        LegacyVector3 theirPos = link->GetMarker(_ctx.predictionTime).pos;

        LegacyVector3 rightAngle =
            (g_context->m_camera->GetPos() - ourPos) ^ (theirPos - ourPos);
        rightAngle.SetLength(20.0f);

        glDisable(GL_CULL_FACE);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D,
                      Resource::GetTexture("textures/laser.bmp"));

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(false);

        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3fv(const_cast<float*>((ourPos - rightAngle).GetData()));
        glTexCoord2i(0, 1);
        glVertex3fv(const_cast<float*>((ourPos + rightAngle).GetData()));
        glTexCoord2i(1, 1);
        glVertex3fv(const_cast<float*>((theirPos + rightAngle).GetData()));
        glTexCoord2i(1, 0);
        glVertex3fv(const_cast<float*>((theirPos - rightAngle).GetData()));
        glEnd();

        glDepthMask(true);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_TEXTURE_2D);
    }
}

#include "pch.h"
#include "FuelBuildingRenderer.h"
#include "GameApp.h"
#include "ShapeStatic.h"
#include "camera.h"
#include "main.h"
#include "preferences.h"
#include "renderer.h"
#include "resource.h"
#include "rocket.h"

void FuelBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    DefaultBuildingRenderer::Render(_building, _ctx);

    auto& fuel = static_cast<const FuelBuilding&>(_building);
    FuelBuilding* fuelBuilding = fuel.GetLinkedBuilding();
    if (fuelBuilding)
    {
        LegacyVector3 ourPipePos = fuel.GetFuelPosition();
        LegacyVector3 theirPipePos = fuelBuilding->GetFuelPosition();

        LegacyVector3 pipeVector = (theirPipePos - ourPipePos).Normalise();
        LegacyVector3 right = pipeVector ^ g_upVector;
        LegacyVector3 up = pipeVector ^ right;

        ourPipePos += pipeVector * 10;

        Matrix34 pipeMat(up, pipeVector, ourPipePos);
        DEBUG_ASSERT(FuelBuilding::s_fuelPipe);
        FuelBuilding::s_fuelPipe->Render(_ctx.predictionTime, pipeMat);
    }
}

void FuelBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    DefaultBuildingRenderer::RenderAlphas(_building, _ctx);

    auto& fuel = static_cast<const FuelBuilding&>(_building);

    if (fuel.m_currentLevel > 0.0f)
    {
        FuelBuilding* fuelBuilding = fuel.GetLinkedBuilding();
        if (fuelBuilding)
        {
            LegacyVector3 startPos = fuel.GetFuelPosition();
            LegacyVector3 endPos = fuelBuilding->GetFuelPosition();

            LegacyVector3 midPos = (startPos + endPos) / 2.0f;
            LegacyVector3 rightAngle = (g_context->m_camera->GetPos() - midPos) ^ (startPos - endPos);
            rightAngle.SetLength(25.0f);

            glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/fuel.bmp"));
            glEnable(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(false);

            float tx = g_gameTime * -0.5f;
            float tw = 1.0f;

            glColor4f(1.0f, 0.4f, 0.1f, 0.4f * fuel.m_currentLevel);

            float nearPlaneStart = g_context->m_renderer->GetNearPlane();
            g_context->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.2f, g_context->m_renderer->GetFarPlane());

            int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail");
            float maxLoops = 4 - buildingDetail;
            maxLoops = std::max(maxLoops, 1.0f);
            maxLoops = std::min(maxLoops, 3.0f);

            for (int i = 0; i < maxLoops; ++i)
            {
                glBegin(GL_QUADS);
                glTexCoord2f(tx, 0);
                glVertex3fv((startPos - rightAngle).GetData());
                glTexCoord2f(tx, 1);
                glVertex3fv((startPos + rightAngle).GetData());
                glTexCoord2f(tx + tw, 1);
                glVertex3fv((endPos + rightAngle).GetData());
                glTexCoord2f(tx + tw, 0);
                glVertex3fv((endPos - rightAngle).GetData());
                glEnd();
                rightAngle *= 0.7f;
            }

            g_context->m_camera->SetupProjectionMatrix(nearPlaneStart, g_context->m_renderer->GetFarPlane());

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_TEXTURE_2D);
        }
    }
}

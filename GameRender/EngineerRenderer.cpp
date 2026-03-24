#include "pch.h"

#include "EngineerRenderer.h"
#include "ShadowRenderer.h"
#include "engineer.h"
#include "ShapeStatic.h"
#include "matrix34.h"
#include "renderer.h"
#include "resource.h"
#include "GameApp.h"
#include "camera.h"
#include "location.h"
#include "main.h"
#include "controltower.h"
#include "bridge.h"
#include "researchitem.h"

void EngineerRenderer::RenderShape(const Engineer& _engineer, float _predictionTime)
{
    LegacyVector3 predictedPos = _engineer.m_pos + _engineer.m_vel * _predictionTime;
    if (_engineer.m_onGround)
    {
        predictedPos.y = std::max(g_context->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z), 0.0f) +
            _engineer.m_hoverHeight;
    }

    LegacyVector3 entityUp = g_upVector;
    LegacyVector3 entityFront = _engineer.m_front;
    entityFront.y *= 0.5f;
    entityFront.Normalise();
    LegacyVector3 entityRight = entityFront ^ entityUp;
    entityUp = entityRight ^ entityFront;

    g_context->m_renderer->SetObjectLighting();

    glEnable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_BLEND);

    if (entityFront.y > 0.5f)
        entityFront.Set(1, 0, 0);
    Matrix34 mat(entityFront, entityUp, predictedPos);
    _engineer.m_shape->Render(_predictionTime, mat);

    glEnable(GL_BLEND);
    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_TEXTURE_2D);
    g_context->m_renderer->UnsetObjectLighting();
    glEnable(GL_CULL_FACE);
}

void EngineerRenderer::RenderLaser(const Engineer& _engineer, float _predictionTime)
{
    LegacyVector3 fromPos = _engineer.m_pos;
    LegacyVector3 toPos;

    Building* building = g_context->m_location->GetBuilding(_engineer.m_buildingId);
    if (building)
    {
        if (building->m_type == Building::TypeControlTower)
        {
            auto tower = static_cast<ControlTower*>(building);
            tower->GetConsolePosition(_engineer.m_positionId, toPos);
        }
        else if (building->m_type == Building::TypeBridge)
        {
            LegacyVector3 front;
            auto bridge = static_cast<Bridge*>(building);
            bridge->GetAvailablePosition(toPos, front);
        }
        else if (building->m_type == Building::TypeResearchItem)
        {
            auto item = static_cast<ResearchItem*>(building);
            LegacyVector3 end1, end2;
            item->GetEndPositions(end1, end2);
            float time = g_gameTime + _engineer.m_id.GetIndex();
            toPos = end1;
            toPos += (end2 - end1) / 2.0f;
            toPos += (end2 - end1) * sinf(time) * 0.5f;
        }

        LegacyVector3 midPoint = fromPos + (toPos - fromPos) / 2.0f;
        LegacyVector3 camToMidPoint = g_context->m_camera->GetPos() - midPoint;
        LegacyVector3 rightAngle = (camToMidPoint ^ (midPoint - toPos)).Normalise();

        rightAngle *= 0.5f;

        glColor4f(0.2f, 0.4f, 1.0f, fabs(sinf(g_gameTime * 3.0f)));

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(false);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/laser.bmp"));

        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3fv((fromPos - rightAngle).GetData());
        glTexCoord2i(0, 1);
        glVertex3fv((fromPos + rightAngle).GetData());
        glTexCoord2i(1, 1);
        glVertex3fv((toPos + rightAngle).GetData());
        glTexCoord2i(1, 0);
        glVertex3fv((toPos - rightAngle).GetData());
        glEnd();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_TEXTURE_2D);
        glDepthMask(true);
        glDisable(GL_BLEND);
    }
}

void EngineerRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const Engineer& engineer = static_cast<const Engineer&>(_entity);

    LegacyVector3 predictedPos = engineer.m_pos + engineer.m_vel * _ctx.predictionTime;
    if (engineer.m_onGround)
    {
        predictedPos.y = std::max(g_context->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z), 0.0f) +
            engineer.m_hoverHeight;
    }

    if (!engineer.m_dead)
    {
        RenderShape(engineer, _ctx.predictionTime);

        ShadowRenderer::Begin();
        ShadowRenderer::Render(predictedPos, 10.0f);
        ShadowRenderer::End();
    }

    if (engineer.m_state == Engineer::StateReprogramming ||
        engineer.m_state == Engineer::StateOperatingBridge ||
        engineer.m_state == Engineer::StateResearching)
    {
        RenderLaser(engineer, _ctx.predictionTime);
    }
}

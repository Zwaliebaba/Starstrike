#include "pch.h"

#include "SnowRenderer.h"
#include "3d_sprite.h"
#include "resource.h"
#include "snow.h"
#include "GameApp.h"
#include "globals.h"

void SnowRenderer::Render(const WorldObject& _object, const WorldObjectRenderContext& _ctx)
{
    const Snow& snow = static_cast<const Snow&>(_object);

    float predictionTime = _ctx.predictionTime - SERVER_ADVANCE_PERIOD;

    LegacyVector3 predictedPos = snow.m_pos + snow.m_vel * predictionTime;
    predictedPos += snow.m_hover * predictionTime;

    float size = 20.0f;

    glColor4f(1.0f, 1.0f, 1.0f, 1.0);
    Render3DSprite(predictedPos, size, size, Resource::GetTexture("textures/starburst.bmp"));
}

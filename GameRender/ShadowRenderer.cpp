#include "pch.h"

#include "ShadowRenderer.h"
#include "GameApp.h"
#include "location.h"
#include "renderer.h"
#include "camera.h"
#include "resource.h"

static float s_savedNearPlane;

void ShadowRenderer::Begin()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/glow.bmp"));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glDisable(GL_CULL_FACE);
    glDepthMask(false);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
    glColor4f(0.6f, 0.6f, 0.6f, 0.0f);

    s_savedNearPlane = g_app->m_renderer->GetNearPlane();
    g_app->m_camera->SetupProjectionMatrix(s_savedNearPlane * 1.05f,
                                           g_app->m_renderer->GetFarPlane());
}

void ShadowRenderer::End()
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);

    glDepthMask(true);
    glEnable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);

    g_app->m_camera->SetupProjectionMatrix(s_savedNearPlane,
                                           g_app->m_renderer->GetFarPlane());
}

void ShadowRenderer::Render(const LegacyVector3& _pos, float _size)
{
    LegacyVector3 shadowPos = _pos;
    auto shadowR = LegacyVector3(_size, 0, 0);
    auto shadowU = LegacyVector3(0, 0, _size);

    LegacyVector3 posA = shadowPos - shadowR - shadowU;
    LegacyVector3 posB = shadowPos + shadowR - shadowU;
    LegacyVector3 posC = shadowPos + shadowR + shadowU;
    LegacyVector3 posD = shadowPos - shadowR + shadowU;

    posA.y = g_app->m_location->m_landscape.m_heightMap->GetValue(posA.x, posA.z) + 0.9f;
    posB.y = g_app->m_location->m_landscape.m_heightMap->GetValue(posB.x, posB.z) + 0.9f;
    posC.y = g_app->m_location->m_landscape.m_heightMap->GetValue(posC.x, posC.z) + 0.9f;
    posD.y = g_app->m_location->m_landscape.m_heightMap->GetValue(posD.x, posD.z) + 0.9f;

    posA.y = max(posA.y, 1.0f);
    posB.y = max(posB.y, 1.0f);
    posC.y = max(posC.y, 1.0f);
    posD.y = max(posD.y, 1.0f);

    if (posA.y > _pos.y && posB.y > _pos.y && posC.y > _pos.y && posD.y > _pos.y)
    {
        // The object casting the shadow is beneath the ground
        return;
    }

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3fv(posA.GetData());
    glTexCoord2f(1.0f, 0.0f);
    glVertex3fv(posB.GetData());
    glTexCoord2f(1.0f, 1.0f);
    glVertex3fv(posC.GetData());
    glTexCoord2f(0.0f, 1.0f);
    glVertex3fv(posD.GetData());
    glEnd();
}

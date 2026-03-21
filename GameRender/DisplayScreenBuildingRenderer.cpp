#include "pch.h"

#include "DisplayScreenBuildingRenderer.h"
#include "constructionyard.h"
#include "main.h"
#include "camera.h"
#include "resource.h"
#include "renderer.h"
#include "ShapeStatic.h"
#include "GameApp.h"

void DisplayScreenBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    DefaultBuildingRenderer::RenderAlphas(_building, _ctx);

    auto& screen = static_cast<const DisplayScreen&>(_building);

    LegacyVector3 armourPos = _building.m_centerPos + LegacyVector3(0, 75, 0);
    LegacyVector3 armourFront(0, 0, 1);
    armourFront.RotateAroundY(g_gameTime * -0.75f);

    Matrix34 armourMat(armourFront, g_upVector, armourPos);
    armourMat.f *= 3.0f;
    armourMat.u *= 3.0f;
    armourMat.r *= 3.0f;

    LegacyVector3 targetPos = armourMat.pos + LegacyVector3(0, 50, 0);

    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDepthMask(false);

    //
    // Render black blob

    LegacyVector3 camRight = g_app->m_camera->GetRight();
    LegacyVector3 camUp = g_app->m_camera->GetUp();
    float size = 70.0f;
    glColor4f(0.4f, 0.3f, 0.4f, 0.0f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/glow.bmp"));

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);

    // NOTE: The original code had a commented-out glBegin(GL_QUADS) before
    // these vertices.  The vertices are orphaned — no quad is emitted.
    // Preserving as-is for behavioral parity.
    //glBegin( GL_QUADS );
    glTexCoord2i(0, 0);
    glVertex3fv((targetPos - camRight * size - camUp * size).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((targetPos + camRight * size - camUp * size).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((targetPos + camRight * size + camUp * size).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((targetPos - camRight * size + camUp * size).GetData());
    glEnd();

    glDisable(GL_TEXTURE_2D);

    //
    // Render rays

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glShadeModel(GL_SMOOTH);

    for (int i = 0; i < DISPLAYSCREEN_NUMRAYS; ++i)
    {
        Matrix34 buildingMat(_building.m_front, _building.m_up, _building.m_pos);
        Matrix34 rayMat = _building.m_shape->GetMarkerWorldMatrix(screen.m_rays[i], buildingMat);

        LegacyVector3 rayToArmour = (rayMat.pos - targetPos).Normalise();
        LegacyVector3 right = (g_app->m_camera->GetPos() - rayMat.pos) ^ rayToArmour;
        right.Normalise();

        glBegin(GL_QUADS);
        glColor4f(0.9f, 0.9f, 0.9f, 0.5f);
        glVertex3fv((rayMat.pos - right).GetData());
        glVertex3fv((rayMat.pos + right).GetData());

        glColor4f(0.9f, 0.9f, 0.9f, 0.0f);
        glVertex3fv((targetPos + right * 30).GetData());
        glVertex3fv((targetPos - right * 30).GetData());
        glEnd();
    }

    glShadeModel(GL_FLAT);

    //
    // Render armour

    glEnable(GL_NORMALIZE);

    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    screen.m_armour->Render(_ctx.predictionTime, armourMat);

    g_app->m_renderer->UnsetObjectLighting();

    glDisable(GL_NORMALIZE);
}

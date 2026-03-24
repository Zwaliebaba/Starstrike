#include "pch.h"

#include "BlueprintStoreBuildingRenderer.h"
#include "blueprintstore.h"
#include "GameApp.h"
#include "main.h"
#include "resource.h"

// ---------------------------------------------------------------------------
// BlueprintStoreBuildingRenderer::RenderAlphas
// ---------------------------------------------------------------------------

void BlueprintStoreBuildingRenderer::RenderAlphas(const Building& _building,
                                                   const BuildingRenderContext& _ctx)
{
    BlueprintBuildingRenderer::RenderAlphas(_building, _ctx);

    auto& store = static_cast<const BlueprintStore&>(_building);

    LegacyVector3 screenPos, screenRight, screenUp;
    float screenSize;
    const_cast<BlueprintStore&>(store).GetDisplay(screenPos, screenRight, screenUp, screenSize);

    //
    // Render main darwinian

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("sprites/darwinian.bmp"));
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glDepthMask(false);

    float texX = 0.0f;
    float texY = 0.0f;
    float texH = 1.0f;
    float texW = 1.0f;

    glShadeModel(GL_SMOOTH);

    glBegin(GL_QUADS);
    float infected = store.m_segments[0] / 100.0f;
    glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
    glTexCoord2f(texX, texY);
    glVertex3fv(const_cast<float*>(screenPos.GetData()));

    infected = store.m_segments[1] / 100.0f;
    glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
    glTexCoord2f(texX + texW, texY);
    glVertex3fv(const_cast<float*>((screenPos + screenRight * screenSize * 2.0f).GetData()));

    infected = store.m_segments[2] / 100.0f;
    glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
    glTexCoord2f(texX + texW, texY + texH);
    glVertex3fv(const_cast<float*>((screenPos + screenRight * screenSize * 2.0f + screenUp * screenSize * 2.0f).GetData()));

    infected = store.m_segments[3] / 100.0f;
    glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
    glTexCoord2f(texX, texY + texH);
    glVertex3fv(const_cast<float*>((screenPos + screenUp * screenSize * 2.0f).GetData()));
    glEnd();

    //
    // Render lines for over effect

    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/interface_grey.bmp"));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    texX = 0.0f;
    texW = 3.0f;
    texY = g_gameTime * 0.01f;
    texH = 0.3f;

    for (int i = 0; i < 2; ++i)
    {
        glBegin(GL_QUADS);
        infected = store.m_segments[0] / 100.0f;
        glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
        glTexCoord2f(texX, texY);
        glVertex3fv(const_cast<float*>(screenPos.GetData()));

        infected = store.m_segments[1] / 100.0f;
        glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
        glTexCoord2f(texX + texW, texY);
        glVertex3fv(const_cast<float*>((screenPos + screenRight * screenSize * 2.0f).GetData()));

        infected = store.m_segments[2] / 100.0f;
        glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
        glTexCoord2f(texX + texW, texY + texH);
        glVertex3fv(const_cast<float*>((screenPos + screenRight * screenSize * 2.0f + screenUp * screenSize * 2.0f).GetData()));

        infected = store.m_segments[3] / 100.0f;
        glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
        glTexCoord2f(texX, texY + texH);
        glVertex3fv(const_cast<float*>((screenPos + screenUp * screenSize * 2.0f).GetData()));
        glEnd();

        texY *= 1.5f;
        texH = 0.1f;
    }

    glShadeModel(GL_FLAT);

    glDepthMask(true);
    glDisable(GL_TEXTURE_2D);
}

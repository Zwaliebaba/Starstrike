#include "pch.h"

#include "SpiritReceiverBuildingRenderer.h"
#include "spiritreceiver.h"
#include "camera.h"
#include "GameApp.h"
#include "location.h"
#include "resource.h"
#include "ShapeStatic.h"

// ---------------------------------------------------------------------------
// SpiritReceiverBuildingRenderer::Render
// ---------------------------------------------------------------------------

void SpiritReceiverBuildingRenderer::Render(const Building& _building,
                                            const BuildingRenderContext& _ctx)
{
    auto& receiver = static_cast<const SpiritReceiver&>(_building);

    // Editor: update orientation from landscape normal (mutates sim state —
    // editor-only, matches legacy behaviour).
    if (g_context->m_editing)
    {
        auto& mut = const_cast<SpiritReceiver&>(receiver);
        mut.m_up = g_context->m_location->m_landscape.m_normalMap->GetValue(
            receiver.m_pos.x, receiver.m_pos.z);
        LegacyVector3 right(1, 0, 0);
        mut.m_front = right ^ mut.m_up;
    }

    // Base shape (same as ReceiverBuilding::Render → Building::Render)
    ReceiverBuildingRenderer::Render(_building, _ctx);

    // Head shape at the head marker
    Matrix34 mat(receiver.m_front, receiver.m_up, receiver.m_pos);
    LegacyVector3 headPos = receiver.m_shape->GetMarkerWorldMatrix(receiver.m_headMarker, mat).pos;
    LegacyVector3 up = g_upVector;
    LegacyVector3 right(1, 0, 0);
    LegacyVector3 front = up ^ right;
    Matrix34 headMat(front, up, headPos);
    receiver.m_headShape->Render(_ctx.predictionTime, headMat);
}

// ---------------------------------------------------------------------------
// SpiritReceiverBuildingRenderer::RenderPorts
// ---------------------------------------------------------------------------

void SpiritReceiverBuildingRenderer::RenderPorts(const Building& _building)
{
    auto& receiver = static_cast<const SpiritReceiver&>(_building);

    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));
    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    int numPorts = const_cast<Building&>(_building).GetNumPorts();
    for (int i = 0; i < numPorts; ++i)
    {
        Matrix34 rootMat(receiver.m_front, receiver.m_up, receiver.m_pos);
        Matrix34 worldMat = receiver.m_shape->GetMarkerWorldMatrix(receiver.m_statusMarkers[i], rootMat);

        float size = 6.0f;
        LegacyVector3 camR = g_context->m_camera->GetRight() * size;
        LegacyVector3 camU = g_context->m_camera->GetUp() * size;

        LegacyVector3 statusPos = worldMat.pos;

        if (const_cast<Building&>(_building).GetPortOccupant(i).IsValid())
            glColor4f(0.3f, 1.0f, 0.3f, 1.0f);
        else
            glColor4f(1.0f, 0.3f, 0.3f, 1.0f);

        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3fv(const_cast<float*>((statusPos - camR - camU).GetData()));
        glTexCoord2i(1, 0);
        glVertex3fv(const_cast<float*>((statusPos + camR - camU).GetData()));
        glTexCoord2i(1, 1);
        glVertex3fv(const_cast<float*>((statusPos + camR + camU).GetData()));
        glTexCoord2i(0, 1);
        glVertex3fv(const_cast<float*>((statusPos - camR + camU).GetData()));
        glEnd();
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glDepthMask(true);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
}

// ---------------------------------------------------------------------------
// SpiritReceiverBuildingRenderer::RenderAlphas
// ---------------------------------------------------------------------------

void SpiritReceiverBuildingRenderer::RenderAlphas(const Building& _building,
                                                  const BuildingRenderContext& _ctx)
{
    ReceiverBuildingRenderer::RenderAlphas(_building, _ctx);

    // Original code computed fractionOccupied but never used it in RenderAlphas.
    // Keeping the delegate-only path.
}

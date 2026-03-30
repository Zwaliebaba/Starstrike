#include "pch.h"

#include "TriffidBuildingRenderer.h"
#include "triffid.h"
#include "matrix34.h"
#include "ShapeStatic.h"
#include "resource.h"
#include "hi_res_time.h"
#include "GameApp.h"
#include "main.h"
#include "text_renderer.h"

#ifdef LOCATION_EDITOR
#include "location_editor.h"
#include "location.h"
#include "landscape.h"
#endif

void TriffidBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    const Triffid& triffid = static_cast<const Triffid&>(_building);

    Matrix34 mat = const_cast<Triffid&>(triffid).GetHead();

    // Stem lines from head down to base
    LegacyVector3 stemPos = triffid.m_shape->GetMarkerWorldMatrix(triffid.m_stem, mat).pos;
    LegacyVector3 midPoint = mat.pos + (stemPos - mat.pos).SetLength(10.0f);
    glColor4f(1.0f, 1.0f, 0.5f, 1.0f);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
    glVertex3fv(mat.pos.GetData());
    glVertex3fv(midPoint.GetData());
    glVertex3fv(midPoint.GetData());
    glVertex3fv(const_cast<LegacyVector3&>(triffid.m_pos).GetData());
    glEnd();

    // Damage flicker effect
    if (triffid.m_renderDamaged && !g_context->m_editing && triffid.m_damage > 0.0f)
    {
        float timeIndex = g_gameTime + triffid.m_id.GetUniqueId() * 10;
        float thefrand = frand();
        if (thefrand > 0.7f)
            mat.f *= (1.0f - sinf(timeIndex) * 0.5f);
        else if (thefrand > 0.4f)
            mat.u *= (1.0f - sinf(timeIndex) * 0.2f);
        else
            mat.r *= (1.0f - sinf(timeIndex) * 0.5f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
    }

    glEnable(GL_NORMALIZE);

    triffid.m_shape->Render(_ctx.predictionTime, mat);

    // Egg preview when about to fire
    if (triffid.m_triggered && GetHighResTime() > triffid.m_timerSync - triffid.m_reloadTime * 0.25f)
    {
        Matrix34 launchMat = triffid.m_shape->GetMarkerWorldMatrix(triffid.m_launchPoint, mat);
        ShapeStatic* eggShape = Resource::GetShapeStatic("triffidegg.shp");
        Matrix34 eggMat(launchMat.u, -launchMat.f, launchMat.pos);
        eggMat.f *= triffid.m_size;
        eggMat.u *= triffid.m_size;
        eggMat.r *= triffid.m_size;
        eggShape->Render(_ctx.predictionTime, eggMat);
    }

    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_NORMALIZE);
}

void TriffidBuildingRenderer::RenderAlphas(const Building& _building, [[maybe_unused]] const BuildingRenderContext& _ctx)
{
    if (!g_context->m_editing)
        return;

    const Triffid& triffid = static_cast<const Triffid&>(_building);

    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);

    Matrix34 headMat = const_cast<Triffid&>(triffid).GetHead();
    Matrix34 mat(triffid.m_front, g_upVector, headMat.pos);
    Matrix34 launchMat = triffid.m_shape->GetMarkerWorldMatrix(triffid.m_launchPoint, mat);

    LegacyVector3 point1 = launchMat.pos;

    LegacyVector3 angle = launchMat.f;
    angle.HorizontalAndNormalise();
    angle.RotateAroundY(triffid.m_variance * 0.5f);
    LegacyVector3 right = angle ^ g_upVector;
    angle.RotateAround(right * triffid.m_pitch * 0.5f);
    LegacyVector3 point2 = point1 + angle * triffid.m_force * 3.0f;

    angle = launchMat.f;
    angle.HorizontalAndNormalise();
    angle.RotateAroundY(triffid.m_variance * -0.5f);
    right = angle ^ g_upVector;
    angle.RotateAround(right * triffid.m_pitch * 0.5f);
    LegacyVector3 point3 = point1 + angle * triffid.m_force * 3.0f;

    glBegin(GL_LINE_LOOP);
    glVertex3fv(point1.GetData());
    glVertex3fv(point2.GetData());
    glVertex3fv(point3.GetData());
    glEnd();

#ifdef LOCATION_EDITOR
    if (g_context->m_locationEditor->m_mode == LocationEditor::ModeBuilding && g_context->m_locationEditor->m_selectionId == triffid.m_id.GetUniqueId())
    {
        LegacyVector3 velocity = headMat.f;
        velocity.SetLength(triffid.m_force * triffid.m_size);
        TriffidEgg egg;
        egg.m_pos = headMat.pos;
        egg.m_vel = velocity;
        egg.m_front = headMat.f;

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        while (true)
        {
            glVertex3fv(egg.m_pos.GetData());
            egg.Advance(NULL);
            glVertex3fv(egg.m_pos.GetData());

            if (egg.m_vel.Mag() < 20.0f)
                break;
        }
        glEnd();

        if (triffid.m_useTrigger)
        {
            LegacyVector3 triggerPos = triffid.m_pos + triffid.m_triggerLocation;
            int numSteps = 20;
            glBegin(GL_LINE_LOOP);

            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            for (int i = 0; i < numSteps; ++i)
            {
                float a = 2.0f * M_PI * static_cast<float>(i) / static_cast<float>(numSteps);
                LegacyVector3 thisPos = triggerPos + LegacyVector3(sinf(a) * triffid.m_triggerRadius, 0.0f, cosf(a) * triffid.m_triggerRadius);
                thisPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(thisPos.x, thisPos.z);
                thisPos.y += 10.0f;
                glVertex3fv(thisPos.GetData());
            }
            glEnd();

            g_editorFont.DrawText3DCenter(triggerPos + LegacyVector3(0, 50, 0), 10.0f, "UseTrigger: %d", triffid.m_useTrigger);
        }
    }
#endif
}

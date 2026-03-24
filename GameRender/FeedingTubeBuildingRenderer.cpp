#include "pch.h"
#include "FeedingTubeBuildingRenderer.h"
#include "feedingtube.h"
#include "resource.h"
#include "ogl_extensions.h"
#include "profiler.h"
#include "camera.h"
#include "GameApp.h"
#include "location.h"
#include "main.h"

void FeedingTubeBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    const FeedingTube& tube = static_cast<const FeedingTube&>(_building);

    if (g_context->m_editing)
        return;

    if (tube.m_receiverId != -1)
    {
        RenderSignal(tube, _ctx.predictionTime, 10.0f, 0.4f);
        RenderSignal(tube, _ctx.predictionTime, 9.0f, 0.2f);
        RenderSignal(tube, _ctx.predictionTime, 8.0f, 0.2f);
        RenderSignal(tube, _ctx.predictionTime, 4.0f, 0.5f);
    }
}

void FeedingTubeBuildingRenderer::RenderSignal(const FeedingTube& _tube, float _predictionTime, float _radius, float _alpha)
{
    START_PROFILE(g_context->m_profiler, "Signal");

    auto receiver = static_cast<FeedingTube*>(g_context->m_location->GetBuilding(_tube.m_receiverId));
    if (!receiver)
    {
        END_PROFILE(g_context->m_profiler, "Signal");
        return;
    }

    LegacyVector3 startPos = const_cast<FeedingTube&>(_tube).GetStartPoint();
    LegacyVector3 endPos = const_cast<FeedingTube&>(_tube).GetEndPoint();
    LegacyVector3 delta = endPos - startPos;
    LegacyVector3 deltaNorm = delta;
    deltaNorm.Normalise();

    float distance = (startPos - endPos).Mag();
    float numRadii = 20.0f;
    int numSteps = static_cast<int>(distance / 200.0f);
    numSteps = std::max(1, numSteps);

    glEnable(GL_TEXTURE_2D);

    gglActiveTextureARB(GL_TEXTURE0_ARB);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/laserfence.bmp", true, true));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE);
    glEnable(GL_TEXTURE_2D);

    gglActiveTextureARB(GL_TEXTURE1_ARB);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/radarsignal.bmp", true, true));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);

    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);
    glDepthMask(false);
    glColor4f(1.0f, 1.0f, 1.0f, _alpha);

    auto& mv = OpenGLD3D::GetModelViewStack();
    mv.Translate(startPos.x, startPos.y, startPos.z);
    LegacyVector3 dishFront = const_cast<FeedingTube&>(_tube).GetForwardsClippingDir(_predictionTime, receiver);
    double eqn1[4] = {dishFront.x, dishFront.y, dishFront.z, -1.0f};
    glClipPlane(GL_CLIP_PLANE0, eqn1);

    LegacyVector3 receiverPos = receiver->GetDishPos(_predictionTime);
    LegacyVector3 receiverFront = receiver->GetForwardsClippingDir(_predictionTime, const_cast<FeedingTube*>(&_tube));
    mv.Translate(-startPos.x, -startPos.y, -startPos.z);
    mv.Translate(receiverPos.x, receiverPos.y, receiverPos.z);

    float thisDistance = -1.0f;

    double eqn2[4] = {receiverFront.x, receiverFront.y, receiverFront.z, thisDistance};
    glClipPlane(GL_CLIP_PLANE1, eqn2);
    mv.Translate(-receiverPos.x, -receiverPos.y, -receiverPos.z);

    mv.Translate(startPos.x, startPos.y, startPos.z);

    glEnable(GL_CLIP_PLANE0);
    glEnable(GL_CLIP_PLANE1);

    float texXInner = -g_gameTime / _radius;
    float texXOuter = -g_gameTime;

    glBegin(GL_QUAD_STRIP);

    for (int s = 0; s < numSteps; ++s)
    {
        LegacyVector3 deltaFrom = 1.2f * delta * static_cast<float>(s) / static_cast<float>(numSteps);
        LegacyVector3 deltaTo = 1.2f * delta * static_cast<float>(s + 1) / static_cast<float>(numSteps);

        LegacyVector3 currentPos = (-delta * 0.1f) + LegacyVector3(0, _radius, 0);

        for (int r = 0; r <= static_cast<int>(numRadii); ++r)
        {
            gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, texXInner, r / numRadii);
            gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, texXOuter, r / numRadii);
            glVertex3fv((currentPos + deltaFrom).GetData());

            gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, texXInner + 10.0f / static_cast<float>(numSteps), r / numRadii);
            gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, texXOuter + distance / (200.0f * static_cast<float>(numSteps)), r / numRadii);
            glVertex3fv((currentPos + deltaTo).GetData());

            currentPos.RotateAround(deltaNorm * (2.0f * M_PI / numRadii));
        }

        texXInner += 10.0f / static_cast<float>(numSteps);
        texXOuter += distance / (200.0f * static_cast<float>(numSteps));
    }

    glEnd();
    mv.Translate(-startPos.x, -startPos.y, -startPos.z);

    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDepthMask(true);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);

    gglActiveTextureARB(GL_TEXTURE1_ARB);
    glDisable(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    gglActiveTextureARB(GL_TEXTURE0_ARB);
    glDisable(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    END_PROFILE(g_context->m_profiler, "Signal");
}

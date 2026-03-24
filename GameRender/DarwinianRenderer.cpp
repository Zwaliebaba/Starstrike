#include "pch.h"

#include "DarwinianRenderer.h"
#include "darwinian.h"
#include "entity.h"
#include "goddish.h"
#include "camera.h"
#include "location.h"
#include "main.h"
#include "renderer.h"
#include "resource.h"
#include "GameApp.h"
#include "team.h"

void DarwinianRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const Darwinian& d = static_cast<const Darwinian&>(_entity);

    float _predictionTime = _ctx.predictionTime;
    float _highDetail = _ctx.highDetailFactor;

    if (!d.m_enabled)
        return;

    if (d.m_state == Darwinian::StateInsideArmour)
        return;

    RGBAColour colour;
    if (d.m_id.GetTeamId() >= 0)
        colour = g_context->m_location->m_teams[d.m_id.GetTeamId()].m_colour;

    LegacyVector3 predictedPos = d.m_pos + d.m_vel * _predictionTime;
    LegacyVector3 entityUp = g_upVector;

    if (_highDetail > 0.0f)
    {
        if (d.m_onGround && d.m_inWater == -1)
            predictedPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z);
        else if (!d.m_onGround)
            predictedPos.y += 3.0f;

        entityUp = g_context->m_location->m_landscape.m_normalMap->GetValue(predictedPos.x, predictedPos.z);

        if (d.m_state == Darwinian::StateWorshipSpirit || d.m_state == Darwinian::StateOperatingPort || d.m_state == Darwinian::StateWatchingSpectacle)
            entityUp.RotateAround(d.m_front * sinf(g_gameTime) * 0.3f);
    }

    if (!d.m_onGround)
        entityUp.RotateAround(d.m_front * g_gameTime * (d.m_id.GetUniqueId() % 10) * 0.1f);

    LegacyVector3 entityRight(d.m_front ^ entityUp);

    if (d.m_state == Darwinian::StateCapturedByAnt)
    {
        LegacyVector3 capturedRight = entityUp;
        entityUp = d.m_vel * -1;
        entityUp.Normalise();
        entityRight = capturedRight;
        predictedPos += LegacyVector3(0, 4, 0);
    }

    float size = 3.0f;
    size *= (1.0f + 0.03f * ((d.m_id.GetIndex() * d.m_id.GetUniqueId()) % 10));
    entityRight *= size;
    entityUp *= size * 2.0f;

    //
    // Draw our shadow on the landscape

    if (_highDetail > 0.0f && d.m_shadowBuildingId == -1 && !d.m_dead)
    {
        int alpha = 150 * _highDetail;
        alpha = std::min(alpha, 255);
        glColor4ub(0, 0, 0, alpha);

        LegacyVector3 pos1 = predictedPos - entityRight;
        LegacyVector3 pos2 = predictedPos + entityRight;
        LegacyVector3 pos4 = pos1 + LegacyVector3(0.0f, 0.0f, size * 2.0f);
        LegacyVector3 pos3 = pos2 + LegacyVector3(0.0f, 0.0f, size * 2.0f);

        pos1.y = 0.3f + g_context->m_location->m_landscape.m_heightMap->GetValue(pos1.x, pos1.z);
        pos2.y = 0.3f + g_context->m_location->m_landscape.m_heightMap->GetValue(pos2.x, pos2.z);
        pos3.y = 0.3f + g_context->m_location->m_landscape.m_heightMap->GetValue(pos3.x, pos3.z);
        pos4.y = 0.3f + g_context->m_location->m_landscape.m_heightMap->GetValue(pos4.x, pos4.z);

        glDepthMask(false);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3fv(pos1.GetData());
        glTexCoord2f(1.0f, 0.0f);
        glVertex3fv(pos2.GetData());
        glTexCoord2f(1.0f, 1.0f);
        glVertex3fv(pos3.GetData());
        glTexCoord2f(0.0f, 1.0f);
        glVertex3fv(pos4.GetData());
        glEnd();
        glDepthMask(true);
    }

    if (!d.m_dead || !d.m_onGround)
    {
        //
        // Draw our texture

        float maxHealth = EntityBlueprint::GetStat(d.m_type, Entity::StatHealth);
        float health = static_cast<float>(d.m_stats[Entity::StatHealth]) / maxHealth;
        if (health > 1.0f)
            health = 1.0f;
        colour *= 0.3f + 0.7f * health;
        colour.a = 255;

        if (d.m_dead)
        {
            glEnable(GL_BLEND);
            colour.a = 2.5f * static_cast<float>(d.m_stats[Entity::StatHealth]);
            colour.r *= 0.2f;
            colour.g *= 0.2f;
            colour.b *= 0.2f;
        }

        if (d.m_state == Darwinian::StateOnFire)
        {
            colour.r *= 0.01f;
            colour.g *= 0.01f;
            colour.b *= 0.01f;
        }

        glColor4ubv(colour.GetData());
        glBegin(GL_QUADS);
        glTexCoord2i(0, 1);
        glVertex3fv((predictedPos - entityRight + entityUp).GetData());
        glTexCoord2i(1, 1);
        glVertex3fv((predictedPos + entityRight + entityUp).GetData());
        glTexCoord2i(1, 0);
        glVertex3fv((predictedPos + entityRight).GetData());
        glTexCoord2i(0, 0);
        glVertex3fv((predictedPos - entityRight).GetData());
        glEnd();

        //
        // Draw a blue glow if we are under control

        if (d.m_state == Darwinian::StateUnderControl)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glEnable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);
            float scale = 1.1f;
            entityRight *= scale;
            entityUp *= scale;
            float alpha = fabs(sinf(g_gameTime * 2.0f));
            glColor4f(0.0f, 0.0f, 1.0f, alpha);
            glBegin(GL_QUADS);
            glTexCoord2i(0, 1);
            glVertex3fv((predictedPos - entityRight + entityUp).GetData());
            glTexCoord2i(1, 1);
            glVertex3fv((predictedPos + entityRight + entityUp).GetData());
            glTexCoord2i(1, 0);
            glVertex3fv((predictedPos + entityRight).GetData());
            glTexCoord2i(0, 0);
            glVertex3fv((predictedPos - entityRight).GetData());
            glEnd();
            glEnable(GL_DEPTH_TEST);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        //
        // Draw shadow if we are watching a God Dish

        if (_highDetail > 0.0f && d.m_shadowBuildingId != -1)
        {
            Building* building = g_context->m_location->GetBuilding(d.m_shadowBuildingId);
            if (building)
            {
                float alpha = 1.0f;
                if (building->m_type == Building::TypeGodDish)
                {
                    auto dish = static_cast<GodDish*>(building);
                    alpha = dish->m_timer * 0.1f;
                }
                else if (building->m_type == Building::TypeEscapeRocket)
                {
                    float distance = (building->m_pos - d.m_pos).Mag();
                    if (distance < 400.0f)
                        alpha = 1.0f;
                    else if (distance < 700.0f)
                        alpha = (700 - distance) / 300.0f;
                    else
                        alpha = 0.0f;
                }
                alpha = std::min(alpha, 1.0f);
                alpha = std::max(alpha, 0.0f);

                if (alpha > 0.0f)
                {
                    LegacyVector3 length = (predictedPos - building->m_pos).SetLength(size * 10);

                    // Shadow behind the Darwinian (green dudes only)
                    if (d.m_id.GetTeamId() == 0)
                    {
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        glEnable(GL_BLEND);
                        glColor4f(0.0f, 0.0f, 0.0f, alpha);
                        LegacyVector3 shadowVector = length;
                        shadowVector.SetLength(0.05f);
                        predictedPos += shadowVector;
                        glBegin(GL_QUADS);
                        glTexCoord2i(0, 1);
                        glVertex3fv((predictedPos - entityRight + entityUp).GetData());
                        glTexCoord2i(1, 1);
                        glVertex3fv((predictedPos + entityRight + entityUp).GetData());
                        glTexCoord2i(1, 0);
                        glVertex3fv((predictedPos + entityRight).GetData());
                        glTexCoord2i(0, 0);
                        glVertex3fv((predictedPos - entityRight).GetData());
                        glEnd();
                    }

                    // Shadow on the ground

                    LegacyVector3 pos1 = predictedPos - entityRight;
                    LegacyVector3 pos2 = predictedPos + entityRight;
                    LegacyVector3 pos4 = pos1 + length;
                    LegacyVector3 pos3 = pos2 + length;
                    pos4 -= entityRight;
                    pos3 += entityRight;

                    pos1.y = 0.3f + g_context->m_location->m_landscape.m_heightMap->GetValue(pos1.x, pos1.z);
                    pos2.y = 0.3f + g_context->m_location->m_landscape.m_heightMap->GetValue(pos2.x, pos2.z);
                    pos3.y = 0.3f + g_context->m_location->m_landscape.m_heightMap->GetValue(pos3.x, pos3.z);
                    pos4.y = 0.3f + g_context->m_location->m_landscape.m_heightMap->GetValue(pos4.x, pos4.z);

                    glShadeModel(GL_SMOOTH);
                    glDepthMask(false);
                    glBegin(GL_QUADS);
                    glColor4f(0.0f, 0.0f, 0.0f, alpha);
                    glTexCoord2f(0.0f, 0.0f);
                    glVertex3fv(pos1.GetData());
                    glTexCoord2f(1.0f, 0.0f);
                    glVertex3fv(pos2.GetData());
                    glColor4f(0.0f, 0.0f, 0.0f, 0.1f * alpha);
                    glTexCoord2f(1.0f, 1.0f);
                    glVertex3fv(pos3.GetData());
                    glTexCoord2f(0.0f, 1.0f);
                    glVertex3fv(pos4.GetData());
                    glEnd();
                    glShadeModel(GL_FLAT);
                    glDepthMask(true);
                }
            }
        }
    }

    //
    // Render a santa hat if it's Christmas

    if (d.m_id.GetTeamId() == 0 && Location::ChristmasModEnabled() == 1)
    {
        if (d.m_id.GetUniqueId() % 3 == 0)
        {
            int santaHatId = Resource::GetTexture("sprites/santahat.bmp");
            glBindTexture(GL_TEXTURE_2D, santaHatId);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            predictedPos += entityUp * 0.95f;
            predictedPos += d.m_front * 0.01f;
            entityRight *= 0.65f;
            entityUp *= 0.65f;
            glBegin(GL_QUADS);
            glTexCoord2i(0, 1);
            glVertex3fv((predictedPos - entityRight + entityUp).GetData());
            glTexCoord2i(1, 1);
            glVertex3fv((predictedPos + entityRight + entityUp).GetData());
            glTexCoord2i(1, 0);
            glVertex3fv((predictedPos + entityRight).GetData());
            glTexCoord2i(0, 0);
            glVertex3fv((predictedPos - entityRight).GetData());
            glEnd();
            glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("sprites/darwinian.bmp"));
        }
    }

    //
    // Render our box kite if we have one

    if (d.m_boxKiteId.IsValid())
    {
        // TODO Phase 6: eliminate mutation — box kite update should move to Advance()
        auto boxKite = static_cast<BoxKite*>(g_context->m_location->GetEffect(d.m_boxKiteId));
        if (boxKite)
        {
            boxKite->m_up = entityUp;
            boxKite->m_up.Normalise();
            boxKite->m_front = d.m_front;
            boxKite->m_front.Normalise();
            boxKite->m_pos = predictedPos + d.m_front * 2 + boxKite->m_up * 3;
            boxKite->m_vel = d.m_vel;
        }
    }

    //
    // If we are dead render us in pieces

    if (d.m_dead)
    {
        LegacyVector3 entityFront = d.m_front;
        entityFront.Normalise();
        entityUp = g_upVector;
        entityRight = entityFront ^ entityUp;
        entityUp *= size * 2.0f;
        entityRight.Normalise();
        entityRight *= size;
        unsigned char alpha = static_cast<float>(d.m_stats[Entity::StatHealth]) * 2.55f;

        glColor4ub(0, 0, 0, alpha);

        entityRight *= 0.5f;
        entityUp *= 0.5;
        float predictedHealth = d.m_stats[Entity::StatHealth];
        if (d.m_onGround)
            predictedHealth -= 40 * _predictionTime;
        else
            predictedHealth -= 20 * _predictionTime;
        float landHeight = g_context->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z);

        for (int i = 0; i < 3; ++i)
        {
            LegacyVector3 fragmentPos = predictedPos;
            if (i == 0)
                fragmentPos.x += 10.0f - predictedHealth / 10.0f;
            if (i == 1)
                fragmentPos.x += 10.0f - predictedHealth / 10.0f;
            if (i == 1)
                fragmentPos.z += 10.0f - predictedHealth / 10.0f;
            if (i == 2)
                fragmentPos.x -= 10.0f - predictedHealth / 10.0f;
            fragmentPos.y += (fragmentPos.y - landHeight) * i * 0.5f;

            float left = 0.0f;
            float right = 1.0f;
            float top = 1.0f;
            float bottom = 0.0f;

            if (i == 0)
            {
                right -= (right - left) / 2;
                bottom -= (bottom - top) / 2;
            }
            if (i == 1)
            {
                left += (right - left) / 2;
                bottom -= (bottom - top) / 2;
            }
            if (i == 2)
            {
                top += (bottom - top) / 2;
                left += (right - left) / 2;
            }

            glBegin(GL_QUADS);
            glTexCoord2f(left, bottom);
            glVertex3fv((fragmentPos - entityRight + entityUp).GetData());
            glTexCoord2f(right, bottom);
            glVertex3fv((fragmentPos + entityRight + entityUp).GetData());
            glTexCoord2f(right, top);
            glVertex3fv((fragmentPos + entityRight).GetData());
            glTexCoord2f(left, top);
            glVertex3fv((fragmentPos - entityRight).GetData());
            glEnd();
        }
    }
}

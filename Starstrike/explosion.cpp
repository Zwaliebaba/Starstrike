#include "pch.h"
#include "explosion.h"
#include "GameApp.h"
#include "globals.h"
#include "main.h"
#include "math_utils.h"
#include "profiler.h"
#include "renderer.h"
#include "resource.h"

#define EXPLOSION_LIFETIME		5.0f
#define MAX_INITIAL_SPEED		3.0f
#define MAX_ANG_VEL				4.0f
#define ACCEL_DUE_TO_GRAV		(-GRAVITY)
#define INITIAL_VERTICAL_SPEED	10.0f
#define FRICTION_COEF			0.05f			// Bigger means more friction
#define ROT_FRICTION_COEF		0.2f			// Bigger means more rotational friction
#define NUM_TUMBLERS			5
#define MIN_CIRCUM				6.0f			// Triangles with perimeter below this are discarded

Tumbler::Tumbler()
{
  m_rotMat.SetToIdentity();
  m_angVel.x = sfrand(MAX_ANG_VEL);
  m_angVel.y = sfrand(MAX_ANG_VEL);
  m_angVel.z = sfrand(MAX_ANG_VEL);
}

void Tumbler::Advance()
{
  Matrix33 rotIncrement(m_angVel.x * g_advanceTime, m_angVel.y * g_advanceTime, m_angVel.z * g_advanceTime);
  m_rotMat *= rotIncrement;

  // Rotational Friction
  m_angVel *= 1.0f - (g_advanceTime * ROT_FRICTION_COEF);
}

Explosion::Explosion(const ShapeFragmentData* _frag, const FragmentState* _states, const Matrix34& _transform, float _fraction)
  : m_tumblers(NUM_TUMBLERS),
    m_timeToDie(g_gameTime + EXPLOSION_LIFETIME)
{
  std::vector<ExplodingTri> triangles;
  triangles.reserve(_frag->m_numTriangles);

  Matrix34 localTransform = _states
    ? Matrix34(_states[_frag->m_fragmentIndex].transform)
    : Matrix34(_frag->m_baseTransform);
  Matrix34 totalTransform(localTransform * _transform);
  LegacyVector3 transformedFragCenter = _frag->m_center * totalTransform;

  for (int j = 0; j < _frag->m_numTriangles; ++j)
  {
    if (_fraction < 1.0f && frand(1.0f) > _fraction)
      continue;

    const ShapeTriangle& shapeTri = _frag->m_triangles[j];
    const VertexPosCol& v1 = _frag->m_vertices[shapeTri.v1];
    const VertexPosCol& v2 = _frag->m_vertices[shapeTri.v2];
    const VertexPosCol& v3 = _frag->m_vertices[shapeTri.v3];

    ExplodingTri tri;
    tri.m_colour = _frag->m_colours[v1.m_colId];
    tri.m_v1 = _frag->m_positions[v1.m_posId] * totalTransform;
    tri.m_v2 = _frag->m_positions[v2.m_posId] * totalTransform;
    tri.m_v3 = _frag->m_positions[v3.m_posId] * totalTransform;

    if (tri.m_v1 == tri.m_v2 || tri.m_v2 == tri.m_v3 || tri.m_v1 == tri.m_v3)
      continue;

    float circum = (tri.m_v1 - tri.m_v2).Mag() + (tri.m_v2 - tri.m_v3).Mag() + (tri.m_v3 - tri.m_v1).Mag();

    if (circum < 6.0f)
    {
      // Too small to care about
      continue;
    }

    LegacyVector3 center = (tri.m_v1 + tri.m_v2 + tri.m_v3) * 0.3333f;

    tri.m_v1 -= center;
    tri.m_v2 -= center;
    tri.m_v3 -= center;

    LegacyVector3 a = tri.m_v1 - tri.m_v2;
    LegacyVector3 b = tri.m_v2 - tri.m_v3;
    tri.m_norm = (a ^ b).Normalise();
    if (j & 1)
      tri.m_norm = -tri.m_norm;

    tri.m_vel = (center - transformedFragCenter) * (MAX_INITIAL_SPEED);
    tri.m_vel.y += INITIAL_VERTICAL_SPEED;
    tri.m_pos = center;
    tri.m_tumbler = &m_tumblers[darwiniaRandom() % NUM_TUMBLERS];
    tri.m_timeToDie = g_gameTime + EXPLOSION_LIFETIME;

    triangles.push_back(tri);
  }

  m_tris = std::move(triangles);
}

bool Explosion::Advance()
{
  float deltaVelY = ACCEL_DUE_TO_GRAV * g_advanceTime;

  // Advance all tumblers
  for (auto& tumbler : m_tumblers)
    tumbler.Advance();

  // Advance all ExplodingTriangles
  for (auto& tri : m_tris)
  {
    if (g_gameTime > tri.m_timeToDie)
      continue;

    tri.m_pos += tri.m_vel * g_advanceTime;

    // Friction
    float speed = tri.m_vel.Mag();
    float friction = speed * FRICTION_COEF * g_advanceTime;
    if (friction > 1.0f)
      friction = 1.0f;
    tri.m_vel *= 1.0f - friction;

    // Gravity
    tri.m_vel.y += deltaVelY;
  }

  if (g_gameTime > m_timeToDie)
    return true;

  return false;
}

void Explosion::Render() const
{
  // someone tries to render explosion with 0 tris
  // happens at "kill all enemies"
  if (m_tris.empty())
    return;

  float age = (EXPLOSION_LIFETIME + (g_gameTime - m_timeToDie)) / EXPLOSION_LIFETIME;

  glBegin(GL_TRIANGLES);
  for (const auto& tri : m_tris)
  {
    if (g_gameTime > tri.m_timeToDie)
      continue;

    const LegacyVector3 norm(tri.m_tumbler->m_rotMat * tri.m_norm);
    const LegacyVector3 v1(tri.m_tumbler->m_rotMat * tri.m_v1 + tri.m_pos);
    const LegacyVector3 v2(tri.m_tumbler->m_rotMat * tri.m_v2 + tri.m_pos);
    const LegacyVector3 v3(tri.m_tumbler->m_rotMat * tri.m_v3 + tri.m_pos);

    glColor4ub(tri.m_colour.r, tri.m_colour.g, tri.m_colour.b, (1.0f - age) * 255);

    glNormal3fv(norm.GetDataConst());

    glTexCoord2i(0, 0);
    glVertex3fv(v1.GetDataConst());
    glTexCoord2i(0, 1);
    glVertex3fv(v2.GetDataConst());
    glTexCoord2i(1, 1);
    glVertex3fv(v3.GetDataConst());
  }
  glEnd();
}

LegacyVector3 Explosion::GetCenter() const
{
  auto sum = LegacyVector3(0, 0, 0);
  unsigned summed = 0;
  for (const auto& tri : m_tris)
  {
    if (g_gameTime > tri.m_timeToDie)
      continue;
    sum += tri.m_pos;
    summed++;
  }
  if (summed)
    return sum / summed;
  DEBUG_ASSERT(0);
  return sum;
}

ExplosionManager::ExplosionManager() {}

ExplosionManager::~ExplosionManager()
{
  for (auto* e : m_explosions)
    delete e;
}

void ExplosionManager::AddExplosion(const ShapeFragmentData* _frag, const FragmentState* _states, const Matrix34& _transform, bool _recurse, float _fraction)
{
  if (_fraction <= 0.0f)
    return;

  if (_frag->m_numTriangles > 0)
  {
    auto explosion = new Explosion(_frag, _states, _transform, _fraction);
    m_explosions.push_back(explosion);
  }

  if (_recurse)
  {
    Matrix34 localTransform = _states
      ? Matrix34(_states[_frag->m_fragmentIndex].transform)
      : Matrix34(_frag->m_baseTransform);
    Matrix34 totalMatrix(localTransform * _transform);

    for (unsigned int i = 0; i < _frag->m_childFragments.Size(); ++i)
    {
      ShapeFragmentData* child = _frag->m_childFragments.GetData(i);
      AddExplosion(child, _states, totalMatrix, true, _fraction);
    }
  }
}

void ExplosionManager::AddExplosion(const ShapeStatic* _shape, const Matrix34& _transform, float _fraction)
{
  if (_fraction > 0.0f)
    AddExplosion(_shape->m_rootFragment, _shape->m_defaultStates, _transform, true, _fraction);
}

void ExplosionManager::Reset()
{
  for (auto* e : m_explosions)
    delete e;
  m_explosions.clear();
}

void ExplosionManager::Advance()
{
  START_PROFILE(g_context->m_profiler, "Advance Explosions");

  for (size_t i = 0; i < m_explosions.size(); ++i)
  {
    if (m_explosions[i]->Advance())
    {
      delete m_explosions[i];
      m_explosions[i] = m_explosions.back();
      m_explosions.pop_back();
      --i;
    }
  }

  END_PROFILE(g_context->m_profiler, "Advance Explosions");
}

void ExplosionManager::Render()
{
  START_PROFILE(g_context->m_profiler, "Render Explosions");

  if (!m_explosions.empty())
  {
    CHECK_OPENGL_STATE();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/shapewireframe.bmp"));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    g_context->m_renderer->SetObjectLighting();

    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);

    for (auto* e : m_explosions)
      e->Render();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glDisable(GL_COLOR_MATERIAL);
    g_context->m_renderer->UnsetObjectLighting();

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(GL_TEXTURE_2D);
  }

  END_PROFILE(g_context->m_profiler, "Render Explosions");
}

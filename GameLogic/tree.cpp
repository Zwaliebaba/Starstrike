#include "pch.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"
#include "debug_render.h"
#include "file_writer.h"
#include "math_utils.h"
#include "resource.h"
#include "text_stream_readers.h"
#include "debug_utils.h"
#include "shape.h"
#include "preferences.h"
#include "tree.h"
#include "soundsystem.h"
#include "app.h"
#include "globals.h"
#include "particle_system.h"
#include "location.h"
#include "global_world.h"

Tree::Tree()
  : Building(),
    m_branchDisplayListId(-1),
    m_leafDisplayListId(-1),
    m_fireDamage(0.0f),
    m_onFire(0.0f),
    m_burnSoundPlaying(false),
    m_height(50.0f),
    m_budsize(1.0f),
    m_pushUp(1.0f),
    m_pushOut(1.0f),
    m_iterations(6),
    m_seed(0),
    m_leafColour(0xffffffff),
    m_branchColour(0xffffffff),
    m_leafDropRate(0) { m_type = TypeTree; }

Tree::~Tree() { DeleteDisplayLists(); }

void Tree::Initialise(Building* _template)
{
  Building::Initialise(_template);

  DarwiniaDebugAssert(_template->m_type == TypeTree);
  auto tree = static_cast<Tree*>(_template);

  m_height = tree->m_height;
  m_iterations = tree->m_iterations;
  m_budsize = tree->m_budsize;
  m_pushOut = tree->m_pushOut;
  m_pushUp = tree->m_pushUp;
  m_seed = tree->m_seed;
  m_branchColour = tree->m_branchColour;
  m_leafColour = tree->m_leafColour;
  m_leafDropRate = tree->m_leafDropRate;
}

void Tree::SetDetail(int _detail)
{
  Building::SetDetail(_detail);

  int oldIterations = m_iterations;
  if (_detail == 0)
    m_iterations = 0;
  else
    m_iterations -= (_detail - 1);
  m_iterations = max(m_iterations, 3);

  Generate();

  m_iterations = oldIterations;
  m_centrePos = m_pos + m_hitcheckCentre * m_height;
  m_radius = m_hitcheckRadius * m_height * 1.5f;
}

bool Tree::Advance()
{
  m_fireDamage -= SERVER_ADVANCE_PERIOD;
  if (m_fireDamage < 0.0f)
    m_fireDamage = 0.0f;
  if (m_fireDamage > 100.0f)
    m_fireDamage = 100.0f;

  if (m_onFire > 0.0f)
  {
    //
    // Spawn fire particle

    float actualHeight = GetActualHeight(0.0f);
    int numFire = actualHeight / 5;
    for (int i = 0; i < numFire; ++i)
    {
      LegacyVector3 fireSpawn = m_pos + LegacyVector3(0, actualHeight, 0);
      fireSpawn += LegacyVector3(sfrand(actualHeight * 1.0f), sfrand(actualHeight * 0.5f), sfrand(actualHeight * 1.0f));
      float fireSize = actualHeight * 2.0f;
      fireSize *= (1.0f + sfrand(0.5f));
      g_app->m_particleSystem->CreateParticle(fireSpawn, g_zeroVector, Particle::TypeFire, fireSize);
    }

    if (frand(100.0f) < 10.0f)
    {
      LegacyVector3 fireSpawn = m_pos + LegacyVector3(0, actualHeight, 0);
      fireSpawn += LegacyVector3(sfrand(actualHeight * 0.75f), sfrand(actualHeight * 0.75f), sfrand(actualHeight * 0.75f));
      g_app->m_particleSystem->CreateParticle(fireSpawn, g_zeroVector, Particle::TypeExplosionDebris);
    }

    //
    // Burn down

    m_onFire += SERVER_ADVANCE_PERIOD * 0.01f;
    if (m_onFire > 1.0f)
      m_onFire = -1.0f;

    //
    // Spread to nearby trees

    LegacyVector3 hitCentre = m_pos + m_hitcheckCentre * actualHeight;
    float hitRadius = m_hitcheckRadius * actualHeight;

    for (int b = 0; b < g_app->m_location->m_buildings.Size(); ++b)
    {
      if (g_app->m_location->m_buildings.ValidIndex(b))
      {
        Building* building = g_app->m_location->m_buildings[b];
        if (building != this && building->m_type == TypeTree)
        {
          auto tree = static_cast<Tree*>(building);
          float distance = (tree->m_pos - m_pos).Mag();
          float theirActualHeight = tree->GetActualHeight(0.0f);
          float theirRadius = theirActualHeight * tree->m_hitcheckRadius * 1.5f;
          float ourRadius = actualHeight * m_hitcheckRadius * 1.5f;

          if (theirRadius + ourRadius >= distance)
            tree->Damage(-1.0f);
        }
      }
    }
  }
  else if (m_onFire < 0.0f)
  {
    if (m_burnSoundPlaying)
    {
      g_app->m_soundSystem->StopAllSounds(m_id, "Tree Burn");
      g_app->m_soundSystem->TriggerBuildingEvent(this, "Create");
      m_burnSoundPlaying = false;
    }

    //
    // Regrow

    m_onFire += SERVER_ADVANCE_PERIOD * 0.01f;
    if (m_onFire > 0.0f)
      m_onFire = 0.0f;
  }

  if (m_onFire == 0.0f && m_leafDropRate > 0)
  {
    // drop some leaves
    if (rand() % (51 - m_leafDropRate) == 0)
    {
      float actualHeight = GetActualHeight(0.0f);
      LegacyVector3 fireSpawn = m_pos + LegacyVector3(0, actualHeight, 0);
      fireSpawn += LegacyVector3(sfrand(actualHeight * 1.0f), sfrand(actualHeight * 0.25f), sfrand(actualHeight * 1.0f));
      g_app->m_particleSystem->CreateParticle(fireSpawn, g_zeroVector, Particle::TypeLeaf, -1.0f,
                                              RGBAColour(m_leafColourArray[0], m_leafColourArray[1], m_leafColourArray[2]));
    }
  }

  return false;
}

float Tree::GetActualHeight(float _predictionTime)
{
  float predictedOnFire = m_onFire;
  if (predictedOnFire != 0.0f)
    predictedOnFire += _predictionTime * 0.01f;

  float actualHeight = m_height * (1.0f - fabs(predictedOnFire));
  if (actualHeight < m_height * 0.1f)
    actualHeight = m_height * 0.1f;

  return actualHeight;
}

static void intToArray(unsigned x, unsigned char* a)
{
  a[0] = x & 0xFF;
  x >>= 8;
  a[1] = x & 0xFF;
  x >>= 8;
  a[2] = x & 0xFF;
  x >>= 8;
  a[3] = x & 0xFF;
}

void Tree::DeleteDisplayLists()
{
  if (m_branchDisplayListId != -1)
    m_branchDisplayListId = -1;

  if (m_leafDisplayListId != -1)
    m_leafDisplayListId = -1;
}

void Tree::Generate()
{
  m_hitcheckCentre.Zero();
  m_hitcheckRadius = 0.0f;
  m_numLeafs = 0;

  DeleteDisplayLists();

  intToArray(m_branchColour, m_branchColourArray);
  intToArray(m_leafColour, m_leafColourArray);

  int treeDetail = g_prefsManager->GetInt("RenderTreeDetail", 1);
  if (treeDetail > 1)
  {
    int alpha = m_leafColourArray[3];
    alpha *= pow(1.3f, treeDetail);
    alpha = min(alpha, 255);
    m_leafColourArray[3] = alpha;
  }

  darwiniaSeedRandom(m_seed);
  g_imRenderer->Begin(PRIM_QUADS);
  RenderBranch(g_zeroVector, g_upVector, m_iterations, false, true, false);
  g_imRenderer->End();

  darwiniaSeedRandom(m_seed);
  g_imRenderer->Begin(PRIM_QUADS);
  RenderBranch(g_zeroVector, g_upVector, m_iterations, false, false, true);
  g_imRenderer->End();

  //
  // We now have all the leaf positions accumulated in m_hitcheckCentre
  // So we can calculate the actual centre position, then the radius

  m_hitcheckCentre /= static_cast<float>(m_numLeafs);
  RenderBranch(g_zeroVector, g_upVector, m_iterations, true, false, true);
  m_hitcheckRadius *= 0.8f;
}

void Tree::Render(float _predictionTime)
{
  //RenderHitCheck();
}

bool Tree::PerformDepthSort(LegacyVector3& _centrePos)
{
  _centrePos = m_pos + m_hitcheckCentre * m_height;
  return true;
}

void Tree::RenderAlphas(float _predictionTime)
{
  if (m_branchDisplayListId == -1 || m_leafDisplayListId == -1)
    Generate();

  if (g_app->m_editing)
  {
    intToArray(m_branchColour, m_branchColourArray);
    intToArray(m_leafColour, m_leafColourArray);
  }

  float actualHeight = GetActualHeight(_predictionTime);

  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
  g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/laser.bmp"));
  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);

  g_imRenderer->PushMatrix();
  Matrix34 mat(m_front, g_upVector, m_pos);
  g_imRenderer->MultMatrixf(mat.ConvertToOpenGLFormat());
  g_imRenderer->Scalef(actualHeight, actualHeight, actualHeight);

  if (Location::ChristmasModEnabled() == 1)
  {
    m_branchColourArray[0] = 180;
    m_branchColourArray[1] = 100;
    m_branchColourArray[2] = 50;
  }

  g_imRenderer->Color4ubv(m_branchColourArray);

  if (Location::ChristmasModEnabled() != 1)
    g_imRenderer->Color4ubv(m_leafColourArray);

  g_imRenderer->PopMatrix();

  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_BACK);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);
}

void Tree::RenderHitCheck()
{
#ifdef DEBUG_RENDER_ENABLED
  float actualHeight = GetActualHeight(0.0f);

  RenderSphere(m_pos, 10.0f);
  RenderSphere(m_pos + m_hitcheckCentre * actualHeight, m_hitcheckRadius * actualHeight);
#endif
}

void Tree::Damage(float _damage)
{
  Building::Damage(_damage);

  if (m_fireDamage < 50.0f)
  {
    m_fireDamage += _damage * -1.0f;

    if (m_fireDamage > 50.0f)
    {
      if (m_onFire == 0.0f)
        m_onFire = 0.01f;
      else if (m_onFire < 0.0f)
        m_onFire = m_onFire * -1.0f;

      if (!m_burnSoundPlaying)
      {
        g_app->m_soundSystem->StopAllSounds(m_id, "Tree Create");
        g_app->m_soundSystem->TriggerBuildingEvent(this, "Burn");
        m_burnSoundPlaying = true;
      }
    }
  }
}

bool Tree::DoesSphereHit(const LegacyVector3& _pos, float _radius)
{
  if (SphereSphereIntersection(m_pos, 10.0f, _pos, _radius))
    return true;

  float actualHeight = GetActualHeight(0.0f);

  if (SphereSphereIntersection(m_pos + m_hitcheckCentre * actualHeight, m_hitcheckRadius * actualHeight, _pos, _radius))
    return true;

  return false;
}

bool Tree::DoesShapeHit(Shape* _shape, Matrix34 _transform)
{
  SpherePackage packageA(m_pos, 10.0f);
  if (_shape->SphereHit(&packageA, _transform))
    return true;

  float actualHeight = GetActualHeight(0.0f);

  SpherePackage packageB(m_pos + m_hitcheckCentre * actualHeight, m_hitcheckRadius * actualHeight);
  if (_shape->SphereHit(&packageB, _transform))
    return true;

  return false;
}

bool Tree::DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, LegacyVector3* _pos,
                      LegacyVector3* _norm)
{
  if (RaySphereIntersection(_rayStart, _rayDir, m_pos, 10.00f, _rayLen, _pos, _norm))
    return true;

  float actualHeight = GetActualHeight(0.0f);

  if (RaySphereIntersection(_rayStart, _rayDir, m_pos + m_hitcheckCentre * actualHeight, m_hitcheckRadius * actualHeight, _rayLen, _pos,
                            _norm))
    return true;

  return false;
}

void Tree::RenderBranch(LegacyVector3 _from, LegacyVector3 _to, int _iterations, bool _calcRadius, bool _renderBranch, bool _renderLeaf)
{
  if (_iterations == 0)
    return;
  _iterations--;

  if (_iterations == 0)
  {
    if (_calcRadius)
    {
      float distToCentre = (_to - m_hitcheckCentre).Mag();
      if (distToCentre > m_hitcheckRadius)
        m_hitcheckRadius = distToCentre;
    }
    else if (_renderLeaf)
    {
      m_hitcheckCentre += _to;
      m_numLeafs++;
    }
  }

  /*
  Make the tree seem more alive animation
  _to += LegacyVector3(sinf(g_gameTime*(7-_iterations))*0.005f * _iterations,
                 sinf(g_gameTime) * 0.01f,
                 sinf(g_gameTime * (7-_iterations))*0.005f * _iterations );
  */

  LegacyVector3 rightAngleA = ((_to - _from) ^ _to).Normalise();
  LegacyVector3 rightAngleB = (rightAngleA ^ (_to - _from)).Normalise();

  LegacyVector3 thisBranch = (_to - _from);

  float thickness = thisBranch.Mag();

  float budsize = 0.1f;

  if (_iterations == 0)
    budsize *= m_budsize;

  LegacyVector3 camRightA = rightAngleA * thickness * budsize;
  LegacyVector3 camRightB = rightAngleB * thickness * budsize;

  if ((_iterations == 0 && _renderLeaf) || (_iterations != 0 && _renderBranch)) {}

  int numBranches = 4;

  for (int i = 0; i < numBranches; ++i)
  {
    LegacyVector3 thisRightAngle;
    if (i == 0)
      thisRightAngle = rightAngleA;
    if (i == 1)
      thisRightAngle = -rightAngleA;
    if (i == 2)
      thisRightAngle = rightAngleB;
    if (i == 3)
      thisRightAngle = -rightAngleB;

    float distance = 0.3f + frand(0.6f);
    LegacyVector3 thisFrom = _from + thisBranch * distance;
    LegacyVector3 thisTo = thisFrom + thisRightAngle * thickness * 0.4f * m_pushOut + thisBranch * (1.0f - distance) * m_pushUp;
    RenderBranch(thisFrom, thisTo, _iterations, _calcRadius, _renderBranch, _renderLeaf);
  }
}

void Tree::ListSoundEvents(LList<char*>* _list)
{
  Building::ListSoundEvents(_list);

  _list->PutData("Burn");
}

void Tree::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_height = atof(_in->GetNextToken());
  m_budsize = atof(_in->GetNextToken());
  m_pushOut = atof(_in->GetNextToken());
  m_pushUp = atof(_in->GetNextToken());
  m_iterations = atoi(_in->GetNextToken());
  m_seed = atoi(_in->GetNextToken());
  m_branchColour = atoi(_in->GetNextToken());
  m_leafColour = atoi(_in->GetNextToken());
  if (_in->TokenAvailable())
    m_leafDropRate = atoi(_in->GetNextToken());
}

void Tree::Write(FileWriter* _out)
{
  Building::Write(_out);

  _out->printf("%-8.2f", m_height);
  _out->printf("%-8.2f", m_budsize);
  _out->printf("%-8.2f", m_pushOut);
  _out->printf("%-8.2f", m_pushUp);
  _out->printf("%-8d", m_iterations);
  _out->printf("%-8d", m_seed);
  _out->printf("%-12d", m_branchColour);
  _out->printf("%-12d", m_leafColour);
  _out->printf("%-8d", m_leafDropRate);
}

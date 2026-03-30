#include "pch.h"
#include "text_stream_readers.h"
#include "ShapeStatic.h"
#include "file_writer.h"
#include "resource.h"
#include "math_utils.h"
#include "hi_res_time.h"
#include "preferences.h"
#include "language_table.h"
#include "GameContext.h"
#include "camera.h"
#include "global_world.h"
#include "location.h"
#include "main.h"
#include "GameSimEventQueue.h"
#include "mine.h"
#include "constructionyard.h"
#include "trunkport.h"

// ****************************************************************************
// Class MineBuilding
// ****************************************************************************

ShapeStatic* MineBuilding::s_wheelShape = nullptr;

ShapeStatic* MineBuilding::s_cartShape = nullptr;
ShapeMarkerData* MineBuilding::s_cartMarker1 = nullptr;
ShapeMarkerData* MineBuilding::s_cartMarker2 = nullptr;
ShapeMarkerData* MineBuilding::s_cartContents[] = {nullptr, nullptr, nullptr};

ShapeStatic* MineBuilding::s_polygon1 = nullptr;
ShapeStatic* MineBuilding::s_primitive1 = nullptr;

float MineBuilding::s_refineryPopulation = 0.0f;
float MineBuilding::s_refineryRecalculateTimer = 0.0f;

MineBuilding::MineBuilding()
  : Building(),
    m_trackLink(-1),
    m_trackMarker1(nullptr),
    m_trackMarker2(nullptr),
    m_previousMineSpeed(0.0f),
    m_wheelRotate(0.0f)
{
  if (!s_cartShape)
  {
    s_wheelShape = Resource::GetShapeStatic("wheel.shp");
    s_cartShape = Resource::GetShapeStatic("minecart.shp");
    s_cartMarker1 = s_cartShape->GetMarkerData("MarkerTrack1");
    s_cartMarker2 = s_cartShape->GetMarkerData("MarkerTrack2");
    s_cartContents[0] = s_cartShape->GetMarkerData("MarkerContents1");
    s_cartContents[1] = s_cartShape->GetMarkerData("MarkerContents2");
    s_cartContents[2] = s_cartShape->GetMarkerData("MarkerContents3");

    s_polygon1 = Resource::GetShapeStatic("minepolygon1.shp");
    s_primitive1 = Resource::GetShapeStatic("mineprimitive1.shp");
  }
}

void MineBuilding::Initialise(Building* _template)
{
  Building::Initialise(_template);
  m_trackLink = static_cast<MineBuilding*>(_template)->m_trackLink;

  float cart = 0.0f;

  while (true)
  {
    cart += 0.2f;
    cart += syncfrand(1.5f);
    if (cart >= 1.0f)
      break;

    auto mineCart = new MineCart();
    mineCart->m_progress = cart;
    m_carts.PutData(mineCart);
  }
}

bool MineBuilding::IsInView()
{
  Building* trackLink = g_context->m_location->GetBuilding(m_trackLink);
  if (trackLink)
  {
    LegacyVector3 midPoint = (trackLink->m_centerPos + m_centerPos) / 2.0f;
    float radius = (trackLink->m_centerPos - m_centerPos).Mag() / 2.0f;
    radius += m_radius;

    if (g_context->m_camera->SphereInViewFrustum(midPoint, radius))
      return true;
  }

  return Building::IsInView();
}

bool MineBuilding::Advance()
{
  float mineSpeed = RefinerySpeed();
  if (m_previousMineSpeed <= 0.1f && mineSpeed > 0.1f)
    g_simEventQueue.Push(SimEvent::MakeSoundBuilding(m_id, "CogTurn"));
  m_previousMineSpeed = mineSpeed;

  if (mineSpeed > 0.0f)
  {
    for (int i = m_carts.Size() - 1; i >= 0; --i)
    {
      MineCart* thisCart = m_carts[i];
      thisCart->m_progress += mineSpeed * SERVER_ADVANCE_PERIOD;

      if (thisCart->m_progress >= 1.0f)
      {
        float remainder = thisCart->m_progress - 1.0f;
        m_carts.RemoveData(i);
        --i;

        Building* trackLink = g_context->m_location->GetBuilding(m_trackLink);
        if (trackLink)
        {
          auto mineBuilding = static_cast<MineBuilding*>(trackLink);
          mineBuilding->TriggerCart(thisCart, remainder);
        }
      }
    }
  }

  //
  // Rotate our wheels

  m_wheelRotate -= 3.0f * mineSpeed * SERVER_ADVANCE_PERIOD;

  return Building::Advance();
}

void MineBuilding::TriggerCart(MineCart* _cart, float _initValue)
{
  _cart->m_progress = _initValue;
  m_carts.PutDataAtStart(_cart);
}

LegacyVector3 MineBuilding::GetTrackMarker1()
{
  if (!m_trackMarker1 || g_context->m_editing)
  {
    m_trackMarker1 = m_shape->GetMarkerData("MarkerTrack1");
    DEBUG_ASSERT(m_trackMarker1);
    Matrix34 rootMat(m_front, g_upVector, m_pos);
    m_trackMatrix1 = m_shape->GetMarkerWorldMatrix(m_trackMarker1, rootMat);
  }

  return m_trackMatrix1.pos;
}

LegacyVector3 MineBuilding::GetTrackMarker2()
{
  if (!m_trackMarker2 || g_context->m_editing)
  {
    m_trackMarker2 = m_shape->GetMarkerData("MarkerTrack2");
    DEBUG_ASSERT(m_trackMarker2);
    Matrix34 rootMat(m_front, g_upVector, m_pos);
    m_trackMatrix2 = m_shape->GetMarkerWorldMatrix(m_trackMarker2, rootMat);
  }

  return m_trackMatrix2.pos;
}

void MineBuilding::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);
  m_trackLink = atoi(_in->GetNextToken());
}

void MineBuilding::Write(FileWriter* _out)
{
  Building::Write(_out);
  _out->printf("%-8d", m_trackLink);
}

int MineBuilding::GetBuildingLink() { return m_trackLink; }

void MineBuilding::SetBuildingLink(int _buildingId) { m_trackLink = _buildingId; }

float MineBuilding::RefinerySpeed()
{
  if (GetHighResTime() >= s_refineryRecalculateTimer)
  {
    //
    // Find the refinery
    // If not a refinery, look for a construction yard
    // If not, look for a fuel generator

    Building* driver = nullptr;

    int numFuelGenerators = 0;
    float fuelGeneratorFactor = 0.0f;

    for (int i = 0; i < g_context->m_location->m_buildings.Size(); ++i)
    {
      if (g_context->m_location->m_buildings.ValidIndex(i))
      {
        Building* building = g_context->m_location->m_buildings[i];
        if (building->m_type == TypeRefinery || building->m_type == TypeYard)
        {
          driver = building;
          break;
        }
      }
    }

    //
    // If we found a refinery calculate our speed based on that
    // If not, there may be a construction yard driving us
    // If not, there may be a Fuel Generator driving us
    // Otherwise assume the refinery is on a different level, and use global state data

    if (driver)
    {
      int numPorts = driver->GetNumPorts();
      int numPortsOccupied = driver->GetNumPortsOccupied();
      s_refineryPopulation = static_cast<float>(numPortsOccupied) / static_cast<float>(numPorts);
    }
    else if (numFuelGenerators > 0)
      s_refineryPopulation = fuelGeneratorFactor / static_cast<float>(numFuelGenerators);
    else
    {
      int mineLocationId = g_context->m_globalWorld->GetLocationId("mine");
      s_refineryPopulation = 0.0f;

      for (int i = 0; i < g_context->m_globalWorld->m_buildings.Size(); ++i)
      {
        if (g_context->m_globalWorld->m_buildings.ValidIndex(i))
        {
          GlobalBuilding* gb = g_context->m_globalWorld->m_buildings[i];
          if (gb && gb->m_locationId == mineLocationId && gb->m_type == TypeRefinery && gb->m_online)
          {
            s_refineryPopulation = 1.0f;
            break;
          }
        }
      }
    }

    s_refineryRecalculateTimer = GetHighResTime() + 0.1f;
  }

  float speed = s_refineryPopulation * fabs(sinf(g_gameTime * 2.0f)) * 0.5f;

  return speed;
}

// ****************************************************************************
// Class MineCart
// ****************************************************************************

MineCart::MineCart()
  : m_progress(0.0f)
{
  for (int i = 0; i < 3; ++i)
  {
    m_polygons[i] = false;
    m_primitives[i] = false;
  }
}

// ****************************************************************************
// Class TrackLink
// ****************************************************************************

TrackLink::TrackLink()
  : MineBuilding()
{
  m_type = TypeTrackLink;
  SetShape(Resource::GetShapeStatic("tracklink.shp"));
}

bool TrackLink::Advance() { return MineBuilding::Advance(); }

// ****************************************************************************
// Class TrackJunction
// ****************************************************************************

TrackJunction::TrackJunction()
  : MineBuilding()
{
  m_type = TypeTrackJunction;
  SetShape(Resource::GetShapeStatic("tracklink.shp"));
}

void TrackJunction::Initialise(Building* _template)
{
  MineBuilding::Initialise(_template);

  auto trackJunction = static_cast<TrackJunction*>(_template);
  for (int i = 0; i < trackJunction->m_trackLinks.Size(); ++i)
  {
    int trackLink = trackJunction->m_trackLinks[i];
    m_trackLinks.PutData(trackLink);
  }
}

void TrackJunction::TriggerCart(MineCart* _cart, float _initValue)
{
  if (m_trackLinks.Size() > 0)
  {
    int chosenLink = syncrand() % m_trackLinks.Size();
    int buildingId = m_trackLinks[chosenLink];
    Building* linkBuilding = g_context->m_location->GetBuilding(buildingId);
    if (linkBuilding)
    {
      auto mine = static_cast<MineBuilding*>(linkBuilding);
      mine->TriggerCart(_cart, _initValue);
    }
  }
}

void TrackJunction::SetBuildingLink(int _buildingId) { m_trackLinks.PutData(_buildingId); }

void TrackJunction::Read(TextReader* _in, bool _dynamic)
{
  MineBuilding::Read(_in, _dynamic);

  while (_in->TokenAvailable())
  {
    int trackLink = atoi(_in->GetNextToken());
    m_trackLinks.PutData(trackLink);
  }
}

void TrackJunction::Write(FileWriter* _out)
{
  MineBuilding::Write(_out);

  for (int i = 0; i < m_trackLinks.Size(); ++i)
    _out->printf("%-4d", m_trackLinks[i]);
}

// ****************************************************************************
// Class TrackStart
// ****************************************************************************

TrackStart::TrackStart()
  : MineBuilding(),
    m_reqBuildingId(-1)
{
  m_type = TypeTrackStart;
  SetShape(Resource::GetShapeStatic("tracklink.shp"));
}

void TrackStart::Initialise(Building* _template)
{
  MineBuilding::Initialise(_template);

  m_reqBuildingId = static_cast<TrackStart*>(_template)->m_reqBuildingId;
}

bool TrackStart::Advance()
{
  //
  // Is our required building online yet?
  // Fill carts with primitives when they reach 50%

  GlobalBuilding* globalBuilding = g_context->m_globalWorld->GetBuilding(m_reqBuildingId, g_context->m_locationId);

  if (globalBuilding && globalBuilding->m_online)
  {
    for (int i = 0; i < m_carts.Size(); ++i)
    {
      MineCart* cart = m_carts[i];

      if (cart->m_progress > 0.5f)
      {
        bool primitiveFound = false;
        for (int j = 0; j < 3; ++j)
        {
          if (cart->m_primitives[j])
          {
            primitiveFound = true;
            break;
          }
        }

        if (!primitiveFound)
        {
          int primIndex = syncrand() % 3;
          cart->m_primitives[primIndex] = true;
        }
      }
    }
  }

  return MineBuilding::Advance();
}

void TrackStart::Read(TextReader* _in, bool _dynamic)
{
  MineBuilding::Read(_in, _dynamic);

  m_reqBuildingId = atoi(_in->GetNextToken());
}

void TrackStart::Write(FileWriter* _out)
{
  MineBuilding::Write(_out);

  _out->printf("%-8d", m_reqBuildingId);
}

// ****************************************************************************
// Class TrackEnd
// ****************************************************************************

TrackEnd::TrackEnd()
  : MineBuilding(),
    m_reqBuildingId(-1)
{
  m_type = TypeTrackEnd;
  SetShape(Resource::GetShapeStatic("trackLink.shp"));
}

void TrackEnd::Initialise(Building* _template)
{
  MineBuilding::Initialise(_template);

  m_reqBuildingId = static_cast<TrackEnd*>(_template)->m_reqBuildingId;
}

bool TrackEnd::Advance()
{
  //
  // Is our required building online yet?
  // Empty carts of primitives when they reach 50%

  //GlobalBuilding *globalBuilding = g_context->m_globalWorld->GetBuilding( m_reqBuildingId, g_context->m_locationId );
  Building* building = g_context->m_location->GetBuilding(m_reqBuildingId);

  bool online = false;
  if (building->m_type == TypeTrunkPort && static_cast<TrunkPort*>(building)->m_openTimer > 0.0f)
    online = true;
  if (building->m_type == TypeYard)
    online = true;
  if (building->m_type == TypeFuelGenerator)
    online = true;

  if (online)
  {
    for (int i = 0; i < m_carts.Size(); ++i)
    {
      MineCart* cart = m_carts[i];

      if (cart->m_progress > 0.5f)
      {
        for (int j = 0; j < 3; ++j)
        {
          if (cart->m_primitives[j])
          {
            if (building && building->m_type == TypeYard)
            {
              auto yard = static_cast<ConstructionYard*>(building);
              bool added = yard->AddPrimitive();
              if (added)
                cart->m_primitives[j] = false;
            }
            else
              cart->m_primitives[j] = false;
          }

          if (cart->m_polygons[j])
            cart->m_polygons[j] = false;
        }
      }
    }
  }

  return MineBuilding::Advance();
}

void TrackEnd::Read(TextReader* _in, bool _dynamic)
{
  MineBuilding::Read(_in, _dynamic);

  m_reqBuildingId = atoi(_in->GetNextToken());
}

void TrackEnd::Write(FileWriter* _out)
{
  MineBuilding::Write(_out);

  _out->printf("%-8d", m_reqBuildingId);
}

// ****************************************************************************
// Class Refinery
// ****************************************************************************

Refinery::Refinery()
  : MineBuilding(),
    m_wheel1(nullptr),
    m_wheel2(nullptr),
    m_wheel3(nullptr),
    m_counter1(nullptr)
{
  m_type = TypeRefinery;
  SetShape(Resource::GetShapeStatic("refinery.shp"));

  m_wheel1 = m_shape->GetMarkerData("MarkerWheel01");
  m_wheel2 = m_shape->GetMarkerData("MarkerWheel02");
  m_wheel3 = m_shape->GetMarkerData("MarkerWheel03");

  m_counter1 = m_shape->GetMarkerData("MarkerCounter");
}

const char* Refinery::GetObjectiveCounter()
{
  GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_context->m_locationId);
  int numRefined = 0;
  if (gb)
    numRefined = gb->m_link;

  static char result[256];
  snprintf(result, sizeof(result), "%s : %d", LANGUAGEPHRASE("objective_refined"), numRefined);
  return result;
}

bool Refinery::Advance()
{
  GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_context->m_locationId);

  if (gb && gb->m_link == -1)
  {
    // This occurs the first time
    gb->m_link = 0;
  }

  if (gb && gb->m_link >= 20)
  {
    if (!gb->m_online)
    {
      gb->m_online = true;
      g_context->m_globalWorld->EvaluateEvents();
    }
  }

  return MineBuilding::Advance();
}

void Refinery::TriggerCart(MineCart* _cart, float _initValue)
{
  // The refinery MUST be online at this point,
  // otherwise the carts would never have reached us

  bool polygonsFound = false;
  bool primitivesFound = false;

  for (int i = 0; i < 3; ++i)
  {
    if (_cart->m_polygons[i])
      polygonsFound = true;
    if (_cart->m_primitives[i])
      primitivesFound = true;
  }

  if (!primitivesFound && polygonsFound)
  {
    for (int i = 0; i < 3; ++i)
      _cart->m_polygons[i] = false;

    int primIndex = syncrand() % 3;
    _cart->m_primitives[primIndex] = true;

    GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_context->m_locationId);

    if (gb)
      gb->m_link++;
  }

  MineBuilding::TriggerCart(_cart, _initValue);
}

// ****************************************************************************
// Class Mine
// ****************************************************************************

Mine::Mine()
  : MineBuilding(),
    m_wheel1(nullptr),
    m_wheel2(nullptr)
{
  m_type = TypeMine;
  SetShape(Resource::GetShapeStatic("mine.shp"));

  m_wheel1 = m_shape->GetMarkerData("MarkerWheel01");
  m_wheel2 = m_shape->GetMarkerData("MarkerWheel02");
}

void Mine::TriggerCart(MineCart* _cart, float _initValue)
{
  //
  // If there is anything already in here, don't do anything

  bool somethingFound = false;
  for (int i = 0; i < 3; ++i)
  {
    if (_cart->m_primitives[i] || _cart->m_polygons[i])
    {
      somethingFound = true;
      break;
    }
  }

  if (!somethingFound)
  {
    float fractionOccupied = static_cast<float>(GetNumPortsOccupied()) / static_cast<float>(GetNumPorts());

    if (syncfrand() <= fractionOccupied)
    {
      for (int i = 0; i < 3; ++i)
      {
        _cart->m_primitives[i] = false;
        int cartIndex = syncrand() % 3;
        _cart->m_polygons[cartIndex] = true;
      }
    }
  }

  MineBuilding::TriggerCart(_cart, _initValue);
}

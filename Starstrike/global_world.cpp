#include "pch.h"
#include "language_table.h"
#include "filesys_utils.h"
#include "file_writer.h"
#include "input.h"
#include "targetcursor.h"
#include "math_utils.h"
#include "profiler.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "string_utils.h"
#include "text_renderer.h"
#include "text_stream_readers.h"
#include "LegacyVector3.h"
#include "eclipse.h"
#include "GameApp.h"
#include "camera.h"
#include "global_internet.h"
#include "global_world.h"
#include "landscape.h"
#include "level_file.h"
#include "location.h"
#include "main.h"
#include "renderer.h"
#include "script.h"
#include "user_input.h"
#include "taskmanager_interface.h"
#include "building.h"
#include "trunkport.h"

GlobalLocation::GlobalLocation()
  : m_id(-1),
    m_available(false),
    m_name("none"),
    m_mapFilename("none"),
    m_missionCompleted(false),
    m_missionFilename("none"),
    m_numSpirits(0)
{}

void GlobalLocation::AddSpirits(int _count) { m_numSpirits += _count; }

// ****************************************************************************
// Class GlobalBuilding
// ****************************************************************************

GlobalBuilding::GlobalBuilding()
  : m_id(-1),
    m_teamId(-1),
    m_locationId(-1),
    m_type(Building::TypeTrunkPort),
    m_online(false),
    m_link(-1),
    m_shape(nullptr) { m_shape = Resource::GetShapeStatic("trunkport.shp"); }

// ****************************************************************************
// Class GlobalEventCondition
// ****************************************************************************

GlobalEventCondition::GlobalEventCondition()
  : m_type(-1),
    m_id(-1),
    m_locationId(-1),
    m_stringId(nullptr),
    m_cutScene(nullptr) {}

GlobalEventCondition::GlobalEventCondition(const GlobalEventCondition& _other)
  : m_type(_other.m_type),
    m_id(_other.m_id),
    m_locationId(_other.m_locationId),
    m_stringId(NewStr(_other.m_stringId)),
    m_cutScene(NewStr(_other.m_cutScene)) {}

GlobalEventCondition::~GlobalEventCondition()
{
  delete [] m_stringId;
  delete [] m_cutScene;
}

void GlobalEventCondition::SetStringId(const char* _stringId)
{
  delete [] m_stringId;
  m_stringId = NewStr(_stringId);
}

void GlobalEventCondition::SetCutScene(char* _cutScene)
{
  delete [] m_cutScene;
  m_cutScene = NewStr(_cutScene);
}

const char* GlobalEventCondition::GetTypeName(int _type)
{
  static const char* names[] = {"AlwaysTrue", "BuildingOnline", "BuildingOffline", "ResearchOwned", "NotInLocation", "DebugKey", "NeverTrue"};

  DEBUG_ASSERT(_type >= 0 && _type < NumConditions);

  return names[_type];
}

int GlobalEventCondition::GetType(const char* _typeName)
{
  for (int i = 0; i < NumConditions; ++i)
  {
    if (_stricmp(_typeName, GetTypeName(i)) == 0)
      return i;
  }

  return -1;
}

bool GlobalEventCondition::Evaluate()
{
  switch (m_type)
  {
  case AlwaysTrue:
    return true;

  case BuildingOnline:
    {
      GlobalBuilding* building = g_context->m_globalWorld->GetBuilding(m_id, m_locationId);
      if (building)
        return building->m_online;
      break;
    }

  case BuildingOffline:
    {
      GlobalBuilding* building = g_context->m_globalWorld->GetBuilding(m_id, m_locationId);
      if (building)
        return !building->m_online;
      break;
    }

  case ResearchOwned:
    return (g_context->m_globalWorld->m_research->HasResearch(m_id));

  case NotInLocation:
    return (g_context->m_location == nullptr);

case DebugKey:
  // JAMES_FIX: g_keys[] is internal to inputdriver_win32.cpp with no extern.
  // DebugKey conditions in data files will evaluate as false.
  return false;

case NeverTrue:
    return false;

  default: DEBUG_ASSERT(false);
  }

  return false;
}

void GlobalEventCondition::Save(FileWriter* _out)
{
  _out->printf("%s ", GetTypeName(m_type));

  switch (m_type)
  {
  case AlwaysTrue:
    break;

  case BuildingOnline:
  case BuildingOffline:
    _out->printf(":%s,%d ", g_context->m_globalWorld->GetLocationName(m_locationId), m_id);
    break;

  case ResearchOwned:
    _out->printf(":%s ", GlobalResearch::GetTypeName(m_id));
    break;

  case DebugKey:
    _out->printf(":%d ", m_id);
    break;
  }
}

// ****************************************************************************
// Class GlobalEventAction
// ****************************************************************************

const char* GlobalEventAction::GetTypeName(int _type)
{
  static const char* names[] = {"SetMission", "RunScript", "MakeAvailable"};

  DEBUG_ASSERT(_type >= 0 && _type < NumActionTypes);

  return names[_type];
}

GlobalEventAction::GlobalEventAction() : m_type(-1), m_locationId(-1), m_filename("null") {}

void GlobalEventAction::Read(TextReader* _in)
{
  char* action = _in->GetNextToken();

  if (_stricmp(action, "SetMission") == 0)
  {
    m_type = SetMission;
    m_locationId = g_context->m_globalWorld->GetLocationId(_in->GetNextToken());
    DEBUG_ASSERT(m_locationId != -1);
    m_filename = _in->GetNextToken();
  }
  else if (_stricmp(action, "RunScript") == 0)
  {
    m_type = RunScript;
    m_filename = _in->GetNextToken();
  }
  else if (_stricmp(action, "MakeAvailable") == 0)
  {
    m_type = MakeAvailable;
    m_locationId = g_context->m_globalWorld->GetLocationId(_in->GetNextToken());
    DEBUG_ASSERT(m_locationId != -1);
  }
  else { DEBUG_ASSERT(false); }
}

void GlobalEventAction::Write(FileWriter* _out)
{
  _out->printf("\t\tAction %-10s ", GetTypeName(m_type));

  const char* locationName = g_context->m_globalWorld->GetLocationName(m_locationId);

  switch (m_type)
  {
  case SetMission:
    _out->printf("%s %s", locationName, m_filename.c_str());
    break;
  case RunScript:
    _out->printf("%s", m_filename.c_str());
    break;
  case MakeAvailable:
    _out->printf("%s", locationName);
    break;

  default: DEBUG_ASSERT(false);
  }

  _out->printf("\n");
}

void GlobalEventAction::Execute()
{
  switch (m_type)
  {
  case SetMission:
    {
      GlobalLocation* loc = g_context->m_globalWorld->GetLocation(m_locationId);
      DEBUG_ASSERT(loc);
      loc->m_missionFilename = m_filename;
      break;
    }
  case RunScript:
    {
      g_context->m_script->RunScript(m_filename.c_str());
      break;
    }
  case MakeAvailable:
    {
      GlobalLocation* loc = g_context->m_globalWorld->GetLocation(m_locationId);
      DEBUG_ASSERT(loc);
      loc->m_available = true;
      break;
    }

  default: DEBUG_ASSERT(false);
  }
}

// ****************************************************************************
// Class GlobalEvent
// ****************************************************************************

GlobalEvent::GlobalEvent() {}

bool GlobalEvent::Evaluate()
{
  bool success = true;

  for (int i = 0; i < m_conditions.Size(); ++i)
  {
    GlobalEventCondition* condition = m_conditions[i];
    if (!condition->Evaluate())
    {
      success = false;
      break;
    }
  }

  return success;
}

bool GlobalEvent::Execute()
{
  if (m_actions.Size() == 0)
    return true;

  GlobalEventAction* action = m_actions[0];
  m_actions.RemoveData(0);
  action->Execute();
  delete action;

  if (m_actions.Size() == 0)
    return true;

  return false;
}

void GlobalEvent::MakeAlwaysTrue()
{
  if (m_conditions.Size() == 1 && m_conditions[0]->m_type == GlobalEventCondition::AlwaysTrue)
    return;

  m_conditions.EmptyAndDelete();

  auto cond = new GlobalEventCondition();
  cond->m_type = GlobalEventCondition::AlwaysTrue;
  m_conditions.PutData(cond);
}

void GlobalEvent::Read(TextReader* _in)
{
  //
  // Parse conditions line

  while (_in->TokenAvailable())
  {
    char* conditionTypeName = _in->GetNextToken();

    auto condition = new GlobalEventCondition;
    condition->m_type = condition->GetType(conditionTypeName);
    DEBUG_ASSERT(condition->m_type != -1);

    switch (condition->m_type)
    {
    case GlobalEventCondition::AlwaysTrue:
    case GlobalEventCondition::NotInLocation:
      break;

    case GlobalEventCondition::BuildingOffline:
    case GlobalEventCondition::BuildingOnline:
      condition->m_locationId = g_context->m_globalWorld->GetLocationId(_in->GetNextToken());
      condition->m_id = atoi(_in->GetNextToken());
      DEBUG_ASSERT(condition->m_locationId != -1);
      break;

    case GlobalEventCondition::ResearchOwned:
      condition->m_id = GlobalResearch::GetType(_in->GetNextToken());
      DEBUG_ASSERT(condition->m_id != -1);
      break;

    case GlobalEventCondition::DebugKey:
      condition->m_id = atoi(_in->GetNextToken());
      break;
    }

    m_conditions.PutData(condition);
  }

  //
  // Parse actions

  while (_in->ReadLine())
  {
    if (_in->TokenAvailable())
    {
      char* word = _in->GetNextToken();
      if (_stricmp(word, "end") == 0)
        break;
      DEBUG_ASSERT(_stricmp( word, "action" ) == 0);

      auto action = new GlobalEventAction;
      action->Read(_in);
      m_actions.PutData(action);
    }
  }
}

void GlobalEvent::Write(FileWriter* _out)
{
  _out->printf("\tEvent ");

  for (int i = 0; i < m_conditions.Size(); ++i)
  {
    GlobalEventCondition* gec = m_conditions[i];
    gec->Save(_out);
  }

  _out->printf("\n");

  for (int i = 0; i < m_actions.Size(); ++i)
  {
    GlobalEventAction* gea = m_actions[i];
    gea->Write(_out);
  }

  _out->printf("\t\tEnd\n");
}

// ****************************************************************************
// Class GlobalResearch
// ****************************************************************************

GlobalResearch::GlobalResearch()
  : m_researchPoints(0),
    m_researchTimer(0.0f)
{
  for (int i = 0; i < NumResearchItems; ++i)
  {
    m_researchLevel[i] = 0;
    m_researchProgress[i] = 0;
  }

  m_currentResearch = TypeSquad;
}

void GlobalResearch::AddResearch(int _type)
{
  DEBUG_ASSERT(_type >= 0 && _type < NumResearchItems);

  if (m_researchLevel[_type] < 1)
  {
    m_researchProgress[_type] = RequiredProgress(0);
    EvaluateLevel(_type);
  }
}

bool GlobalResearch::HasResearch(int _type)
{
  DEBUG_ASSERT(_type >= 0 && _type < NumResearchItems);

  return (m_researchLevel[_type] > 0);
}

int GlobalResearch::CurrentProgress(int _type)
{
  DEBUG_ASSERT(_type >= 0 && _type < NumResearchItems);

  return m_researchProgress[_type];
}

int GlobalResearch::RequiredProgress(int _level)
{
  DEBUG_ASSERT(_level >= 0 && _level < 4);

  static int s_requiredProgress[] = {1, 50, 100, 200};

  return s_requiredProgress[_level];
}

void GlobalResearch::EvaluateLevel(int _type)
{
  int currentLevel = m_researchLevel[_type];

  if (currentLevel < 4)
  {
    int requiredProgress = RequiredProgress(currentLevel);

    if (m_researchProgress[_type] >= requiredProgress)
    {
      // Level change has just occurred
      m_researchLevel[_type]++;
      m_researchProgress[_type] -= requiredProgress;

      if (currentLevel > 0)
      {
        // This action should only go off if a player UPGRADES an existing research item
        // Not if he finds a new one
        g_context->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageResearchUpgrade, _type, 4.0f);
      }
    }
  }
}

void GlobalResearch::SetCurrentResearch(int _type)
{
  int currentLevel = m_researchLevel[_type];

  if (currentLevel == 4)
  {
    // Fully researched already
    return;
  }

  if (m_currentResearch != _type)
  {
    m_currentResearch = _type;
  }
}

void GlobalResearch::GiveResearchPoints(int _numPoints) { m_researchPoints += _numPoints; }

void GlobalResearch::AdvanceResearch()
{
  if (m_researchPoints > 0 && CurrentLevel(m_currentResearch) < 4)
  {
    m_researchTimer -= g_advanceTime;
    if (m_researchTimer <= 0.0f)
    {
      m_researchTimer = GLOBALRESEARCH_TIMEPERPOINT;
      IncreaseProgress(1);
      --m_researchPoints;
    }
  }
}

void GlobalResearch::SetCurrentProgress(int _type, int _progress)
{
  DEBUG_ASSERT(_type >= 0 && _type < NumResearchItems);

  m_researchProgress[_type] = _progress;
  EvaluateLevel(_type);
}

void GlobalResearch::IncreaseProgress(int _amount)
{
  m_researchProgress[m_currentResearch] += _amount;
  EvaluateLevel(m_currentResearch);
}

void GlobalResearch::DecreaseProgress(int _amount)
{
  m_researchProgress[m_currentResearch] -= _amount;
  EvaluateLevel(m_currentResearch);
}

int GlobalResearch::CurrentLevel(int _type)
{
  DEBUG_ASSERT(_type >= 0 && _type < NumResearchItems);

  return m_researchLevel[_type];
}

void GlobalResearch::Write(FileWriter* _out)
{
  _out->printf("Research_StartDefinition\n");

  for (int i = 0; i < NumResearchItems; ++i)
    _out->printf("\tResearch %s %d %d\n", GetTypeName(i), CurrentProgress(i), CurrentLevel(i));

  _out->printf("\tCurrentResearch %s\n", GetTypeName(m_currentResearch));
  _out->printf("\tCurrentPoints %d\n", m_researchPoints);
  _out->printf("Research_EndDefinition\n\n");
}

void GlobalResearch::Read(TextReader* _in)
{
  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;
    char* word = _in->GetNextToken();

    if (_stricmp(word, "research_enddefinition") == 0)
      return;
    if (_stricmp(word, "Research") == 0)
    {
      char* type = _in->GetNextToken();
      int progress = atoi(_in->GetNextToken());
      int level = atoi(_in->GetNextToken());

      int researchType = GetType(type);
      m_researchLevel[researchType] = level;
      SetCurrentProgress(researchType, progress);
    }
    else if (_stricmp(word, "CurrentResearch") == 0)
    {
      char* type = _in->GetNextToken();
      int researchType = GetType(type);
      m_currentResearch = researchType;
    }
    else if (_stricmp(word, "CurrentPoints") == 0)
    {
      int points = atoi(_in->GetNextToken());
      m_researchPoints = points;
    }
    else
      ASSERT_TEXT(false, "Error loading GlobalResearch");
  }
}

const char* GlobalResearch::GetTypeName(int _type)
{
  const char* names[] = {
    "Darwinian", "Officer", "Squad", "Laser", "Grenade", "Rocket", "Controller", "AirStrike", "Armour", "TaskManager", "Engineer"
  };

  DEBUG_ASSERT(_type >= 0 && _type < NumResearchItems);
  return names[_type];
}

const char* GlobalResearch::GetTypeNameTranslated(int _type)
{
  const char* typeName = GetTypeName(_type);

  auto stringId = std::format("researchname_{}", typeName);

  if (ISLANGUAGEPHRASE(stringId.c_str()))
    return LANGUAGEPHRASE(stringId.c_str());
  return typeName;
}

int GlobalResearch::GetType(const char* _name)
{
  for (int i = 0; i < NumResearchItems; ++i)
  {
    if (_stricmp(_name, GetTypeName(i)) == 0)
      return i;
  }

  return -1;
}

// ****************************************************************************
// Class GlobalWorld
// ****************************************************************************

void ColourShapeFragment(ShapeFragmentData* _frag, const RGBAColour& _colour)
{
  if (_frag->m_numColours == 0)
    _frag->m_colours = new RGBAColour [1];
  _frag->m_colours[0] = _colour;

  for (unsigned int i = 0; i < _frag->m_numVertices; ++i)
  {
    VertexPosCol* vert = &_frag->m_vertices[i];
    vert->m_colId = 0;
  }

  for (int i = 0; i < _frag->m_childFragments.Size(); ++i)
    ColourShapeFragment(_frag->m_childFragments[i], _colour);
}

SphereWorld::SphereWorld()
  : m_shapeOuter(nullptr),
    m_shapeMiddle(nullptr),
    m_shapeInner(nullptr),
    m_numLocations(0),
    m_spirits(nullptr)
{
  m_shapeOuter = Resource::GetShapeStatic("globalworld_outer.shp");
  m_shapeMiddle = Resource::GetShapeStatic("globalworld_middle.shp");
  m_shapeInner = Resource::GetShapeStatic("globalworld_inner.shp");
}

void SphereWorld::AddLocation(int _locationId)
{
  // Initialise the sphere world to
  if (_locationId < m_numLocations)
    return;

  int oldNumLocations = m_numLocations;
  m_numLocations = _locationId + 1;
  auto newSpirits = new LList<float>[m_numLocations];

  // Initialise the spirits for the new worlds.
  for (int locationId = 0; locationId < m_numLocations; ++locationId)
  {
    if (locationId < oldNumLocations)
    {
      // Copy across the old spirits
      newSpirits[locationId] = m_spirits[locationId];
    }
    else
    {
      // Generate some new spirits
      GlobalLocation* loc = g_context->m_globalWorld->GetLocation(locationId);
      if (loc)
      {
        if (loc->m_name == "receiver")
        {
          for (int i = 0; i < 60; ++i)
          {
            float value = frand();
            newSpirits[locationId].PutData(value);
          }
        }
        else
        {
          for (int i = 0; i < 10; ++i)
          {
            float value = frand();
            newSpirits[locationId].PutData(value);
          }
        }
      }
    }
  }

  delete [] m_spirits;
  m_spirits = newSpirits;
}

void SphereWorld::Render()
{
  // For some reason this fixes a slight glitch in the first few frames of the
  // start sequence on my machine. God knows why, but it won't cause any harm.
  static int frameCount = 0;
  if (frameCount < 10)
  {
    frameCount++;
    return;
  }

  RenderWorldShape();
  RenderIslands();
  RenderTrunkLinks();

  if (!g_context->m_editing)
  {
    RenderSpirits();
    RenderHeaven();
  }
}

void SphereWorld::RenderSpirits()
{
  START_PROFILE(g_context->m_profiler, "Spirits");

  //
  // Advance all spirits

  for (int locationId = 0; locationId < m_numLocations; ++locationId)
  {
    GlobalLocation* location = g_context->m_globalWorld->GetLocation(locationId);
    if (location)
    {
      bool isReceiver = (location->m_name == "receiver");

      if (isReceiver && frand(30) < 1.0f)
        m_spirits[locationId].PutDataAtStart(0.0f);
      else if (!isReceiver && frand(300) < 1.0f)
        m_spirits[locationId].PutDataAtStart(1.0f);

      if (location->m_numSpirits > 0 && frand(20) < 1.0f)
      {
        m_spirits[locationId].PutDataAtStart(1.0f);
        location->m_numSpirits--;
      }

      for (int i = 0; i < m_spirits[locationId].Size(); ++i)
      {
        float* thisSpirit = m_spirits[locationId].GetPointer(i);

        if (isReceiver)
          *thisSpirit += g_advanceTime * 0.02f;
        else
          *thisSpirit -= g_advanceTime * 0.02f;

        if (*thisSpirit >= 1.0f || *thisSpirit <= 0.0f)
        {
          m_spirits[locationId].RemoveData(i);
          --i;
        }
      }
    }
  }

  //
  // Render all spirits

  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glDepthMask(false);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/glow.bmp"));

  LegacyVector3 camRight = g_context->m_camera->GetRight();
  LegacyVector3 camUp = g_context->m_camera->GetUp();

  for (int locationId = 0; locationId < m_numLocations; ++locationId)
  {
    GlobalLocation* location = g_context->m_globalWorld->GetLocation(locationId);
    if (location)
    {
      for (int i = 0; i < m_spirits[locationId].Size(); ++i)
      {
        float* thisSpirit = m_spirits[locationId].GetPointer(i);

        LegacyVector3 fromPos = g_context->m_globalWorld->GetLocationPosition(locationId);

        float alphaValue = *thisSpirit * 3.0f;
        if (alphaValue > 1.0f)
          alphaValue = 1.0f;

        LegacyVector3 position = fromPos * (*thisSpirit);
        float timeOffset = g_gameTime / 2.0f;
        float posOffset = 1000;

        position.x += sinf(*thisSpirit * 14 + timeOffset) * posOffset;
        position.y += sinf(*thisSpirit * 15 + timeOffset) * posOffset;
        position.z += sinf(*thisSpirit * 16 + timeOffset) * posOffset;

        float scale = 0.4f;

        glColor4f(0.6f, 0.2f, 0.1f, alphaValue);
        glBegin(GL_QUADS);
        glTexCoord2f(0.5f, 0.5f);
        glVertex3fv((position + camUp * 300 * scale).GetData());
        glTexCoord2f(0.5f, 0.5f);
        glVertex3fv((position + camRight * 300 * scale).GetData());
        glTexCoord2f(0.5f, 0.5f);
        glVertex3fv((position - camUp * 300 * scale).GetData());
        glTexCoord2f(0.5f, 0.5f);
        glVertex3fv((position - camRight * 300 * scale).GetData());
        glEnd();

        glColor4f(0.6f, 0.2f, 0.1f, alphaValue);
        glBegin(GL_QUADS);
        glTexCoord2f(0.5f, 0.5f);
        glVertex3fv((position + camUp * 100 * scale).GetData());
        glTexCoord2f(0.5f, 0.5f);
        glVertex3fv((position + camRight * 100 * scale).GetData());
        glTexCoord2f(0.5f, 0.5f);
        glVertex3fv((position - camUp * 100 * scale).GetData());
        glTexCoord2f(0.5f, 0.5f);
        glVertex3fv((position - camRight * 100 * scale).GetData());
        glEnd();

        glColor4f(0.6f, 0.2f, 0.1f, alphaValue / 4.0f);
        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3fv((position + camUp * 6000 * scale).GetData());
        glTexCoord2i(1, 0);
        glVertex3fv((position + camRight * 6000 * scale).GetData());
        glTexCoord2i(1, 1);
        glVertex3fv((position - camUp * 6000 * scale).GetData());
        glTexCoord2i(0, 1);
        glVertex3fv((position - camRight * 6000 * scale).GetData());
        glEnd();
      }
    }
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glDisable(GL_TEXTURE_2D);

  glDepthMask(true);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
  glEnable(GL_CULL_FACE);

  END_PROFILE(g_context->m_profiler, "Spirits");
}

void SphereWorld::RenderWorldShape()
{
  START_PROFILE(g_context->m_profiler, "Shape");

  g_context->m_globalWorld->SetupLights();

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  float spec = 0.5f;
  float diffuse = 1.0f;
  float amb = 0.0f;
  GLfloat materialShininess[] = {10.0f};
  GLfloat materialSpecular[] = {spec, spec, spec, 1.0f};
  GLfloat materialDiffuse[] = {diffuse, diffuse, diffuse, 1.0f};
  GLfloat ambCol[] = {amb, amb, amb, 1.0f};

  glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
  glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);
  glMaterialfv(GL_FRONT, GL_AMBIENT, ambCol);

  auto& mv = OpenGLD3D::GetModelViewStack();
  mv.Push();
  mv.Scale(120.0f, 120.0f, 120.0f);
  glEnable(GL_NORMALIZE);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glDisable(GL_CULL_FACE);

  //
  // Render outer

  m_shapeOuter->Render(0.0f, g_identityMatrix34);
  m_shapeMiddle->Render(0.0f, g_identityMatrix34);
  m_shapeInner->Render(0.0f, g_identityMatrix34);

  glDisable(GL_NORMALIZE);
  mv.Pop();

  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHT1);

  END_PROFILE(g_context->m_profiler, "Shape");
}

void SphereWorld::RenderTrunkLinks()
{
  //if( g_context->m_editing ) return;

  Matrix34 rootMat(0);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glDepthMask(false);

  glBegin(GL_QUADS);

  for (int i = 0; i < g_context->m_globalWorld->m_buildings.Size(); ++i)
  {
    GlobalBuilding* building = g_context->m_globalWorld->m_buildings[i];
    if (building->m_type == Building::TypeTrunkPort && building->m_link != -1)
    {
      GlobalLocation* fromLoc = g_context->m_globalWorld->GetLocation(building->m_locationId);
      GlobalLocation* toLoc = g_context->m_globalWorld->GetLocation(building->m_link);

      if (fromLoc && toLoc && (fromLoc->m_available && toLoc->m_available) || g_context->m_editing)
      {
        LegacyVector3 fromPos = g_context->m_globalWorld->GetLocationPosition(building->m_locationId);
        LegacyVector3 toPos = g_context->m_globalWorld->GetLocationPosition(building->m_link);

        if (building->m_online)
          glColor4f(0.4f, 0.3f, 1.0f, 1.0f);
        else
          glColor4f(0.4f, 0.3f, 1.0f, 0.4f);

        //fromPos *= 120.0f;
        //toPos *= 120.0f;

        LegacyVector3 midPoint = fromPos + (toPos - fromPos) / 2.0f;
        LegacyVector3 camToMidPoint = g_context->m_camera->GetPos() - midPoint;
        LegacyVector3 rightAngle = (camToMidPoint ^ (midPoint - toPos)).Normalise();

        rightAngle *= 200.0f;

        glVertex3fv((fromPos - rightAngle).GetData());
        glVertex3fv((fromPos + rightAngle).GetData());
        glVertex3fv((toPos + rightAngle).GetData());
        glVertex3fv((toPos - rightAngle).GetData());
      }
    }
  }

  glEnd();
  glDepthMask(true);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
}

void SphereWorld::RenderHeaven()
{
  START_PROFILE(g_context->m_profiler, "Heaven");

  g_context->m_globalWorld->SetupLights();

  //
  // Render the central repository of spirits

  auto& mv = OpenGLD3D::GetModelViewStack();
  mv.Push();
  mv.Scale(120.0f, 120.0f, 120.0f);

  LegacyVector3 camUp = g_context->m_camera->GetUp();
  LegacyVector3 camRight = g_context->m_camera->GetRight();

  glDepthMask(false);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/glow.bmp"));

  for (int i = 0; i < 50; ++i)
  {
    LegacyVector3 pos(sinf(i / g_gameTime + i) * 20, sinf(g_gameTime + i) * i, cosf(i / g_gameTime + i) * 20);

    float size = i;

    glColor4f(0.6f, 0.2f, 0.1f, 0.9f);

    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv((pos - camRight * size + camUp * size).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((pos + camRight * size + camUp * size).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((pos + camRight * size - camUp * size).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((pos - camRight * size - camUp * size).GetData());
    glEnd();
  }

  mv.Pop();

  //
  // Render god rays going down

  /*
      glBindTexture   ( GL_TEXTURE_2D, Resource::GetTexture( "textures/godray.bmp" ) );
  
    for (int i = 0; i < g_context->m_globalWorld->m_locations.Size(); ++i)
    {
      GlobalLocation *loc = g_context->m_globalWorld->m_locations.GetData(i);
          LegacyVector3 islandPos = g_context->m_globalWorld->GetLocationPosition(loc->m_id );
          LegacyVector3 centerPos = g_zeroVector;
  
          for( int j = 0; j < 6; ++j )
          {
              LegacyVector3 godRayPos = islandPos;
  
              godRayPos.x += sinf( g_gameTime + i + j/2 ) * 1000;
              godRayPos.y += sinf( g_gameTime + i + j/2 ) * 1000;
              godRayPos.z += cosf( g_gameTime + i + j/2 ) * 1000;
  
              LegacyVector3 camToCenter = g_context->m_camera->GetPos() - centerPos;
              LegacyVector3 lineToCenter = camToCenter ^ ( centerPos - godRayPos );
              lineToCenter.Normalise();
  
              glColor4f( 0.6f, 0.2f, 0.1f, 0.8f);
  
              glBegin( GL_QUADS );
                  glTexCoord2f(0.75f,0);      glVertex3fv( (centerPos - lineToCenter * 1000).GetData() );
                  glTexCoord2f(0.75f,1);      glVertex3fv( (centerPos + lineToCenter * 1000).GetData() );
                  glTexCoord2f(0.05f,1);      glVertex3fv( (godRayPos + lineToCenter * 1000).GetData() );
                  glTexCoord2f(0.05f,0);      glVertex3fv( (godRayPos - lineToCenter * 1000).GetData() );
              glEnd();
          }
      }
  
  */

  glDisable(GL_TEXTURE_2D);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
  glDepthMask(true);

  END_PROFILE(g_context->m_profiler, "Heaven");
}

void SphereWorld::RenderIslands()
{
  if (g_context->m_camera->IsInMode(Camera::ModeSphereWorldIntro) || g_context->m_camera->IsInMode(Camera::ModeSphereWorldOutro))
    return;

  //
  // Render the islands

  START_PROFILE(g_context->m_profiler, "Islands");

  LegacyVector3 rayStart, rayDir;
  g_context->m_camera->GetClickRay(g_target->X(), g_target->Y(), &rayStart, &rayDir);

  LegacyVector3 camRight = g_context->m_camera->GetRight();
  LegacyVector3 camUp = g_context->m_camera->GetUp();

  //    glColor4f       ( 1.0f, 1.0f, 1.0f, 1.0f );
  glColor4f(0.6f, 0.2f, 0.1f, 1.0f);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(false);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));

  for (int i = 0; i < g_context->m_globalWorld->m_locations.Size(); ++i)
  {
    GlobalLocation* loc = g_context->m_globalWorld->m_locations.GetData(i);
    if (loc->m_available || g_context->m_editing)
    {
      LegacyVector3 islandPos = g_context->m_globalWorld->GetLocationPosition(loc->m_id);

      int numRedraws = 5;
      if (!loc->m_missionCompleted && _stricmp(loc->m_missionFilename.c_str(), "null") != 0 && fmodf(g_gameTime, 1.0f) < 0.7f)
        numRedraws = 10;

      glBegin(GL_QUADS);
      for (int j = 0; j <= numRedraws; ++j)
      {
        glTexCoord2i(0, 0);
        glVertex3fv((islandPos + camUp * 1000 * j).GetData());
        glTexCoord2i(1, 0);
        glVertex3fv((islandPos + camRight * 1000 * j).GetData());
        glTexCoord2i(1, 1);
        glVertex3fv((islandPos - camUp * 1000 * j).GetData());
        glTexCoord2i(0, 1);
        glVertex3fv((islandPos - camRight * 1000 * j).GetData());
      }
      glEnd();
    }
  }

  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(true);
  glEnable(GL_DEPTH_TEST);

  //
  // Render the islands names

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  for (int i = 0; i < g_context->m_globalWorld->m_locations.Size(); ++i)
  {
    GlobalLocation* loc = g_context->m_globalWorld->m_locations.GetData(i);
    if (loc->m_available || g_context->m_editing)
    {
      LegacyVector3 islandPos = g_context->m_globalWorld->GetLocationPosition(loc->m_id);
      char* islandName = _strdup(g_context->m_globalWorld->GetLocationNameTranslated(loc->m_id));
      _strupr(islandName);

      float size = 5.0f * sqrtf((g_context->m_camera->GetPos() - islandPos).Mag());
      size = 1000.0f;

      g_gameFont.SetRenderShadow(true);
      glColor4f(0.7f, 0.7f, 0.7f, 0.0f);
      g_gameFont.DrawText3DCenter(islandPos + camUp * size * 1.5f, size * 3.0f, islandName);

      if (g_context->m_editing)
      {
        g_gameFont.DrawText3DCenter(islandPos, size, loc->m_mapFilename.c_str());
        g_gameFont.DrawText3DCenter(islandPos - camUp * size, size, loc->m_missionFilename.c_str());
      }

      islandPos += camUp * size * 0.3f;
      islandPos += camRight * size * 0.1f;

      g_gameFont.SetRenderShadow(false);
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
      if (_stricmp(loc->m_missionFilename.c_str(), "null") == 0)
        glColor4f(0.5f, 0.5f, 0.5f, 1.0f);

      g_gameFont.DrawText3DCenter(islandPos + camUp * size * 1.5f, size * 3.0f, islandName);

      if (g_context->m_editing)
      {
        g_gameFont.DrawText3DCenter(islandPos, size, loc->m_mapFilename.c_str());
        g_gameFont.DrawText3DCenter(islandPos - camUp * size, size, loc->m_missionFilename.c_str());
      }

      free(islandName);
    }
  }

  END_PROFILE(g_context->m_profiler, "Islands");
}

// ****************************************************************************
// Class SphereWorld
// ****************************************************************************

GlobalWorld::GlobalWorld()
  : m_myTeamId(255),
    m_editorMode(0),
    m_editorSelectionId(-1),
    m_nextLocationId(0),
    m_nextBuildingId(0),
    m_locationRequested(-1)
{
  m_globalInternet = new GlobalInternet();
  m_sphereWorld = new SphereWorld();
  m_research = new GlobalResearch();
}

GlobalWorld::~GlobalWorld()
{
  m_locations.EmptyAndDelete();
  m_buildings.EmptyAndDelete();
  m_events.EmptyAndDelete();

  delete m_globalInternet;
  delete m_sphereWorld;
  delete m_research;
}

void GlobalWorld::Advance()
{
  if (g_context->m_editing)
  {
    if (m_editorMode == 0)
    {
      // Edit locations
      if (g_inputManager->controlEvent(ControlSelectLocation))
      {
        LegacyVector3 rayStart, rayDir;
        g_context->m_camera->GetClickRay(g_target->X(), g_target->Y(), &rayStart, &rayDir);
        int locId = LocationHit(rayStart, rayDir);
        if (locId != -1)
        {
          GlobalLocation* loc = GetLocation(locId);
          g_context->m_requestedLocationId = locId;
          strncpy(g_context->m_requestedMission, loc->m_missionFilename.c_str(), sizeof(g_context->m_requestedMission));
          g_context->m_requestedMission[sizeof(g_context->m_requestedMission) - 1] = '\0';
          strncpy(g_context->m_requestedMap, loc->m_mapFilename.c_str(), sizeof(g_context->m_requestedMap));
          g_context->m_requestedMap[sizeof(g_context->m_requestedMap) - 1] = '\0';
        }
      }
    }
    else
    {
      // Move locations
      if (g_inputManager->controlEvent(ControlSelectLocation))
      {
        LegacyVector3 rayStart, rayDir;
        g_context->m_camera->GetClickRay(g_target->X(), g_target->Y(), &rayStart, &rayDir);
        m_editorSelectionId = LocationHit(rayStart, rayDir);
      }
      else if (g_inputManager->controlEvent(ControlLocationDragActive))
      {
        GlobalLocation* loc = GetLocation(m_editorSelectionId);
        if (loc)
        {
          LegacyVector3 mousePos3D = g_context->m_userInput->GetMousePos3d();
          loc->m_pos = mousePos3D / 120.0f;
        }
      }
      else if (g_inputManager->controlEvent(ControlDeselectLocation))
        m_editorSelectionId = -1;
    }
  }
  else
  {
    // Has the user clicked on a location?
    if (g_inputManager->controlEvent(ControlSelectLocation) && m_locationRequested == -1 && EclGetWindows()->Size() == 0)
    {
      LegacyVector3 rayStart, rayDir;
      g_context->m_camera->GetClickRay(g_target->X(), g_target->Y(), &rayStart, &rayDir);
      int locId = LocationHit(rayStart, rayDir);
      if (locId != -1)
      {
        GlobalLocation* loc = GetLocation(locId);
        if (loc->m_missionFilename != "null" && loc->m_available)
        {
          // Default behaviour is to go the location
          m_locationRequested = locId;
          g_context->m_renderer->StartFadeOut();
        }
      }
    }
    // Is the cursor attracted to a point?
    else if (m_locationRequested == -1 && EclGetWindows()->Size() == 0)
    {
      LegacyVector3 rayStart, rayDir;
      g_context->m_camera->GetClickRay(g_target->X(), g_target->Y(), &rayStart, &rayDir);
      int locId = LocationHit(rayStart, rayDir, 10000.0f);
      if (locId != -1)
      {
        // We're close to a location, but not there, so drag the pointer towards it
        GlobalLocation* loc = GetLocation(locId);
        float locX, locY;
        g_context->m_camera->Get2DScreenPos(loc->m_pos, &locX, &locY);
        locY = g_context->m_renderer->ScreenH() - locY;
        int movX = static_cast<int>(locX - g_target->X());
        int movY = static_cast<int>(locY - g_target->Y());
        int movMag2 = movX * movX + movY * movY;
        int movFactor = 30 / movMag2;
        if (movFactor > 0)
          g_target->MoveCursor(movX * movFactor, movY * movFactor);
      }
    }

    // Has the fade out finished?
    if (m_locationRequested != -1 && g_context->m_renderer->IsFadeComplete())
    {
      GlobalLocation* loc = GetLocation(m_locationRequested);
      g_context->m_requestedLocationId = m_locationRequested;
      strncpy(g_context->m_requestedMission, loc->m_missionFilename.c_str(), sizeof(g_context->m_requestedMission));
      g_context->m_requestedMission[sizeof(g_context->m_requestedMission) - 1] = '\0';
      strncpy(g_context->m_requestedMap, loc->m_mapFilename.c_str(), sizeof(g_context->m_requestedMap));
      g_context->m_requestedMap[sizeof(g_context->m_requestedMap) - 1] = '\0';

      m_locationRequested = -1;
    }
  }
}

void GlobalWorld::Render()
{
  START_PROFILE(g_context->m_profiler, "Render Global World");

  if (!g_context->m_editing)
    m_globalInternet->Render();
  CHECK_OPENGL_STATE();
  m_sphereWorld->Render();
  CHECK_OPENGL_STATE();

  END_PROFILE(g_context->m_profiler, "Render Global World");
}

// Returns the ID of the location the line intersects. Returns -1 if line
// does not intersect any location
int GlobalWorld::LocationHit(const LegacyVector3& _pos, const LegacyVector3& _dir, float locationRadius)
{
  //float locationRadius = 5000.0f;

  for (int i = 0; i < m_locations.Size(); ++i)
  {
    GlobalLocation* gl = m_locations[i];
    LegacyVector3 locPos = GetLocationPosition(gl->m_id);

    bool hit = RaySphereIntersection(_pos, _dir, locPos, locationRadius);
    if (hit)
      return gl->m_id;
  }

  return -1;
}

GlobalLocation* GlobalWorld::GetLocation(int _id)
{
  for (int i = 0; i < m_locations.Size(); ++i)
  {
    GlobalLocation* loc = m_locations[i];
    if (loc->m_id == _id)
      return loc;
  }

  return nullptr;
}

GlobalLocation* GlobalWorld::GetHighlightedLocation()
{
  int screenX = g_target->X();
  int screenY = g_target->Y();

  LegacyVector3 rayStart, rayDir;
  g_context->m_camera->GetClickRay(screenX, screenY, &rayStart, &rayDir);
  int locId = g_context->m_globalWorld->LocationHit(rayStart, rayDir);

  GlobalLocation* loc = GetLocation(locId);

  if (loc && loc->m_available)
    return loc;
  if (loc && g_context->m_editing)
    return loc;

  return nullptr;
}

GlobalLocation* GlobalWorld::GetLocation(const char* _name)
{
  int locationId = GetLocationId(_name);
  if (locationId != -1)
    return GetLocation(locationId);
  return nullptr;
}

int GlobalWorld::GetLocationId(const char* _name)
{
  for (int i = 0; i < m_locations.Size(); ++i)
  {
    GlobalLocation* loc = m_locations[i];
    DEBUG_ASSERT(loc);
    if (_stricmp(loc->m_name.c_str(), _name) == 0)
      return loc->m_id;
  }

  return -1;
}

int GlobalWorld::GetLocationIdFromMapFilename(const char* _mapFilename)
{
  std::string buf(_mapFilename);
  auto pos = buf.find("map_");
  if (pos == std::string::npos)
    return -1;
  std::string mapName = buf.substr(pos + 4);
  // Strip the .xxx extension
  if (mapName.size() >= 4)
    mapName.resize(mapName.size() - 4);

  return GetLocationId(mapName.c_str());
}

const char* GlobalWorld::GetLocationName(int _id)
{
  GlobalLocation* loc = GetLocation(_id);
  if (loc)
    return loc->m_name.c_str();
  return nullptr;
}

const char* GlobalWorld::GetLocationNameTranslated(int _id)
{
  GlobalLocation* loc = GetLocation(_id);
  if (!loc)
    return nullptr;

  auto stringId = std::format("location_{}", loc->m_name);

  if (ISLANGUAGEPHRASE(stringId.c_str()))
    return LANGUAGEPHRASE(stringId.c_str());
  return loc->m_name.c_str();
}

LegacyVector3 GlobalWorld::GetLocationPosition(int _id)
{
  GlobalLocation* location = GetLocation(_id);
  if (!location)
    return g_zeroVector;
  return location->m_pos * 120.0f;
}

GlobalBuilding* GlobalWorld::GetBuilding(int _id, int _locationId)
{
  if (_id == -1 || _locationId == -1)
    return nullptr;

  for (int i = 0; i < m_buildings.Size(); ++i)
  {
    GlobalBuilding* buil = m_buildings[i];
    if (buil->m_id == _id && buil->m_locationId == _locationId)
      return buil;
  }

  return nullptr;
}

void GlobalWorld::AddLocation(GlobalLocation* location)
{
  if (location->m_id == -1)
  {
    location->m_id = m_nextLocationId;
    m_nextLocationId++;
  }
  else if (location->m_id >= m_nextLocationId)
    m_nextLocationId = location->m_id + 1;

  m_locations.PutData(location);
  m_sphereWorld->AddLocation(location->m_id);
}

void GlobalWorld::AddBuilding(GlobalBuilding* building)
{
  if (building->m_id == -1)
  {
    building->m_id = m_nextBuildingId;
    m_nextBuildingId++;
  }
  else if (building->m_id >= m_nextBuildingId)
    m_nextBuildingId = building->m_id + 1;

  m_buildings.PutData(building);
}

void GlobalWorld::ParseLocations(TextReader* _in)
{
  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;

    char* word = _in->GetNextToken();

    if (_stricmp(word, "Locations_EndDefinition") == 0)
      return;

    auto location = new GlobalLocation();

    location->m_id = atoi(word);
    location->m_available = static_cast<bool>(atoi(_in->GetNextToken()));

    location->m_mapFilename = _in->GetNextToken();
    location->m_missionFilename = _in->GetNextToken();

    // Extract name from map filename: strip "map_" prefix and ".xxx" extension
    location->m_name = location->m_mapFilename.substr(4);
    if (location->m_name.size() >= 4)
      location->m_name.resize(location->m_name.size() - 4);

    AddLocation(location);
  }
}

void GlobalWorld::ParseBuildings(TextReader* _in)
{
  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;

    char* word = _in->GetNextToken();

    if (_stricmp(word, "buildings_enddefinition") == 0)
      return;

    auto building = new GlobalBuilding();

    building->m_id = atoi(word);
    building->m_teamId = atoi(_in->GetNextToken());
    building->m_locationId = atoi(_in->GetNextToken());
    building->m_type = atoi(_in->GetNextToken());
    building->m_link = atoi(_in->GetNextToken());
    building->m_online = atoi(_in->GetNextToken());

    AddBuilding(building);
  }
}

void GlobalWorld::ParseEvents(TextReader* _in)
{
  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;
    char* word = _in->GetNextToken();

    if (_stricmp(word, "events_enddefinition") == 0)
      return;

    DEBUG_ASSERT(_stricmp( word, "Event" ) == 0);

    auto event = new GlobalEvent();
    event->Read(_in);
    m_events.PutData(event);
  }
}

void GlobalWorld::AddLevelBuildingToGlobalBuildings(Building* _building, int _locId)
{
  if (_building->m_isGlobal)
  {
    GlobalBuilding* gb = GetBuilding(_building->m_id.GetUniqueId(), _locId);
    if (!gb)
    {
      gb = new GlobalBuilding();
      gb->m_type = _building->m_type;
      gb->m_locationId = _locId;
      gb->m_id = _building->m_id.GetUniqueId();
      gb->m_teamId = _building->m_id.GetTeamId();
      m_buildings.PutData(gb);

      if (_building->m_type == Building::TypeTrunkPort)
        gb->m_link = static_cast<TrunkPort*>(_building)->m_targetLocationId;
    }
    gb->m_pos = _building->m_pos;
  }
}

void GlobalWorld::LoadGame(const char* _filename)
{
  TextReader* in = nullptr;

  if (!g_context->m_editing)
  {
    auto fullFilename = std::format("{}users/{}/{}", g_context->GetProfileDirectory(), g_context->m_userProfileName, _filename);
    if (DoesFileExist(fullFilename.c_str()))
      in = new TextFileReader(fullFilename.c_str());
  }

  if (!in)
    in = Resource::GetTextReader(_filename);

  if (in)
  {
    while (in->ReadLine())
    {
      if (!in->TokenAvailable())
        continue;
      char* word = in->GetNextToken();

      if (_stricmp("locations_startdefinition", word) == 0)
        ParseLocations(in);
      else if (_stricmp("buildings_startdefinition", word) == 0)
        ParseBuildings(in);
      else if (_stricmp("events_startdefinition", word) == 0)
        ParseEvents(in);
      else if (_stricmp("research_startdefinition", word) == 0)
        m_research->Read(in);
    }
  }

  delete in;
  in = nullptr;

  //
  // Load locations

  LoadLocations("locations.txt");

  //
  // Load all map files into memory

  for (int i = 0; i < m_locations.Size(); i++)
  {
    if (!m_locations.ValidIndex(i))
      continue;

    // Get the next location
    GlobalLocation* loc = m_locations.GetData(i);

    // Load all the level files for the location
    LevelFile levFile("null", loc->m_mapFilename.c_str());
    for (int b = 0; b < levFile.m_buildings.Size(); ++b)
    {
      Building* building = levFile.m_buildings[b];
      AddLevelBuildingToGlobalBuildings(building, loc->m_id);
    }

    auto filter = std::format("mission_{}*.txt", GetLocationName(loc->m_id));

    for (auto missionName : Resource::ListResources("levels\\", filter.c_str(), false))
    {
        LevelFile missionLevFile(missionName.c_str(), loc->m_mapFilename.c_str());

      for (int b = 0; b < missionLevFile.m_buildings.Size(); ++b)
      {
        Building* building = missionLevFile.m_buildings[b];
        AddLevelBuildingToGlobalBuildings(building, loc->m_id);

        if (building->m_type == Building::TypeAntHill || building->m_type == Building::TypeTriffid || building->m_type ==
          Building::TypeIncubator)
        {
          if (!building->m_dynamic)
          {
            DebugTrace("{} found on level {} should be dynamic (otherwise save games wont work)\n", Building::GetTypeName(building->m_type),
                       GetLocationName(loc->m_id));
          }
        }
      }

      bool objectivesComplete = true;
      for (int j = 0; j < missionLevFile.m_primaryObjectives.Size(); ++j)
      {
        GlobalEventCondition* gec = missionLevFile.m_primaryObjectives[j];
        if (!gec->Evaluate())
        {
          objectivesComplete = false;
          break;
        }
      }

      if (objectivesComplete)
        loc->m_missionCompleted = true;
    }
  }

  EvaluateEvents();
}

void GlobalWorld::LoadLocations(const char* _filename)
{
  TextReader* in = Resource::GetTextReader(_filename);

  while (in->ReadLine())
  {
    if (!in->TokenAvailable())
      continue;

    int locIndex = atoi(in->GetNextToken());
    float posX = atof(in->GetNextToken());
    float posY = atof(in->GetNextToken());
    float posZ = atof(in->GetNextToken());

    GlobalLocation* location = GetLocation(locIndex);
    if (location)
      location->m_pos.Set(posX, posY, posZ);
  }

  delete in;
}

// Find the lowest unused building ID in the current location
int GlobalWorld::GenerateBuildingId()
{
  int id = 0;
  while (true)
  {
    if (!g_context->m_location->GetBuilding(id))
      break;
    ++id;
  }
  return id;
}

// Checks to see if any event's conditions are true. If they are, the first action
// for that event will be executed. That event action is then deleted, and if there
// are no more actions for the event, then the event is deleted too.
// Returns true if actions remain to be completed
bool GlobalWorld::EvaluateEvents()
{
  if (g_context->m_script && g_context->m_script->IsRunningScript())
    return true;

  for (int i = 0; i < m_events.Size(); ++i)
  {
    GlobalEvent* event = m_events[i];

    if (event->Evaluate())
    {
      event->MakeAlwaysTrue();
      bool amIDone = event->Execute();
      if (amIDone)
      {
        m_events.RemoveData(i);
        delete event;
        --i;
      }
      return true;
    }
  }

  return false;
}

void GlobalWorld::TransferSpirits(int _locationId)
{
  //
  // Count how many spirits remain on the location

  DEBUG_ASSERT(g_context->m_location);
  int remainingSpirits = g_context->m_location->m_spirits.NumUsed();

  GlobalLocation* location = GetLocation(_locationId);
  ASSERT_TEXT(location, "GlobalWorld::TransferSpirits, failed to lookup location %d", _locationId);

  int spiritCount = location->m_numSpirits + remainingSpirits / 2;
  location->m_numSpirits = remainingSpirits / 2;

  float position = 1.0f;
  for (int i = 0; i < spiritCount; ++i)
  {
    m_sphereWorld->m_spirits[_locationId].PutDataAtStart(position);
    position -= frand(0.04f);
  }
}

void GlobalWorld::SetupLights()
{
  float black[] = {0, 0, 0, 0};
  float colour1[] = {2.0f, 1.5f, 0.75f, 1.0f};

  LegacyVector3 light0(0, 1, 0);
  light0.Normalise();
  GLfloat light0AsFourFloats[] = {light0.x, light0.y, light0.z, 0.0f};

  glLightfv(GL_LIGHT0, GL_POSITION, light0AsFourFloats);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, colour1);
  glLightfv(GL_LIGHT0, GL_SPECULAR, colour1);
  glLightfv(GL_LIGHT0, GL_AMBIENT, black);

  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHT1);
  glDisable(GL_LIGHTING);
}

void GlobalWorld::SetupFog()
{
  float fog = 0.1f;
  float fogCol[] = {fog, fog, fog, fog};

  glFogf(GL_FOG_DENSITY, 1.0f);
  glFogf(GL_FOG_START, 0.0f);
  glFogf(GL_FOG_END, 19000.0f);
  glFogfv(GL_FOG_COLOR, fogCol);
  glFogi(GL_FOG_MODE, GL_LINEAR);
  //glEnable    (GL_FOG);
}

float GlobalWorld::GetSize()
{
  if (g_context->m_location)
    return g_context->m_location->m_landscape.GetWorldSizeX();

  return 2e5;
}

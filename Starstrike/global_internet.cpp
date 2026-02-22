#include "pch.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"
#include "hi_res_time.h"
#include "math_utils.h"
#include "resource.h"
#include "profiler.h"
#include "debug_utils.h"
#include "app.h"
#include "camera.h"
#include "global_internet.h"
#include "global_world.h"
#include "main.h"

#define DISPLAY_LIST_NAME_LINKS "GlobalInternetLinks"
#define DISPLAY_LIST_NAME_NODES "GlobalInternetNodes"

//*****************************************************************************
// Class GlobalInternetNode
//*****************************************************************************

GlobalInternetNode::GlobalInternetNode()
  : m_size(0),
    m_burst(0),
    m_numLinks(0) {}

void GlobalInternetNode::AddLink(int _id)
{
  DarwiniaDebugAssert(m_numLinks < GLOBALINTERNET_MAXNODELINKS);
  m_links[m_numLinks] = _id;
  m_numLinks++;
}

// ****************************************************************************
// Class GlobalInternet
// ****************************************************************************

GlobalInternet::GlobalInternet()
  : m_nodes(nullptr),
    m_numNodes(0),
    m_links(nullptr),
    m_numLinks(0),
    m_nearestNodeToCentre(-1),
    m_nearestDistance(FLT_MAX)
{
  darwiniaSeedRandom(1);
  GenerateInternet();
}

GlobalInternet::~GlobalInternet() { DeleteInternet(); }

unsigned short GlobalInternet::GenerateInternet(const LegacyVector3& _pos, unsigned char _size)
{
  GetHighResTime();

  GlobalInternetNode* node = &m_nodes[m_numNodes];
  node->m_pos = _pos;
  node->m_size = _size;
  unsigned short nodeIndex = m_numNodes;
  m_numNodes++;
  DarwiniaDebugAssert(m_numNodes < GLOBALINTERNET_MAXNODES);

  float distanceToCentre = _pos.Mag();
  if (distanceToCentre < m_nearestDistance)
  {
    m_nearestDistance = distanceToCentre;
    m_nearestNodeToCentre = nodeIndex;
  }

  unsigned char numLinks = _size;
  float distance = powf(_size, 4.0f) * 2.0f;

  while (numLinks > 0)
  {
    float z = sfrand(distance);
    float y = sfrand(distance);
    float x = sfrand(distance);
    LegacyVector3 newPos = _pos + LegacyVector3(x, y, z);
    unsigned short newIndex = GenerateInternet(newPos, _size - 1);
    m_links[m_numLinks].m_from = nodeIndex;
    m_links[m_numLinks].m_to = newIndex;
    m_links[m_numLinks].m_size = _size;

    m_nodes[newIndex].AddLink(m_numLinks);
    node->AddLink(m_numLinks);

    m_numLinks++;
    DarwiniaDebugAssert(m_numLinks <= GLOBALINTERNET_MAXLINKS);

    --numLinks;
  }

  return nodeIndex;
}

void GlobalInternet::GenerateInternet()
{
  m_links = new GlobalInternetLink[GLOBALINTERNET_MAXLINKS];
  m_nodes = new GlobalInternetNode[GLOBALINTERNET_MAXNODES];

  LegacyVector3 centre(-797, 1949, -1135);
  unsigned short firstNode = GenerateInternet(centre, GLOBALINTERNET_ITERATIONS);

  m_nodes[m_numNodes].m_pos.Zero();
  m_nodes[m_numNodes].m_size = 0.0f;
  unsigned short nodeIndex = m_numNodes;
  m_numNodes++;
  DarwiniaDebugAssert(m_numNodes <= GLOBALINTERNET_MAXNODES);

  m_links[m_numLinks].m_from = m_nearestNodeToCentre;
  m_links[m_numLinks].m_to = nodeIndex;
  m_links[m_numLinks].m_size = 1.0f;
  m_numLinks++;
  DarwiniaDebugAssert(m_numLinks <= GLOBALINTERNET_MAXLINKS);

  for (int i = 0; i < m_numNodes; ++i)
  {
    GlobalInternetNode* node = &m_nodes[i];
    if (node->m_numLinks == 1)
      m_leafs.PutData(i);
  }
}

void GlobalInternet::DeleteInternet()
{
  delete [] m_nodes;
  m_numNodes = 0;
  delete [] m_links;
  m_numLinks = 0;
  m_leafs.Empty();
  m_bursts.Empty();

  g_app->m_resource->DeleteDisplayList(DISPLAY_LIST_NAME_LINKS);
  g_app->m_resource->DeleteDisplayList(DISPLAY_LIST_NAME_NODES);
}

void GlobalInternet::Render()
{
  START_PROFILE(g_app->m_profiler, "Internet");

  /*static*/
  float scale = 1000.0f;

  g_imRenderer->PushMatrix();
  g_imRenderer->Scalef(scale, scale, scale);

  float fog = 0.0f;
  float fogCol[] = {fog, fog, fog, fog};

  /*static*/
  int fogVal = 5810000;
  //    if( g_keys[KEY_P] )
  //    {
  //        fogVal += 100000;
  //    }
  //    if( g_keys[KEY_O] )
  //    {
  //        fogVal -= 100000;
  //    }

  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);

  //
  // Render Links

  g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/laserfence2.bmp"));
  g_imRenderer->Color4f(0.25f, 0.25f, 0.5f, 0.8f);

  g_imRenderer->Begin(PRIM_QUADS);

  for (int i = 0; i < m_numLinks; ++i)
  {
    GlobalInternetLink* link = &m_links[i];
    GlobalInternetNode* from = &m_nodes[link->m_from];
    GlobalInternetNode* to = &m_nodes[link->m_to];

    LegacyVector3 fromPos = from->m_pos;
    LegacyVector3 toPos = to->m_pos;
    LegacyVector3 midPoint = (toPos + fromPos) / 2.0f;
    LegacyVector3 camToMidPoint = g_zeroVector - midPoint;
    LegacyVector3 rightAngle = (camToMidPoint ^ (midPoint - toPos)).Normalise();

    rightAngle *= link->m_size * 0.5f;

    g_imRenderer->TexCoord2i(0, 0);
    g_imRenderer->Vertex3fv((fromPos - rightAngle).GetData());
    g_imRenderer->TexCoord2i(0, 1);
    g_imRenderer->Vertex3fv((fromPos + rightAngle).GetData());
    g_imRenderer->TexCoord2i(1, 1);
    g_imRenderer->Vertex3fv((toPos + rightAngle).GetData());
    g_imRenderer->TexCoord2i(1, 0);
    g_imRenderer->Vertex3fv((toPos - rightAngle).GetData());
  }
  g_imRenderer->End();

  //
  // Render Nodes

  g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/glow.bmp"));
  g_imRenderer->Color4f(0.8f, 0.8f, 1.0f, 0.6f);
  float nodeSize = 10.0f;

  g_imRenderer->Begin(PRIM_QUADS);
  for (int i = 0; i < m_numNodes; ++i)
  {
    GlobalInternetNode* node = &m_nodes[i];

    LegacyVector3 camToMidPoint = (g_zeroVector - node->m_pos).Normalise();
    LegacyVector3 right = camToMidPoint ^ LegacyVector3(0, 0, 1);
    LegacyVector3 up = right ^ camToMidPoint;
    right *= nodeSize;
    up *= nodeSize;
    g_imRenderer->TexCoord2i(0, 0);
    g_imRenderer->Vertex3fv((node->m_pos - up - right).GetData());
    g_imRenderer->TexCoord2i(1, 0);
    g_imRenderer->Vertex3fv((node->m_pos - up + right).GetData());
    g_imRenderer->TexCoord2i(1, 1);
    g_imRenderer->Vertex3fv((node->m_pos + up + right).GetData());
    g_imRenderer->TexCoord2i(0, 1);
    g_imRenderer->Vertex3fv((node->m_pos + up - right).GetData());
  }
  g_imRenderer->End();

  g_imRenderer->UnbindTexture();

  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_BACK);
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);

  RenderPackets();

  g_app->m_globalWorld->SetupFog();

  g_imRenderer->PopMatrix();

  END_PROFILE(g_app->m_profiler, "Internet");
}

void GlobalInternet::TriggerPacket(unsigned short _nodeId, unsigned short _fromLinkId)
{
  GlobalInternetNode* newNode = &m_nodes[_nodeId];

  if (newNode->m_numLinks == 1 && newNode->m_links[0] == _fromLinkId)
    return;

  while (true)
  {
    int newLinkIndex = darwiniaRandom() % newNode->m_numLinks;
    if (newNode->m_links[newLinkIndex] != _fromLinkId)
    {
      GlobalInternetLink* newLink = &m_links[newNode->m_links[newLinkIndex]];
      if (newLink->m_from == _nodeId)
        newLink->m_packets.PutData(1.0f);
      else
        newLink->m_packets.PutData(-1.0f);
      break;
    }
  }
}

void GlobalInternet::RenderPackets()
{
  //
  // Create new packets

  if (frand(100.0f) < 11.0f)
  {
    int leafIndex = frand(m_leafs.Size());
    GlobalInternetNode* node = &m_nodes[m_leafs[leafIndex]];
    node->m_burst = 4.0f;
    m_bursts.PutData(m_leafs[leafIndex]);
  }

  //
  // Deal with data bursts

  for (int i = 0; i < m_bursts.Size(); ++i)
  {
    GlobalInternetNode* node = &m_nodes[m_bursts[i]];
    node->m_burst -= g_advanceTime;
    if (node->m_burst > 0.0f)
      TriggerPacket(m_bursts[i], -1);
    else
    {
      m_bursts.RemoveData(i);
      --i;
    }
  }

  //
  // Advance / render all packets

  float packetSize = 30.0f;
  LegacyVector3 camRight = g_app->m_camera->GetRight() * packetSize;
  LegacyVector3 camUp = g_app->m_camera->GetUp() * packetSize;
  float posChange = g_advanceTime;

  g_imRenderer->Color4f(0.25f, 0.25f, 0.5f, 0.8f);

  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
  g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/starburst.bmp"));
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);

  for (int i = 0; i < m_numLinks; ++i)
  {
    GlobalInternetLink* link = &m_links[i];
    for (int j = 0; j < link->m_packets.Size(); ++j)
    {
      float* thisPacket = link->m_packets.GetPointer(j);
      float packetVal = *thisPacket;
      if (*thisPacket > 0.0f)
      {
        *thisPacket -= posChange;
        if (*thisPacket <= 0.01f)
        {
          *thisPacket = 0.01f;
          TriggerPacket(link->m_to, i);
          link->m_packets.RemoveData(j);
          --j;
        }
      }
      else if (*thisPacket < 0.0f)
      {
        *thisPacket += posChange;
        if (*thisPacket >= -0.01f)
        {
          *thisPacket = -0.01f;
          TriggerPacket(link->m_from, i);
          link->m_packets.RemoveData(j);
          --j;
        }
      }

      GlobalInternetNode* from = &m_nodes[link->m_from];
      GlobalInternetNode* to = &m_nodes[link->m_to];
      LegacyVector3 packetPos;
      if (packetVal > 0.0f)
        packetPos = to->m_pos + (from->m_pos - to->m_pos) * packetVal;
      else if (packetVal < 0.0f)
        packetPos = from->m_pos + (to->m_pos - from->m_pos) * -packetVal;

      g_imRenderer->Begin(PRIM_QUADS);
      g_imRenderer->TexCoord2i(0, 0);
      g_imRenderer->Vertex3fv((packetPos - camUp - camRight).GetData());
      g_imRenderer->TexCoord2i(1, 0);
      g_imRenderer->Vertex3fv((packetPos - camUp + camRight).GetData());
      g_imRenderer->TexCoord2i(1, 1);
      g_imRenderer->Vertex3fv((packetPos + camUp + camRight).GetData());
      g_imRenderer->TexCoord2i(0, 1);
      g_imRenderer->Vertex3fv((packetPos + camUp - camRight).GetData());
      g_imRenderer->End();
    }
  }

  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);
  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_BACK);
}

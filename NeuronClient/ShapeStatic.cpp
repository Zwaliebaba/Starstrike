#include "pch.h"
#include "math_utils.h"
#include "matrix34.h"
#include "ShapeStatic.h"
#include "text_stream_readers.h"
#include "resource.h"
#include "ShapeMeshCache.h"
#include "opengl_directx_internals.h"

// Per-thread scratch buffer for world-space positions in accurate hit-tests.
// Grows to the largest fragment encountered and stays allocated for reuse.
// Safe across recursion: each level fully consumes the buffer before recurring.
static thread_local std::vector<LegacyVector3> t_scratchWS;

// ****************************************************************************
// Class ShapeMarkerData
// ****************************************************************************

// *** Constructor
// This constructor is used in the export process. The m_parentIndices array is never
// populated in the exporter, so it is intentionally left blank
ShapeMarkerData::ShapeMarkerData(const char* _name, const char* _parentName, int _depth, const Matrix34& _transform)
  : m_transform(_transform),
    m_depth(_depth),
    m_parentIndices(nullptr)
{
  m_name = _strdup(_name);
  m_parentName = _strdup(_parentName);

  // Remove spaces from m_name
  char* c = m_name;
  char* d = m_name;
  while (d[0] != '\0')
  {
    c[0] = d[0];
    if (c[0] != ' ')
      c++;
    d++;
  }
  c[0] = '\0';

  // Remove spaces from m_parentName
  c = m_parentName;
  d = m_parentName;
  while (d[0] != '\0')
  {
    c[0] = d[0];
    if (c[0] != ' ')
      c++;
    d++;
  }
  c[0] = '\0';
}

// *** Constructor
ShapeMarkerData::ShapeMarkerData(TextReader* _in, const char* _name)
{
  m_name = _strdup(_name);
  m_parentName = nullptr;
  while (_in->ReadLine())
  {
    char* firstWord = _in->GetNextToken();

    if (firstWord)
    {
      if (_stricmp(firstWord, "ParentName") == 0)
      {
        char* secondWord = _in->GetNextToken();
        m_parentName = _strdup(secondWord);
      }
      else if (_stricmp(firstWord, "Depth") == 0)
      {
        char* secondWord = _in->GetNextToken();
        m_depth = atoi(secondWord);
      }
      else if (_stricmp(firstWord, "front") == 0)
      {
        char* secondWord = _in->GetNextToken();
        m_transform.f.x = static_cast<float>(atof(secondWord));
        m_transform.f.y = static_cast<float>(atof(_in->GetNextToken()));
        m_transform.f.z = static_cast<float>(atof(_in->GetNextToken()));
      }
      else if (_stricmp(firstWord, "up") == 0)
      {
        char* secondWord = _in->GetNextToken();
        m_transform.u.x = static_cast<float>(atof(secondWord));
        m_transform.u.y = static_cast<float>(atof(_in->GetNextToken()));
        m_transform.u.z = static_cast<float>(atof(_in->GetNextToken()));
      }
      else if (_stricmp(firstWord, "pos") == 0)
      {
        char* secondWord = _in->GetNextToken();
        m_transform.pos.x = static_cast<float>(atof(secondWord));
        m_transform.pos.y = static_cast<float>(atof(_in->GetNextToken()));
        m_transform.pos.z = static_cast<float>(atof(_in->GetNextToken()));
      }
      else if (_stricmp(firstWord, "MarkerEnd") == 0)
        break;
    }
  }

  m_transform.Normalise();

  m_parentIndices = new int[m_depth];

  if (!m_name)
    m_name = _strdup("unknown");
  if (!m_parentName)
    m_parentName = _strdup("unknown");
}

ShapeMarkerData::~ShapeMarkerData()
{
  SAFE_FREE(m_parentName);
  SAFE_FREE(m_name);
  SAFE_DELETE(m_parentIndices);
}

// *** GetWorldMatrix
Matrix34 ShapeMarkerData::GetWorldMatrix(const FragmentState* _states, const Matrix34& _rootTransform) const
{
  return Matrix34(GetWorldTransform(_states, static_cast<Transform3D>(_rootTransform)));
}

// *** GetWorldTransform
// SIMD-accelerated version using Transform3D composition (XMMatrixMultiply).
// Uses fragment indices into the states array instead of chasing raw pointers.
Transform3D ShapeMarkerData::GetWorldTransform(const FragmentState* _states, const Transform3D& _rootTransform) const
{
  using namespace DirectX;
  XMMATRIX result = XMLoadFloat4x4(&_rootTransform.m);
  for (int i = 0; i < m_depth; ++i)
  {
    const Transform3D& parentTransform = _states[m_parentIndices[i]].transform;
    XMMATRIX parent = XMLoadFloat4x4(&parentTransform.m);
    result = XMMatrixMultiply(parent, result);
  }
  Transform3D selfTransform = static_cast<Transform3D>(m_transform);
  XMMATRIX self = XMLoadFloat4x4(&selfTransform.m);
  result = XMMatrixMultiply(self, result);

  Transform3D t;
  XMStoreFloat4x4(&t.m, result);
  return t;
}

// ****************************************************************************
// Class ShapeFragmentData
// ****************************************************************************

// This constructor is used to load a shape from a file.
ShapeFragmentData::ShapeFragmentData(TextReader* _in, const char* _name)
  : m_numPositions(0),
    m_positions(nullptr),
    m_numNormals(0),
    m_normals(nullptr),
    m_numColours(0),
    m_colours(nullptr),
    m_numVertices(0),
    m_vertices(nullptr),
    m_numTriangles(0),
    m_maxTriangles(0),
    m_triangles(nullptr),
    m_name(nullptr),
    m_parentName(nullptr),
    m_baseTransform(Neuron::Transform3D::Identity()),
    m_center(0.0f, 0.0f, 0.0f),
    m_radius(-1.0f),
    m_mostPositiveY(0.0f),
    m_mostNegativeY(0.0f),
    m_fragmentIndex(-1)
{
  m_maxTriangles = 1;
  m_triangles = new ShapeTriangle[m_maxTriangles];

  DEBUG_ASSERT(_name);
  m_name = _strdup(_name);

  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;

    char* firstWord = _in->GetNextToken();
    char* secondWord = _in->GetNextToken();

    if (_stricmp(firstWord, "ParentName") == 0)
      m_parentName = _strdup(secondWord);
    else if (_stricmp(firstWord, "front") == 0)
    {
      m_baseTransform.m._31 = static_cast<float>(atof(secondWord));
      m_baseTransform.m._32 = static_cast<float>(atof(_in->GetNextToken()));
      m_baseTransform.m._33 = static_cast<float>(atof(_in->GetNextToken()));
    }
    else if (_stricmp(firstWord, "up") == 0)
    {
      m_baseTransform.m._21 = static_cast<float>(atof(secondWord));
      m_baseTransform.m._22 = static_cast<float>(atof(_in->GetNextToken()));
      m_baseTransform.m._23 = static_cast<float>(atof(_in->GetNextToken()));
    }
    else if (_stricmp(firstWord, "pos") == 0)
    {
      m_baseTransform.m._41 = static_cast<float>(atof(secondWord));
      m_baseTransform.m._42 = static_cast<float>(atof(_in->GetNextToken()));
      m_baseTransform.m._43 = static_cast<float>(atof(_in->GetNextToken()));
    }
    else if (_stricmp(firstWord, "Positions") == 0)
    {
      int numPositions = atoi(secondWord);
      ParsePositionBlock(_in, numPositions);
    }
    else if (_stricmp(firstWord, "Normals") == 0)
    {
      int numNorms = atoi(secondWord);
      ParseNormalBlock(_in, numNorms);
    }
    else if (_stricmp(firstWord, "Colours") == 0)
    {
      int numColours = atoi(secondWord);
      ParseColourBlock(_in, numColours);
    }
    else if (_stricmp(firstWord, "Vertices") == 0)
    {
      int numVerts = atoi(secondWord);
      ParseVertexBlock(_in, numVerts);
    }
    else if (_stricmp(firstWord, "Strips") == 0)
    {
      int numStrips = atoi(secondWord);
      ParseAllStripBlocks(_in, numStrips);
      break;
    }
    else if (_stricmp(firstWord, "Triangles") == 0)
    {
      int numTriangles = atoi(secondWord);
      ParseTriangleBlock(_in, numTriangles);
      break;
    }
  }

  m_baseTransform.Orthonormalize();

  if (!m_name)
    m_name = _strdup("unknown");
  if (!m_parentName)
    m_parentName = _strdup("unknown");

  GenerateNormals();
}

// This constructor is used when you want to build a shape from scratch yourself,
// eg in the exporter, or for the SceneRoot node.
ShapeFragmentData::ShapeFragmentData(const char* _name, const char* _parentName)
  : m_numPositions(0),
    m_positions(nullptr),
    m_numNormals(0),
    m_normals(nullptr),
    m_numColours(0),
    m_colours(nullptr),
    m_numVertices(0),
    m_vertices(nullptr),
    m_numTriangles(0),
    m_maxTriangles(0),
    m_triangles(nullptr),
    m_center(0.0f, 0.0f, 0.0f),
    m_radius(-1.0f),
    m_mostPositiveY(0.0f),
    m_mostNegativeY(0.0f),
    m_fragmentIndex(-1)
{
  m_maxTriangles = 1;
  m_triangles = new ShapeTriangle[m_maxTriangles];

  m_baseTransform = Neuron::Transform3D::Identity();
  m_name = _strdup(_name);
  m_parentName = _strdup(_parentName);

  // Remove spaces from m_name
  char* c = m_name;
  char* d = m_name;
  while (d[0] != '\0')
  {
    c[0] = d[0];
    if (c[0] != ' ')
      c++;
    d++;
  }
  c[0] = '\0';

  // Remove spaces from m_parentName
  c = m_parentName;
  d = m_parentName;
  while (d[0] != '\0')
  {
    c[0] = d[0];
    if (c[0] != ' ')
      c++;
    d++;
  }
  c[0] = '\0';
}

ShapeFragmentData::~ShapeFragmentData()
{
  SAFE_DELETE_ARRAY(m_positions);
  free(m_name);
  m_name = nullptr;
  free(m_parentName);
  m_parentName = nullptr;
  delete [] m_vertices;
  m_vertices = nullptr;
  delete [] m_normals;
  m_normals = nullptr;
  delete [] m_colours;
  m_colours = nullptr;
  delete [] m_triangles;
  m_triangles = nullptr;
  m_childFragments.EmptyAndDelete();
  m_childMarkers.EmptyAndDelete();
}

// *** ParsePositionBlock
void ShapeFragmentData::ParsePositionBlock(TextReader* _in, unsigned int _numPositions)
{
  auto positions = new LegacyVector3[_numPositions];

  unsigned int expectedId = 0;
  while (expectedId < _numPositions)
  {
    if (_in->ReadLine() == 0)
      DEBUG_ASSERT(0);

    char* c = _in->GetNextToken();
    if (c && isdigit(c[0]))
    {
      unsigned int id = atoi(c);
      if (id != expectedId || id >= _numPositions)
        return;

      LegacyVector3* vect = &positions[id];
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      vect->x = static_cast<float>(atof(c));
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      vect->y = static_cast<float>(atof(c));
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      vect->z = static_cast<float>(atof(c));

      expectedId++;
    }
  }

  RegisterPositions(positions, _numPositions);
}

// *** ParseNormalBlock
void ShapeFragmentData::ParseNormalBlock(TextReader* _in, unsigned int _numNorms)
{
  if (_numNorms != 0)
    m_normals = new LegacyVector3[_numNorms];
  m_numNormals = _numNorms;

  unsigned int expectedId = 0;
  while (expectedId < _numNorms)
  {
    if (_in->ReadLine() == 0)
      DEBUG_ASSERT(0);

    char* c = _in->GetNextToken();
    if (c && isdigit(c[0]))
    {
      unsigned int id = atoi(c);
      if (id != expectedId || id >= _numNorms)
        DEBUG_ASSERT(0);

      LegacyVector3* vect = &m_normals[id];
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      vect->x = static_cast<float>(atof(c));
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      vect->y = static_cast<float>(atof(c));
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      vect->z = static_cast<float>(atof(c));

      expectedId++;
    }
  }
}

// *** ParseColourBlock
void ShapeFragmentData::ParseColourBlock(TextReader* _in, unsigned int _numColours)
{
  m_colours = new RGBAColour[_numColours];
  m_numColours = _numColours;

  unsigned int expectedId = 0;
  while (expectedId < _numColours)
  {
    if (_in->ReadLine() == 0)
      DEBUG_ASSERT(0);

    char* c = _in->GetNextToken();
    if (c && isdigit(c[0]))
    {
      unsigned int id = atoi(c);
      if (id != expectedId || id >= _numColours)
        DEBUG_ASSERT(0);

      RGBAColour* col = &m_colours[id];
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      col->r = atoi(c);
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      col->g = atoi(c);
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      col->b = atoi(c);
      col->a = 0;

      *col *= 0.85f;
      col->a = 255;

      expectedId++;
    }
  }
}

// *** ParseVertexBlock
void ShapeFragmentData::ParseVertexBlock(TextReader* _in, unsigned int _numVerts)
{
  m_vertices = new VertexPosCol[_numVerts];
  m_numVertices = _numVerts;

  unsigned int expectedId = 0;
  while (expectedId < _numVerts)
  {
    if (_in->ReadLine() == 0)
      DEBUG_ASSERT(0);

    char* c = _in->GetNextToken();
    if (c && isdigit(c[0]))
    {
      unsigned int id = atoi(c);
      if (id != expectedId || id >= _numVerts)
        DEBUG_ASSERT(0);

      VertexPosCol* vert = &m_vertices[id];
      c = _in->GetNextToken();
      vert->m_posId = atoi(c);
      c = _in->GetNextToken();
      vert->m_colId = atoi(c);

      expectedId++;
    }
  }
}

// *** ParseStripBlock
void ShapeFragmentData::ParseStripBlock(TextReader* _in)
{
  _in->ReadLine();
  char* c = _in->GetNextToken();
  DEBUG_ASSERT(c);

  // Read material name
  if (_stricmp(c, "Material") == 0)
  {
    c = _in->GetNextToken();
    DEBUG_ASSERT(c);

    _in->ReadLine();
    c = _in->GetNextToken();
  }

  // Read number of vertices in strip
  DEBUG_ASSERT(c && (_stricmp(c, "Verts") == 0));
  c = _in->GetNextToken();
  DEBUG_ASSERT(c);
  int numVerts = atoi(c);
  DEBUG_ASSERT(numVerts > 2);

  // Now just read a sequence of verts
  int i = 0;
  unsigned int v1 = 0, v2 = 0;
  while (i < numVerts)
  {
    if (_in->ReadLine() == 0)
      DEBUG_ASSERT(0);

    while (_in->TokenAvailable())
    {
      char* token = _in->GetNextToken();
      DEBUG_ASSERT(token[0] == 'v');

      token++;
      unsigned int v3 = static_cast<unsigned int>(atoi(token));
      DEBUG_ASSERT(v3 < m_numVertices);

      if (i >= 2 && v1 != v2 && v2 != v3 && v1 != v3)
      {
        if (m_numTriangles == m_maxTriangles)
        {
          auto newTriangles = new ShapeTriangle[m_maxTriangles * 2];
          memcpy(newTriangles, m_triangles, sizeof(ShapeTriangle) * m_maxTriangles);
          delete [] m_triangles;
          m_triangles = newTriangles;
          m_maxTriangles *= 2;
        }
        if (i & 1)
        {
          m_triangles[m_numTriangles].v1 = v3;
          m_triangles[m_numTriangles].v2 = v2;
          m_triangles[m_numTriangles].v3 = v1;
        }
        else
        {
          m_triangles[m_numTriangles].v1 = v1;
          m_triangles[m_numTriangles].v2 = v2;
          m_triangles[m_numTriangles].v3 = v3;
        }
        m_numTriangles++;
      }

      v1 = v2;
      v2 = v3;

      i++;
    }
  }
}

// *** ParseAllStripBlocks
void ShapeFragmentData::ParseAllStripBlocks(TextReader* _in, unsigned int _numStrips)
{
  unsigned int expectedId = 0;
  while (_in->ReadLine())
  {
    char* c = _in->GetNextToken();
    if (c && _stricmp(c, "Strip") == 0)
    {
      c = _in->GetNextToken();
      unsigned int id = static_cast<unsigned int>(atoi(c));

      DEBUG_ASSERT(id == expectedId);

      ParseStripBlock(_in);

      expectedId++;

      if (expectedId == _numStrips)
        break;
    }
  }

  // Shrink m_triangles to tightly fit its data
  auto newTriangles = new ShapeTriangle[m_numTriangles];
  memcpy(newTriangles, m_triangles, sizeof(ShapeTriangle) * m_numTriangles);
  delete [] m_triangles;
  m_maxTriangles = m_numTriangles;
  m_triangles = newTriangles;
}

void ShapeFragmentData::ParseTriangleBlock(TextReader* _in, unsigned int _numTriangles)
{
  DEBUG_ASSERT(m_numTriangles == 0 && m_maxTriangles == 1 && m_triangles != NULL);
  delete [] m_triangles;

  m_maxTriangles = _numTriangles;
  m_triangles = new ShapeTriangle[_numTriangles];

  while (m_numTriangles < _numTriangles)
  {
    _in->ReadLine();
    char* c = _in->GetNextToken();
    m_triangles[m_numTriangles].v1 = atoi(c);
    c = _in->GetNextToken();
    m_triangles[m_numTriangles].v2 = atoi(c);
    c = _in->GetNextToken();
    m_triangles[m_numTriangles].v3 = atoi(c);

    m_numTriangles++;
  }
}

// *** GenerateNormals
// Currently this function generates one normal for each face in all the
// strips, rather than one normal per vertex. This is what we need for
// facet shading, rather than smooth (gouraud) shading.
void ShapeFragmentData::GenerateNormals()
{
  m_numNormals = m_numTriangles;
  m_normals = new LegacyVector3[m_numNormals];
  int normId = 0;

  for (unsigned int j = 0; j < m_numTriangles; ++j)
  {
    ShapeTriangle* tri = &m_triangles[j];
    const VertexPosCol& vertA = m_vertices[tri->v1];
    const VertexPosCol& vertB = m_vertices[tri->v2];
    const VertexPosCol& vertC = m_vertices[tri->v3];
    LegacyVector3& a = m_positions[vertA.m_posId];
    LegacyVector3& b = m_positions[vertB.m_posId];
    LegacyVector3& c = m_positions[vertC.m_posId];
    LegacyVector3 ab = b - a;
    LegacyVector3 bc = c - b;
    m_normals[normId] = ab ^ bc;
    m_normals[normId].Normalise();
    normId++;
  }
}

// *** RegisterPositions
void ShapeFragmentData::RegisterPositions(LegacyVector3* _positions, unsigned int _numPositions)
{
  unsigned int i;

  delete [] m_positions;
  m_positions = _positions;
  m_numPositions = _numPositions;

  // Calculate bounding sphere
  {
    // Find the center of the fragment
    {
      float minX = FLT_MAX;
      float maxX = -FLT_MAX;
      float minY = FLT_MAX;
      float maxY = -FLT_MAX;
      float minZ = FLT_MAX;
      float maxZ = -FLT_MAX;
      for (i = 0; i < m_numPositions; ++i)
      {
        if (m_positions[i].x < minX)
          minX = m_positions[i].x;
        if (m_positions[i].x > maxX)
          maxX = m_positions[i].x;
        if (m_positions[i].y < minY)
          minY = m_positions[i].y;
        if (m_positions[i].y > maxY)
          maxY = m_positions[i].y;
        if (m_positions[i].z < minZ)
          minZ = m_positions[i].z;
        if (m_positions[i].z > maxZ)
          maxZ = m_positions[i].z;
      }
      m_center.x = (maxX + minX) / 2;
      m_center.y = (maxY + minY) / 2;
      m_center.z = (maxZ + minZ) / 2;
    }

    // Find the point furthest from the center
    float radiusSquared = 0.0f;
    for (i = 0; i < m_numPositions; ++i)
    {
      LegacyVector3 delta = m_center - m_positions[i];
      float magSquared = delta.MagSquared();
      if (magSquared > radiusSquared)
        radiusSquared = magSquared;
    }
    m_radius = sqrtf(radiusSquared);
  }

  // Calculate bounding vertical cylinder
  {
    float radiusSquared = 0.0f;
    m_mostPositiveY = -FLT_MAX;
    m_mostNegativeY = FLT_MAX;
    for (i = 0; i < m_numPositions; ++i)
    {
      if (m_positions[i].y > m_mostPositiveY)
        m_mostPositiveY = m_positions[i].y;
      if (m_positions[i].y < m_mostNegativeY)
        m_mostNegativeY = m_positions[i].y;
      LegacyVector3 delta = m_center - m_positions[i];
      delta.y = 0.0f;
      float magSquared = delta.MagSquared();
      if (magSquared > radiusSquared)
            radiusSquared = magSquared;
        }
        float height = m_mostPositiveY - m_mostNegativeY;
    float cylinderVolume = M_PI * radiusSquared * height;
    float sphereVolume = 4.0f / 3.0f * M_PI * m_radius * m_radius * m_radius;

    if (cylinderVolume < sphereVolume)
    {
    }
  }
}

void ShapeFragmentData::RegisterNormals(LegacyVector3* _norms, unsigned int _numNorms)
{
  delete [] m_normals;
  m_normals = _norms;
  m_numNormals = _numNorms;
}

void ShapeFragmentData::RegisterColours(RGBAColour* _colours, unsigned int _numColours)
{
  delete [] m_colours;
  m_colours = _colours;
  m_numColours = _numColours;
}

void ShapeFragmentData::RegisterVertices(VertexPosCol* _verts, unsigned int _numVerts)
{
  delete [] m_vertices;
  m_vertices = _verts;
  m_numVertices = _numVerts;
}

void ShapeFragmentData::RegisterTriangles(ShapeTriangle* _tris, unsigned int _numTris)
{
  delete [] m_triangles;
  m_triangles = _tris;
  m_numTriangles = _numTris;
}

// *** AssignIndices
// Depth-first traversal to assign fragment ordinals
int ShapeFragmentData::AssignIndices(int _nextIndex)
{
  m_fragmentIndex = _nextIndex;
  int next = _nextIndex + 1;
  int numChildren = m_childFragments.Size();
  for (int i = 0; i < numChildren; ++i)
    next = m_childFragments.GetData(i)->AssignIndices(next);
  return next;
}

void ShapeFragmentData::Render(const FragmentState* _states, float _predictionTime) const
{
#ifndef EXPORTER_BUILD
  using namespace DirectX;
  Neuron::Transform3D predicted;
  if (_states)
  {
    const FragmentState& state = _states[m_fragmentIndex];
    predicted = state.transform;
    predicted.RotateAround(XMVectorScale(
      XMVectorSet(state.angVel.x, state.angVel.y, state.angVel.z, 0.0f), _predictionTime));
    predicted.Translate(XMVectorScale(
      XMVectorSet(state.vel.x, state.vel.y, state.vel.z, 0.0f), _predictionTime));
  }
  else
  {
    predicted = m_baseTransform;
  }

  bool matrixIsIdentity = predicted.IsIdentity();
  auto& mv = OpenGLD3D::GetModelViewStack();
  if (!matrixIsIdentity)
  {
    mv.Push();
    mv.Multiply(predicted);
  }

  RenderSlow();

  int numChildren = m_childFragments.Size();
  for (int i = 0; i < numChildren; ++i)
    m_childFragments.GetData(i)->Render(_states, _predictionTime);

  if (!matrixIsIdentity)
    mv.Pop();
#endif
}

void ShapeFragmentData::RenderSlow() const
{
#ifndef EXPORTER_BUILD
  if (!m_numTriangles)
    return;
  glBegin(GL_TRIANGLES);

  int norm = 0;
  for (unsigned int i = 0; i < m_numTriangles; i++)
  {
    const VertexPosCol* vertA = &m_vertices[m_triangles[i].v1];
    const VertexPosCol* vertB = &m_vertices[m_triangles[i].v2];
    const VertexPosCol* vertC = &m_vertices[m_triangles[i].v3];

    constexpr unsigned char alpha = 255;

    glNormal3fv(m_normals[norm].GetData());
    glColor4ub(m_colours[vertA->m_colId].r, m_colours[vertA->m_colId].g, m_colours[vertA->m_colId].b, alpha);
    glVertex3fv(m_positions[vertA->m_posId].GetData());

    glNormal3fv(m_normals[norm].GetData());
    glColor4ub(m_colours[vertB->m_colId].r, m_colours[vertB->m_colId].g, m_colours[vertB->m_colId].b, alpha);
    glVertex3fv(m_positions[vertB->m_posId].GetData());

    glNormal3fv(m_normals[norm].GetData());
    glColor4ub(m_colours[vertC->m_colId].r, m_colours[vertC->m_colId].g, m_colours[vertC->m_colId].b, alpha);
    glVertex3fv(m_positions[vertC->m_posId].GetData());
    norm++;
  }
  glEnd();
#endif
}

void ShapeFragmentData::RenderNative(const FragmentState* _states, float _predictionTime) const
{
#ifndef EXPORTER_BUILD
  using namespace DirectX;

  // Look up GPU data for the owning shape
  DEBUG_ASSERT(m_ownerShape);
  const ShapeMeshGPU* gpu = ShapeMeshCache::Get().GetGPUData(m_ownerShape);
  if (!gpu)
    return; // Fallback: shape not uploaded (should not happen after EnsureUploaded)

  // Compute predicted transform for this fragment (same logic as Render())
  Neuron::Transform3D predicted;
  if (_states)
  {
    const FragmentState& state = _states[m_fragmentIndex];
    predicted = state.transform;
    predicted.RotateAround(XMVectorScale(
      XMVectorSet(state.angVel.x, state.angVel.y, state.angVel.z, 0.0f), _predictionTime));
    predicted.Translate(XMVectorScale(
      XMVectorSet(state.vel.x, state.vel.y, state.vel.z, 0.0f), _predictionTime));
  }
  else
  {
    predicted = m_baseTransform;
  }

  bool matrixIsIdentity = predicted.IsIdentity();
  auto& mv = OpenGLD3D::GetModelViewStack();
  if (!matrixIsIdentity)
  {
    mv.Push();
    mv.Multiply(predicted);
  }

  // Draw this fragment's triangles from the GPU buffer
  if (m_numTriangles > 0 && m_fragmentIndex >= 0
      && m_fragmentIndex < static_cast<int>(gpu->fragments.size()))
  {
    const FragmentGPURange& range = gpu->fragments[m_fragmentIndex];
    if (range.vertexCount > 0)
    {
      // PrepareDrawState handles: CB upload, texture/sampler bind, PSO select,
      // viewport/scissor set — same as the GL path through issueDrawCall.
      OpenGLD3D::PrepareDrawState(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      auto* cmdList = Graphics::Core::Get().GetCommandList();
      cmdList->IASetVertexBuffers(0, 1, &gpu->vbView);
      cmdList->DrawInstanced(range.vertexCount, 1, range.vertexStart, 0);
    }
  }

  // Recurse into children
  int numChildren = m_childFragments.Size();
  for (int i = 0; i < numChildren; ++i)
    m_childFragments.GetData(i)->RenderNative(_states, _predictionTime);

  if (!matrixIsIdentity)
    mv.Pop();
#endif
}

// *** LookupFragment
// Recursively look through all child fragments until we find a name match
ShapeFragmentData* ShapeFragmentData::LookupFragment(const char* _name)
{
  if (_stricmp(_name, m_name) == 0)
    return this;
  int numChildFragments = m_childFragments.Size();
  for (int i = 0; i < numChildFragments; ++i)
  {
    ShapeFragmentData* frag = m_childFragments.GetData(i)->LookupFragment(_name);
    if (frag)
      return frag;
  }

  return nullptr;
}

// *** LookupMarker
// Recursively look through all child fragments until we find one with a marker
// matching the specified name
ShapeMarkerData* ShapeFragmentData::LookupMarker(const char* _name)
{
  int i;

  int numMarkers = m_childMarkers.Size();
  for (i = 0; i < numMarkers; ++i)
  {
    ShapeMarkerData* marker = m_childMarkers.GetData(i);
    if (_stricmp(_name, marker->m_name) == 0)
      return marker;
  }

  int numChildFragments = m_childFragments.Size();
  for (i = 0; i < numChildFragments; ++i)
  {
    ShapeMarkerData* marker = m_childFragments.GetData(i)->LookupMarker(_name);
    if (marker)
      return marker;
  }

  return nullptr;
}

bool ShapeFragmentData::RayHit(const FragmentState* _states, RayPackage* _package, const Matrix34& _transform, bool _accurate) const
{
#ifndef EXPORTER_BUILD
  const Transform3D& fragTransform = _states ? _states[m_fragmentIndex].transform : m_baseTransform;
  Matrix34 totalMatrix(fragTransform * _transform);
  LegacyVector3 center = m_center * totalMatrix;

  // First do bounding sphere check
  if (m_radius > 0.0f && RaySphereIntersection(_package->m_rayStart, _package->m_rayDir, center, m_radius, _package->m_rayLen))
  {
    // Exit early to save loads of work if we don't care about accuracy too much
    if (_accurate == false)
      return true;

    // Compute World Space versions of all the vertices
    if (t_scratchWS.size() < m_numPositions)
      t_scratchWS.resize(m_numPositions);
    LegacyVector3* positionsInWS = t_scratchWS.data();
    for (unsigned int i = 0; i < m_numPositions; ++i)
      positionsInWS[i] = m_positions[i] * totalMatrix;

    // Check each triangle in this fragment for intersection
    for (unsigned int j = 0; j < m_numTriangles; ++j)
    {
      VertexPosCol* v1 = &m_vertices[m_triangles[j].v1];
      VertexPosCol* v2 = &m_vertices[m_triangles[j].v2];
      VertexPosCol* v3 = &m_vertices[m_triangles[j].v3];
      if (RayTriIntersection(_package->m_rayStart, _package->m_rayDir, positionsInWS[v1->m_posId], positionsInWS[v2->m_posId],
                             positionsInWS[v3->m_posId]))
        return true;
    }
  }

  // If we haven't found a hit then recurse into all child fragments
  int numFragments = m_childFragments.Size();
  for (int i = 0; i < numFragments; ++i)
  {
    ShapeFragmentData* frag = m_childFragments.GetData(i);
    if (frag->RayHit(_states, _package, totalMatrix, _accurate))
      return true;
  }

#endif
  return false;
}

// *** SphereHit
bool ShapeFragmentData::SphereHit(const FragmentState* _states, SpherePackage* _package, const Matrix34& _transform, bool _accurate) const
{
#ifndef EXPORTER_BUILD
  const Transform3D& fragTransform = _states ? _states[m_fragmentIndex].transform : m_baseTransform;
  Matrix34 totalMatrix(fragTransform * _transform);
  LegacyVector3 center = m_center * totalMatrix;

  if (m_radius > 0.0f && SphereSphereIntersection(_package->m_pos, _package->m_radius, center, m_radius))
  {
    // Exit early to save loads of work if we don't care about accuracy too much
    if (_accurate == false)
      return true;

    // Compute World Space versions of all the vertices
    if (t_scratchWS.size() < m_numPositions)
      t_scratchWS.resize(m_numPositions);
    LegacyVector3* positionsInWS = t_scratchWS.data();
    for (unsigned int i = 0; i < m_numPositions; ++i)
      positionsInWS[i] = m_positions[i] * totalMatrix;

    // Check each triangle in this fragment for intersection
    for (unsigned int j = 0; j < m_numTriangles; ++j)
    {
      VertexPosCol* v1 = &m_vertices[m_triangles[j].v1];
      VertexPosCol* v2 = &m_vertices[m_triangles[j].v2];
      VertexPosCol* v3 = &m_vertices[m_triangles[j].v3];
      if (SphereTriangleIntersection(_package->m_pos, _package->m_radius, positionsInWS[v1->m_posId], positionsInWS[v2->m_posId],
                                     positionsInWS[v3->m_posId]))
        return true;
    }
  }

  // If we haven't found a hit then recurse into all child fragments
  int numFragments = m_childFragments.Size();
  for (int i = 0; i < numFragments; ++i)
  {
    ShapeFragmentData* frag = m_childFragments.GetData(i);
    if (frag->SphereHit(_states, _package, totalMatrix, _accurate))
      return true;
  }

#endif
  return false;
}

// *** ShapeHit
bool ShapeFragmentData::ShapeHit(const FragmentState* _states, ShapeStatic* _shape, const Matrix34& _theTransform, const Matrix34& _ourTransform, bool _accurate) const
{
#ifndef EXPORTER_BUILD

  const Transform3D& fragTransform = _states ? _states[m_fragmentIndex].transform : m_baseTransform;
  Matrix34 totalMatrix(fragTransform * _ourTransform);
  LegacyVector3 center = m_center * totalMatrix;

  if (m_radius > 0.0f)
  {
    SpherePackage package(center, m_radius);
    if (_shape->SphereHit(&package, _theTransform, _accurate))
      return true;
  }

  int i;

  // If we haven't found a hit then recurse into all child fragments
  int numFragments = m_childFragments.Size();
  for (i = 0; i < numFragments; ++i)
  {
    ShapeFragmentData* frag = m_childFragments.GetData(i);
    if (frag->ShapeHit(_states, _shape, _theTransform, totalMatrix, _accurate))
      return true;
  }

#endif
  return false;
}

void ShapeFragmentData::CalculateCenter(const FragmentState* _states, const Matrix34& _transform, LegacyVector3& _center, int& _numFragments) const
{
  const Transform3D& fragTransform = _states ? _states[m_fragmentIndex].transform : m_baseTransform;
  Matrix34 totalMatrix(fragTransform * _transform);
  LegacyVector3 center = m_center * totalMatrix;

  _center += center;
  _numFragments++;

  int numFragments = m_childFragments.Size();
  for (int i = 0; i < numFragments; ++i)
  {
    ShapeFragmentData* frag = m_childFragments.GetData(i);
    frag->CalculateCenter(_states, totalMatrix, _center, _numFragments);
  }
}

void ShapeFragmentData::CalculateRadius(const FragmentState* _states, const Matrix34& _transform, const LegacyVector3& _center, float& _radius) const
{
  const Transform3D& fragTransform = _states ? _states[m_fragmentIndex].transform : m_baseTransform;
  Matrix34 totalMatrix(fragTransform * _transform);
  LegacyVector3 center = m_center * totalMatrix;

  float distance = (center - _center).Mag();
  if (distance + m_radius > _radius)
    _radius = distance + m_radius;

  int numFragments = m_childFragments.Size();
  for (int i = 0; i < numFragments; ++i)
  {
    ShapeFragmentData* frag = m_childFragments.GetData(i);
    frag->CalculateRadius(_states, totalMatrix, _center, _radius);
  }
}

// ****************************************************************************
// Class ShapeStatic
// ****************************************************************************

ShapeStatic::ShapeStatic()
  : m_rootFragment(nullptr),
    m_name(nullptr),
    m_numFragments(0),
    m_defaultStates(nullptr) {}

ShapeStatic::ShapeStatic(const char* _filename)
  : m_rootFragment(nullptr),
    m_name(nullptr),
    m_numFragments(0),
    m_defaultStates(nullptr)
{
  TextFileReader in(_filename);
  Load(&in);
}

ShapeStatic::ShapeStatic(TextReader* _in)
  : m_rootFragment(nullptr),
    m_name(nullptr),
    m_numFragments(0),
    m_defaultStates(nullptr) { Load(_in); }

ShapeStatic::~ShapeStatic()
{
  delete m_rootFragment;
  free(m_name);
  delete [] m_defaultStates;
}

void ShapeStatic::Load(TextReader* _in)
{
  m_name = _strdup(_in->GetFilename());

  constexpr int maxFrags = 100;
  constexpr int maxMarkers = 100;
  int currentFrag = 0;
  int currentMarker = 0;
  ShapeFragmentData* allFrags[maxFrags];
  ShapeMarkerData* allMarkers[maxMarkers];

  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;

    char* c = _in->GetNextToken();

    if (_stricmp(c, "fragment") == 0)
    {
      DEBUG_ASSERT(currentFrag < maxFrags);
      c = _in->GetNextToken();
      allFrags[currentFrag] = new ShapeFragmentData(_in, c);
      currentFrag++;
    }
    else if (_stricmp(c, "marker") == 0)
    {
      DEBUG_ASSERT(currentMarker < maxMarkers);
      c = _in->GetNextToken();
      allMarkers[currentMarker] = new ShapeMarkerData(_in, c);
      currentMarker++;
    }
  }

  m_rootFragment = new ShapeFragmentData("SceneRoot", "");

  // We need to build the hierarchy of fragments from the flat array
  for (int i = 0; i < currentFrag; ++i)
  {
    if (_stricmp(allFrags[i]->m_parentName, "SceneRoot") == 0)
      m_rootFragment->m_childFragments.PutData(allFrags[i]);
    else
    {
      // find the ith fragment's parent
      int j;
      for (j = 0; j < currentFrag; ++j)
      {
        if (i == j)
          continue;
        DEBUG_ASSERT(_stricmp(allFrags[i]->m_name, allFrags[j]->m_name) != 0);
        if (_stricmp(allFrags[i]->m_parentName, allFrags[j]->m_name) == 0)
        {
          allFrags[j]->m_childFragments.PutData(allFrags[i]);
          break;
        }
      }
      DEBUG_ASSERT(j < currentFrag);
    }
  }

  // Assign fragment indices via depth-first traversal
  m_numFragments = m_rootFragment->AssignIndices(0);

  // Set owner back-pointers on all fragments (used by ShapeMeshCache)
  struct SetOwner
  {
    static void Set(ShapeFragmentData* _frag, ShapeStatic* _owner)
    {
      _frag->m_ownerShape = _owner;
      int numChildren = _frag->m_childFragments.Size();
      for (int i = 0; i < numChildren; ++i)
        Set(_frag->m_childFragments.GetData(i), _owner);
    }
  };
  SetOwner::Set(m_rootFragment, this);

  // Build rest-pose default states from base transforms
  m_defaultStates = new FragmentState[m_numFragments];
  for (int i = 0; i < m_numFragments; ++i)
  {
    m_defaultStates[i].angVel.Zero();
    m_defaultStates[i].vel.Zero();
  }
  // Fill transforms from the fragment tree via DFS — helper lambda
  struct FillStates
  {
    static void Fill(const ShapeFragmentData* _frag, FragmentState* _states)
    {
      _states[_frag->m_fragmentIndex].transform = _frag->m_baseTransform;
      int numChildren = _frag->m_childFragments.Size();
      for (int i = 0; i < numChildren; ++i)
        Fill(_frag->m_childFragments.GetData(i), _states);
    }
  };
  FillStates::Fill(m_rootFragment, m_defaultStates);

  // Add the ShapeMarkers into the fragment tree and build parent index arrays
  for (int i = 0; i < currentMarker; ++i)
  {
    ShapeFragmentData* parent = m_rootFragment->LookupFragment(allMarkers[i]->m_parentName);
    DEBUG_ASSERT(parent);
    parent->m_childMarkers.PutData(allMarkers[i]);

    int depth = allMarkers[i]->m_depth - 1;
    allMarkers[i]->m_parentIndices[depth] = parent->m_fragmentIndex;
    depth--;
    while (_stricmp(parent->m_name, "SceneRoot") != 0)
    {
      parent = m_rootFragment->LookupFragment(parent->m_parentName);
      DEBUG_ASSERT(parent && depth >= 0);
      allMarkers[i]->m_parentIndices[depth] = parent->m_fragmentIndex;
      depth--;
    }
    DEBUG_ASSERT(depth == -1);
  }
}

void ShapeStatic::Render(float _predictionTime, const Matrix34& _transform) const { Render(_predictionTime, static_cast<Transform3D>(_transform)); }

void XM_CALLCONV ShapeStatic::Render(float _predictionTime, XMMATRIX _transform) const
{
  // Ensure GPU vertex buffer exists (no-op after first call)
  ShapeMeshCache::Get().EnsureUploaded(this);

  glEnable(GL_COLOR_MATERIAL);
  auto& mv = OpenGLD3D::GetModelViewStack();
  mv.Push();
  mv.Multiply(_transform);

  m_rootFragment->RenderNative(m_defaultStates, _predictionTime);

  glDisable(GL_COLOR_MATERIAL);
  mv.Pop();
}

bool ShapeStatic::RayHit(RayPackage* _package, const Matrix34& _transform, bool _accurate) const
{
  return m_rootFragment->RayHit(m_defaultStates, _package, _transform, _accurate);
}

bool ShapeStatic::SphereHit(SpherePackage* _package, const Matrix34& _transform, bool _accurate) const
{
  return m_rootFragment->SphereHit(m_defaultStates, _package, _transform, _accurate);
}

bool ShapeStatic::ShapeHit(ShapeStatic* _shape, const Matrix34& _theTransform, const Matrix34& _ourTransform, bool _accurate) const
{
  return m_rootFragment->ShapeHit(m_defaultStates, _shape, _theTransform, _ourTransform, _accurate);
}

LegacyVector3 ShapeStatic::CalculateCenter(const Matrix34& _transform) const
{
  LegacyVector3 center;
  int numFragments = 0;

  m_rootFragment->CalculateCenter(m_defaultStates, _transform, center, numFragments);

  center /= static_cast<float>(numFragments);

  return center;
}

float ShapeStatic::CalculateRadius(const Matrix34& _transform, const LegacyVector3& _center) const
{
  float radius = 0.0f;

  m_rootFragment->CalculateRadius(m_defaultStates, _transform, _center, radius);

  return radius;
}

int ShapeStatic::GetFragmentIndex(const char* _name) const
{
  ShapeFragmentData* frag = m_rootFragment->LookupFragment(_name);
  return frag ? frag->m_fragmentIndex : -1;
}

ShapeFragmentData* ShapeStatic::GetFragmentData(const char* _name) const
{
  return m_rootFragment->LookupFragment(_name);
}

ShapeMarkerData* ShapeStatic::GetMarkerData(const char* _name) const
{
  return m_rootFragment->LookupMarker(_name);
}

Matrix34 ShapeStatic::GetMarkerWorldMatrix(const ShapeMarkerData* _marker, const Matrix34& _rootTransform) const
{
  return _marker->GetWorldMatrix(m_defaultStates, _rootTransform);
}

Transform3D ShapeStatic::GetMarkerWorldTransform(const ShapeMarkerData* _marker, const Transform3D& _rootTransform) const
{
  return _marker->GetWorldTransform(m_defaultStates, _rootTransform);
}

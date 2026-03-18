#pragma once

#include "llist.h"
#include "matrix34.h"
#include "rgb_colour.h"
#include "LegacyVector3.h"

class TextReader;
class ShapeFragmentData;
class Matrix34;
class ShapeStatic;

// ****************
// Class RayPackage
// ****************

class RayPackage
{
  public:
    LegacyVector3 m_rayStart;
    LegacyVector3 m_rayEnd;
    LegacyVector3 m_rayDir;
    float m_rayLen;

    RayPackage(const LegacyVector3& _start, const LegacyVector3& _dir, float _length = 1e10)
      : m_rayStart(_start),
        m_rayDir(_dir)
    {
      m_rayLen = _length;
      m_rayEnd = m_rayStart + m_rayDir * _length;
    }
};

// *******************
// Class SpherePackage
// *******************

class SpherePackage
{
  public:
    LegacyVector3 m_pos;
    float m_radius;

    SpherePackage(const LegacyVector3& _pos, float _radius)
      : m_pos(_pos),
        m_radius(_radius) {}
};

// ********************
// Struct FragmentState
// ********************

// Per-instance mutable state for one fragment. Small, contiguous, cache-friendly.
struct FragmentState
{
    Transform3D     transform;  // 64 bytes — current animated transform
    LegacyVector3   angVel;     // 12 bytes — angular velocity for prediction
    LegacyVector3   vel;        // 12 bytes — linear velocity for prediction
    // Total: 88 bytes per fragment
};

// ********************
// Class ShapeMarkerData
// ********************

class ShapeMarkerData
{
  public:
    Matrix34 m_transform;
    char* m_name;
    char* m_parentName;
    int m_depth; // Number of levels in the shape fragment tree from root to self
    int* m_parentIndices; // Fragment ordinals, NOT raw pointers

    ShapeMarkerData(const char* _name, const char* _parentName, int _depth, const Matrix34& _transform);
    ShapeMarkerData(TextReader* _in, const char* _name);
    ~ShapeMarkerData();

    Matrix34 GetWorldMatrix(const FragmentState* _states, const Matrix34& _rootTransform) const;
    Transform3D GetWorldTransform(const FragmentState* _states, const Transform3D& _rootTransform) const;
};

// ******************
// Class VertexPosCol
// ******************

class VertexPosCol
{
  public:
    unsigned short m_posId;
    unsigned short m_colId;
};

// *******************
// Class ShapeTriangle
// *******************

class ShapeTriangle
{
  public:
    unsigned short v1, v2, v3; // Vertex indices (into ShapeFragmentData::m_vertices)
};

// ************************
// Class ShapeFragmentData
// ************************

class ShapeFragmentData
{
  protected:
    void ParsePositionBlock(TextReader* in, unsigned int numPositions);
    void ParseNormalBlock(TextReader* in, unsigned int numNorms);
    void ParseColourBlock(TextReader* in, unsigned int numColours);
    void ParseVertexBlock(TextReader* in, unsigned int numVerts);
    void ParseStripBlock(TextReader* in);
    void ParseAllStripBlocks(TextReader* in, unsigned int numStrips);
    void ParseTriangleBlock(TextReader* in, unsigned int numTriangles);

    void GenerateNormals();

  public:
    // Geometry (loaded from disk, never modified)
    unsigned int m_numPositions;
    LegacyVector3* m_positions;
    unsigned int m_numNormals;
    LegacyVector3* m_normals;
    unsigned int m_numColours;
    RGBAColour* m_colours;
    unsigned int m_numVertices; // Each element contains an index into m_positions and an index into m_colours
    VertexPosCol* m_vertices;
    unsigned int m_numTriangles;
    unsigned int m_maxTriangles;
    ShapeTriangle* m_triangles;

    // Identity / hierarchy
    char* m_name;
    char* m_parentName;
    Transform3D m_baseTransform; // As loaded from file (rest pose)

    // Bounding volume (computed once at load)
    LegacyVector3 m_center;
    float m_radius;
    float m_mostPositiveY;
    float m_mostNegativeY;

    // Tree structure
    int m_fragmentIndex; // Ordinal in ShapeStatic's flat array
    LList<ShapeFragmentData*> m_childFragments;
    LList<ShapeMarkerData*> m_childMarkers;

    ShapeFragmentData(TextReader* _in, const char* _name);
    ShapeFragmentData(const char* _name, const char* _parentName);
    ~ShapeFragmentData();

    void RegisterPositions(LegacyVector3* positions, unsigned int numPositions);
    void RegisterNormals(LegacyVector3* norms, unsigned int numNorms);
    void RegisterColours(RGBAColour* colours, unsigned int numColours);
    void RegisterVertices(VertexPosCol* verts, unsigned int numVerts);
    void RegisterTriangles(ShapeTriangle* tris, unsigned int numTris);

    // Render / hit-test (operate on shared data + external per-instance state)
    void Render(const FragmentState* _states, float _predictionTime) const;
    void RenderSlow() const;

    ShapeFragmentData* LookupFragment(const char* _name); // Recurses into child fragments
    ShapeMarkerData* LookupMarker(const char* _name); // Recurses into child fragments

    void CalculateCenter(const FragmentState* _states, const Matrix34& _transform,
                         LegacyVector3& _center, int& _numFragments) const;
    void CalculateRadius(const FragmentState* _states, const Matrix34& _transform,
                         const LegacyVector3& _center, float& _radius) const;

    bool RayHit(const FragmentState* _states, RayPackage* _package,
                const Matrix34& _transform, bool _accurate = false) const;
    bool SphereHit(const FragmentState* _states, SpherePackage* _package,
                   const Matrix34& _transform, bool _accurate = false) const;
    bool ShapeHit(const FragmentState* _states, ShapeStatic* _shape,
                  const Matrix34& _theTransform,
                  const Matrix34& _ourTransform,
                  bool _accurate = false) const;

    // Assign fragment indices via depth-first traversal (called during load)
    int AssignIndices(int _nextIndex);
};

// ****************
// Class ShapeStatic
// ****************

class ShapeStatic
{
  protected:
    void Load(TextReader* _in);

  public:
    ShapeFragmentData* m_rootFragment;
    char* m_name;
    int m_numFragments;         // Total fragment count (flat)
    FragmentState* m_defaultStates; // Rest-pose states, indexed by fragment ordinal

    ShapeStatic();
    ShapeStatic(const char* _filename);
    ShapeStatic(TextReader* _in);
    ~ShapeStatic();

    // Render/hit-test using default (rest-pose) transforms — for non-animating users
    void Render(float _predictionTime, const Matrix34& _transform) const;
    void XM_CALLCONV Render(float _predictionTime, XMMATRIX _transform) const;

    bool RayHit(RayPackage* _package, const Matrix34& _transform, bool _accurate = false) const;
    bool SphereHit(SpherePackage* _package, const Matrix34& _transform, bool _accurate = false) const;
    bool ShapeHit(ShapeStatic* _shape, const Matrix34& _theTransform,
                  const Matrix34& _ourTransform, bool _accurate = false) const;

    LegacyVector3 CalculateCenter(const Matrix34& _transform) const;
    float CalculateRadius(const Matrix34& _transform, const LegacyVector3& _center) const;

    // Fragment/marker lookup by name
    int GetFragmentIndex(const char* _name) const;
    ShapeFragmentData* GetFragmentData(const char* _name) const;
    ShapeMarkerData* GetMarkerData(const char* _name) const;

    // Marker world-matrix using rest-pose transforms (for non-animating callers)
    Matrix34 GetMarkerWorldMatrix(const ShapeMarkerData* _marker,
                                  const Matrix34& _rootTransform) const;
    Transform3D GetMarkerWorldTransform(const ShapeMarkerData* _marker,
                                        const Transform3D& _rootTransform) const;
};

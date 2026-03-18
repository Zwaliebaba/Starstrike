#pragma once

#include "ShapeStatic.h"

// Per-entity lightweight animation state. Owns a flat FragmentState[] array
// indexed by fragment ordinal. Non-copyable, movable.
class ShapeInstance
{
  public:
    ShapeStatic*    m_master;           // Non-owning; Resource owns all masters
    FragmentState*  m_fragmentStates;   // Flat array [m_master->m_numFragments]

    ShapeInstance();
    explicit ShapeInstance(ShapeStatic* _master);
    ~ShapeInstance();

    // Non-copyable
    ShapeInstance(const ShapeInstance&) = delete;
    ShapeInstance& operator=(const ShapeInstance&) = delete;

    // Movable
    ShapeInstance(ShapeInstance&& _other) noexcept;
    ShapeInstance& operator=(ShapeInstance&& _other) noexcept;

    // Access per-instance state by fragment index
    FragmentState&       GetState(int _fragmentIndex);
    const FragmentState& GetState(int _fragmentIndex) const;

    // Convenience: lookup + access combined
    FragmentState&       GetState(const char* _fragmentName);

    // Render/hit using this instance's transforms
    void Render(float _predictionTime, const Matrix34& _transform) const;
    void XM_CALLCONV Render(float _predictionTime, XMMATRIX _transform) const;

    bool RayHit(RayPackage* _package, const Matrix34& _transform, bool _accurate = false) const;
    bool SphereHit(SpherePackage* _package, const Matrix34& _transform, bool _accurate = false) const;
    bool ShapeHit(ShapeStatic* _shape, const Matrix34& _theTransform,
                  const Matrix34& _ourTransform, bool _accurate = false) const;

    // Marker world-matrix using this instance's fragment transforms
    Matrix34    GetMarkerWorldMatrix(const ShapeMarkerData* _marker,
                                     const Matrix34& _rootTransform) const;
    Transform3D GetMarkerWorldTransform(const ShapeMarkerData* _marker,
                                        const Transform3D& _rootTransform) const;

    LegacyVector3 CalculateCenter(const Matrix34& _transform) const;
    float CalculateRadius(const Matrix34& _transform, const LegacyVector3& _center) const;

    // Access the master for geometry queries
    ShapeStatic* GetMaster() const { return m_master; }
};

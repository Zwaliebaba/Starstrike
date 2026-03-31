#include "pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace DirectX;
using namespace Neuron::Math;

namespace
{
    constexpr float EPSILON = 1e-5f;

    void XM_CALLCONV AssertVectorNear(FXMVECTOR expected, FXMVECTOR actual, float eps = EPSILON)
    {
        Assert::AreEqual(XMVectorGetX(expected), XMVectorGetX(actual), eps);
        Assert::AreEqual(XMVectorGetY(expected), XMVectorGetY(actual), eps);
        Assert::AreEqual(XMVectorGetZ(expected), XMVectorGetZ(actual), eps);
    }
}

TEST_CLASS(Transform3DTests)
{
public:

    // --- Identity -----------------------------------------------------------

    TEST_METHOD(Identity_IsIdentity)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        Assert::IsTrue(t.IsIdentity());
    }

    TEST_METHOD(Identity_RightIsUnitX)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        AssertVectorNear(Vector3::UNITX, t.Right());
    }

    TEST_METHOD(Identity_UpIsUnitY)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        AssertVectorNear(Vector3::UNITY, t.Up());
    }

    TEST_METHOD(Identity_ForwardIsUnitZ)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        AssertVectorNear(Vector3::UNITZ, t.Forward());
    }

    TEST_METHOD(Identity_PositionIsZero)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        AssertVectorNear(Vector3::ZERO, t.Position());
    }

    // --- FromAxes -----------------------------------------------------------

    TEST_METHOD(FromAxes_SetsRowsCorrectly)
    {
        XMVECTOR r = Set(1.0f, 0.0f, 0.0f);
        XMVECTOR u = Set(0.0f, 0.0f, 1.0f);
        XMVECTOR f = Set(0.0f, -1.0f, 0.0f);
        XMVECTOR p = Set(10.0f, 20.0f, 30.0f);
        Neuron::Transform3D t = Neuron::Transform3D::FromAxes(r, u, f, p);

        AssertVectorNear(r, t.Right());
        AssertVectorNear(u, t.Up());
        AssertVectorNear(f, t.Forward());
        AssertVectorNear(p, t.Position());
    }

    // --- F3 accessors -------------------------------------------------------

    TEST_METHOD(F3Accessors_MatchVectorAccessors)
    {
        XMVECTOR r = Set(1.0f, 0.0f, 0.0f);
        XMVECTOR u = Set(0.0f, 1.0f, 0.0f);
        XMVECTOR f = Set(0.0f, 0.0f, 1.0f);
        XMVECTOR p = Set(5.0f, 6.0f, 7.0f);
        Neuron::Transform3D t = Neuron::Transform3D::FromAxes(r, u, f, p);

        XMFLOAT3 rf3 = t.RightF3();
        Assert::AreEqual(1.0f, rf3.x, EPSILON);
        Assert::AreEqual(0.0f, rf3.y, EPSILON);
        Assert::AreEqual(0.0f, rf3.z, EPSILON);

        XMFLOAT3 pf3 = t.PositionF3();
        Assert::AreEqual(5.0f, pf3.x, EPSILON);
        Assert::AreEqual(6.0f, pf3.y, EPSILON);
        Assert::AreEqual(7.0f, pf3.z, EPSILON);
    }

    // --- TransformPoint -----------------------------------------------------

    TEST_METHOD(TransformPoint_Identity_Unchanged)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        XMVECTOR point = Set(1.0f, 2.0f, 3.0f);
        AssertVectorNear(point, t.TransformPoint(point));
    }

    TEST_METHOD(TransformPoint_Translation_AddsOffset)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        t.Translate(Set(10.0f, 0.0f, 0.0f));
        // Row-vector convention: (0,0,0) * M → translation row = (10,0,0)
        AssertVectorNear(Set(10.0f, 0.0f, 0.0f), t.TransformPoint(Vector3::ZERO));
    }

    // --- TransformNormal ----------------------------------------------------

    TEST_METHOD(TransformNormal_Identity_Unchanged)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        XMVECTOR normal = Set(0.0f, 1.0f, 0.0f);
        AssertVectorNear(normal, t.TransformNormal(normal));
    }

    TEST_METHOD(TransformNormal_IgnoresTranslation)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        t.Translate(Set(100.0f, 200.0f, 300.0f));
        XMVECTOR normal = Set(0.0f, 1.0f, 0.0f);
        AssertVectorNear(normal, t.TransformNormal(normal));
    }

    // --- operator* ----------------------------------------------------------

    TEST_METHOD(Multiply_IdentityTimesTransform_IsTransform)
    {
        Neuron::Transform3D id = Neuron::Transform3D::Identity();
        XMVECTOR r = Set(0.0f, 0.0f, -1.0f);
        XMVECTOR u = Set(0.0f, 1.0f, 0.0f);
        XMVECTOR f = Set(1.0f, 0.0f, 0.0f);
        XMVECTOR p = Set(5.0f, 0.0f, 0.0f);
        Neuron::Transform3D t = Neuron::Transform3D::FromAxes(r, u, f, p);

        Neuron::Transform3D result = id * t;
        AssertVectorNear(t.Right(), result.Right());
        AssertVectorNear(t.Up(), result.Up());
        AssertVectorNear(t.Forward(), result.Forward());
        AssertVectorNear(t.Position(), result.Position());
    }

    TEST_METHOD(MultiplyAssign_IdentityTimesTransform_IsTransform)
    {
        Neuron::Transform3D id = Neuron::Transform3D::Identity();
        XMVECTOR r = Set(0.0f, 0.0f, -1.0f);
        XMVECTOR u = Set(0.0f, 1.0f, 0.0f);
        XMVECTOR f = Set(1.0f, 0.0f, 0.0f);
        XMVECTOR p = Set(5.0f, 0.0f, 0.0f);
        Neuron::Transform3D t = Neuron::Transform3D::FromAxes(r, u, f, p);

        id *= t;
        AssertVectorNear(t.Right(), id.Right());
        AssertVectorNear(t.Position(), id.Position());
    }

    // --- Orthonormalize -----------------------------------------------------

    TEST_METHOD(Orthonormalize_AlreadyOrthonormal_Unchanged)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        t.Orthonormalize();
        Assert::IsTrue(t.IsIdentity());
    }

    // --- RotateAround -------------------------------------------------------

    TEST_METHOD(RotateAround_YAxis_RotatesOrientation)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        t.Translate(Set(10.0f, 0.0f, 0.0f));

        // Rotate pi/2 around Y: axis = (0, pi/2, 0)
        t.RotateAround(Set(0.0f, XM_PIDIV2, 0.0f));

        // After post-multiplying by RotY(pi/2):
        //   right  (1,0,0) * R → (0,0,-1)
        //   up     (0,1,0) * R → (0,1,0)
        //   forward(0,0,1) * R → (1,0,0)
        AssertVectorNear(Set(0.0f, 0.0f, -1.0f), t.Right());
        AssertVectorNear(Set(0.0f, 1.0f, 0.0f), t.Up());
        AssertVectorNear(Set(1.0f, 0.0f, 0.0f), t.Forward());
        // Position unchanged by RotateAround
        AssertVectorNear(Set(10.0f, 0.0f, 0.0f), t.Position());
    }

    TEST_METHOD(RotateAround_ZeroAxis_NoChange)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        t.RotateAround(Vector3::ZERO);
        Assert::IsTrue(t.IsIdentity());
    }

    // --- Translate ----------------------------------------------------------

    TEST_METHOD(Translate_AddsOffset)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        t.Translate(Set(1.0f, 2.0f, 3.0f));
        AssertVectorNear(Set(1.0f, 2.0f, 3.0f), t.Position());
    }

    TEST_METHOD(Translate_Cumulative)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        t.Translate(Set(1.0f, 0.0f, 0.0f));
        t.Translate(Set(0.0f, 2.0f, 0.0f));
        AssertVectorNear(Set(1.0f, 2.0f, 0.0f), t.Position());
    }

    // --- IsIdentity ---------------------------------------------------------

    TEST_METHOD(IsIdentity_NonIdentity_ReturnsFalse)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        t.Translate(Set(1.0f, 0.0f, 0.0f));
        Assert::IsFalse(t.IsIdentity());
    }

    // --- Conversion ---------------------------------------------------------

    TEST_METHOD(AsXMMATRIX_Identity_IsIdentity)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        Assert::IsTrue(XMMatrixIsIdentity(t.AsXMMATRIX()));
    }

    TEST_METHOD(AsXMFLOAT4X4_ReturnsSameData)
    {
        Neuron::Transform3D t = Neuron::Transform3D::Identity();
        t.Translate(Set(5.0f, 6.0f, 7.0f));
        const XMFLOAT4X4& f = t.AsXMFLOAT4X4();
        Assert::AreEqual(5.0f, f._41, EPSILON);
        Assert::AreEqual(6.0f, f._42, EPSILON);
        Assert::AreEqual(7.0f, f._43, EPSILON);
    }
};

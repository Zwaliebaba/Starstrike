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

TEST_CLASS(GameMathTests)
{
public:

    // --- Set / Component access ---------------------------------------------

    TEST_METHOD(Set_ReturnsCorrectComponents)
    {
        XMVECTOR v = Set(1.0f, 2.0f, 3.0f);
        Assert::AreEqual(1.0f, GetX(v), EPSILON);
        Assert::AreEqual(2.0f, GetY(v), EPSILON);
        Assert::AreEqual(3.0f, GetZ(v), EPSILON);
    }

    // --- Normalize ----------------------------------------------------------

    TEST_METHOD(Normalize_UnitVector_Unchanged)
    {
        AssertVectorNear(Vector3::UNITX, Normalize(Vector3::UNITX));
    }

    TEST_METHOD(Normalize_ScaledVector_BecomesUnit)
    {
        XMVECTOR v = Set(3.0f, 0.0f, 0.0f);
        AssertVectorNear(Vector3::UNITX, Normalize(v));
    }

    TEST_METHOD(Normalize_ArbitraryVector_HasUnitLength)
    {
        XMVECTOR v = Normalize(Set(1.0f, 2.0f, 3.0f));
        Assert::AreEqual(1.0f, Length(v), EPSILON);
    }

    // --- Cross --------------------------------------------------------------

    TEST_METHOD(Cross_XY_ProducesZ)
    {
        AssertVectorNear(Vector3::UNITZ, Cross(Vector3::UNITX, Vector3::UNITY));
    }

    TEST_METHOD(Cross_YZ_ProducesX)
    {
        AssertVectorNear(Vector3::UNITX, Cross(Vector3::UNITY, Vector3::UNITZ));
    }

    TEST_METHOD(Cross_Parallel_ProducesZero)
    {
        XMVECTOR result = Cross(Vector3::UNITX, Vector3::UNITX);
        Assert::AreEqual(0.0f, Length(result), EPSILON);
    }

    // --- Dot ----------------------------------------------------------------

    TEST_METHOD(Dot_Perpendicular_IsZero)
    {
        XMVECTOR result = Dot(Vector3::UNITX, Vector3::UNITY);
        Assert::AreEqual(0.0f, XMVectorGetX(result), EPSILON);
    }

    TEST_METHOD(Dotf_SameUnit_IsOne)
    {
        Assert::AreEqual(1.0f, Dotf(Vector3::UNITX, Vector3::UNITX), EPSILON);
    }

    // --- Length / LengthSquare ----------------------------------------------

    TEST_METHOD(Length_345Triangle)
    {
        Assert::AreEqual(5.0f, Length(Set(3.0f, 4.0f, 0.0f)), EPSILON);
    }

    TEST_METHOD(LengthSquare_345Triangle)
    {
        Assert::AreEqual(25.0f, LengthSquare(Set(3.0f, 4.0f, 0.0f)), EPSILON);
    }

    // --- SetLength ----------------------------------------------------------

    TEST_METHOD(SetLength_ScalesToTarget)
    {
        XMVECTOR v = SetLength(Set(3.0f, 0.0f, 0.0f), 5.0f);
        Assert::AreEqual(5.0f, Length(v), EPSILON);
        AssertVectorNear(Set(5.0f, 0.0f, 0.0f), v);
    }

    // --- RotateAroundX/Y/Z --------------------------------------------------

    TEST_METHOD(RotateAroundY_UnitX_ProducesNegZ)
    {
        XMVECTOR result = RotateAroundY(Vector3::UNITX, XM_PIDIV2);
        AssertVectorNear(Set(0.0f, 0.0f, -1.0f), result);
    }

    TEST_METHOD(RotateAroundX_UnitY_ProducesZ)
    {
        XMVECTOR result = RotateAroundX(Vector3::UNITY, XM_PIDIV2);
        AssertVectorNear(Set(0.0f, 0.0f, 1.0f), result);
    }

    TEST_METHOD(RotateAroundZ_UnitX_ProducesY)
    {
        XMVECTOR result = RotateAroundZ(Vector3::UNITX, XM_PIDIV2);
        AssertVectorNear(Set(0.0f, 1.0f, 0.0f), result);
    }

    // --- RotateAround (axis-angle vector) -----------------------------------

    TEST_METHOD(RotateAround_YAxis_MatchesExplicitRotation)
    {
        // Axis = Y with magnitude pi/2 — same as RotateAroundY by pi/2
        XMVECTOR axis = Set(0.0f, XM_PIDIV2, 0.0f);
        XMVECTOR result = RotateAround(Vector3::UNITX, axis);
        AssertVectorNear(Set(0.0f, 0.0f, -1.0f), result);
    }

    TEST_METHOD(RotateAround_ZeroAxis_ReturnsInput)
    {
        XMVECTOR result = RotateAround(Vector3::UNITX, Vector3::ZERO);
        AssertVectorNear(Vector3::UNITX, result);
    }

    // --- Invert -------------------------------------------------------------

    TEST_METHOD(Invert_Identity_IsIdentity)
    {
        XMMATRIX inv = Invert(XMMatrixIdentity());
        Assert::IsTrue(XMMatrixIsIdentity(inv));
    }

    // --- operator== ---------------------------------------------------------

    TEST_METHOD(OperatorEquals_SameConstant_ReturnsTrue)
    {
        XMVECTOR unitx = Vector3::UNITX;
        Assert::IsTrue(unitx == Vector3::UNITX);
    }

    // --- Vector3 constants --------------------------------------------------

    TEST_METHOD(Vector3Constants_VerifyValues)
    {
        Assert::AreEqual(0.0f, XMVectorGetX(Vector3::ZERO), EPSILON);
        Assert::AreEqual(0.0f, XMVectorGetY(Vector3::ZERO), EPSILON);
        Assert::AreEqual(0.0f, XMVectorGetZ(Vector3::ZERO), EPSILON);

        Assert::AreEqual(1.0f, XMVectorGetX(Vector3::ONE), EPSILON);
        Assert::AreEqual(1.0f, XMVectorGetY(Vector3::ONE), EPSILON);
        Assert::AreEqual(1.0f, XMVectorGetZ(Vector3::ONE), EPSILON);

        Assert::AreEqual(1.0f, XMVectorGetX(Vector3::RIGHT), EPSILON);
        Assert::AreEqual(1.0f, XMVectorGetY(Vector3::UP), EPSILON);
        Assert::AreEqual(1.0f, XMVectorGetZ(Vector3::FORWARD), EPSILON);

        Assert::AreEqual(-1.0f, XMVectorGetX(Vector3::LEFT), EPSILON);
        Assert::AreEqual(-1.0f, XMVectorGetY(Vector3::DOWN), EPSILON);
        Assert::AreEqual(-1.0f, XMVectorGetZ(Vector3::BACKWARD), EPSILON);
    }
};

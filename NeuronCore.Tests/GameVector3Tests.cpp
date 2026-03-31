#include "pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace DirectX;
using namespace Neuron::Math;

TEST_CLASS(GameVector3Tests)
{
public:

    // --- Construction -------------------------------------------------------

    TEST_METHOD(DefaultCtor_IsZero)
    {
        GameVector3 v;
        Assert::AreEqual(0.0f, v.x);
        Assert::AreEqual(0.0f, v.y);
        Assert::AreEqual(0.0f, v.z);
    }

    TEST_METHOD(ParameterizedCtor_SetsComponents)
    {
        GameVector3 v(1.0f, 2.0f, 3.0f);
        Assert::AreEqual(1.0f, v.x);
        Assert::AreEqual(2.0f, v.y);
        Assert::AreEqual(3.0f, v.z);
    }

    TEST_METHOD(XMFLOAT3Ctor_CopiesValue)
    {
        XMFLOAT3 f{4.0f, 5.0f, 6.0f};
        GameVector3 v(f);
        Assert::AreEqual(4.0f, v.x);
        Assert::AreEqual(5.0f, v.y);
        Assert::AreEqual(6.0f, v.z);
    }

    TEST_METHOD(XMVECTORCtor_StoresCorrectly)
    {
        XMVECTOR xv = XMVectorSet(7.0f, 8.0f, 9.0f, 0.0f);
        GameVector3 v(xv);
        constexpr float eps = 1e-6f;
        Assert::AreEqual(7.0f, v.x, eps);
        Assert::AreEqual(8.0f, v.y, eps);
        Assert::AreEqual(9.0f, v.z, eps);
    }

    // --- Load / Store -------------------------------------------------------

    TEST_METHOD(LoadStore_Roundtrip)
    {
        GameVector3 original(1.5f, 2.5f, 3.5f);
        XMVECTOR loaded = original.Load();
        GameVector3 stored(loaded);
        Assert::IsTrue(original == stored);
    }

    TEST_METHOD(Load_ProducesCorrectXMVECTOR)
    {
        GameVector3 v(10.0f, 20.0f, 30.0f);
        XMVECTOR xv = v.Load();
        constexpr float eps = 1e-6f;
        Assert::AreEqual(10.0f, XMVectorGetX(xv), eps);
        Assert::AreEqual(20.0f, XMVectorGetY(xv), eps);
        Assert::AreEqual(30.0f, XMVectorGetZ(xv), eps);
    }

    // --- Mutators -----------------------------------------------------------

    TEST_METHOD(Set_UpdatesComponents)
    {
        GameVector3 v;
        v.Set(10.0f, 20.0f, 30.0f);
        Assert::AreEqual(10.0f, v.x);
        Assert::AreEqual(20.0f, v.y);
        Assert::AreEqual(30.0f, v.z);
    }

    TEST_METHOD(Zero_ClearsComponents)
    {
        GameVector3 v(1.0f, 2.0f, 3.0f);
        v.Zero();
        Assert::AreEqual(0.0f, v.x);
        Assert::AreEqual(0.0f, v.y);
        Assert::AreEqual(0.0f, v.z);
    }

    // --- Comparison ---------------------------------------------------------

    TEST_METHOD(OperatorEquals_SameValues_ReturnsTrue)
    {
        GameVector3 a(1.0f, 2.0f, 3.0f);
        GameVector3 b(1.0f, 2.0f, 3.0f);
        Assert::IsTrue(a == b);
    }

    TEST_METHOD(OperatorEquals_DifferentValues_ReturnsFalse)
    {
        GameVector3 a(1.0f, 2.0f, 3.0f);
        GameVector3 b(1.0f, 2.0f, 4.0f);
        Assert::IsFalse(a == b);
    }

    TEST_METHOD(OperatorNotEquals_DifferentValues_ReturnsTrue)
    {
        GameVector3 a(1.0f, 2.0f, 3.0f);
        GameVector3 b(4.0f, 5.0f, 6.0f);
        Assert::IsTrue(a != b);
    }

    TEST_METHOD(OperatorEquals_WithinTolerance_ReturnsTrue)
    {
        GameVector3 a(1.0f, 2.0f, 3.0f);
        GameVector3 b(1.0f + 5e-7f, 2.0f, 3.0f);  // Within 1e-6f tolerance
        Assert::IsTrue(a == b);
    }

    // --- Data access --------------------------------------------------------

    TEST_METHOD(GetData_ReturnsPointerToComponents)
    {
        GameVector3 v(1.0f, 2.0f, 3.0f);
        float* data = v.GetData();
        Assert::AreEqual(1.0f, data[0]);
        Assert::AreEqual(2.0f, data[1]);
        Assert::AreEqual(3.0f, data[2]);
    }

    TEST_METHOD(GetDataConst_ReturnsConstPointerToComponents)
    {
        const GameVector3 v(1.0f, 2.0f, 3.0f);
        const float* data = v.GetDataConst();
        Assert::AreEqual(1.0f, data[0]);
        Assert::AreEqual(2.0f, data[1]);
        Assert::AreEqual(3.0f, data[2]);
    }
};

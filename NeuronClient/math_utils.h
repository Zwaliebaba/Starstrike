#ifndef MATHUTILS_H
#define MATHUTILS_H

#include "random.h"

class LegacyVector3;
class Vector2;
class Matrix34;
class Plane;

inline float frand(float range = 1.0f) { return range * (static_cast<float>(darwiniaRandom()) / static_cast<float>(DARWINIA_RAND_MAX)); }

inline float sfrand(float range = 1.0f)
{
  return (0.5f - static_cast<float>(darwiniaRandom()) / static_cast<float>((DARWINIA_RAND_MAX))) * range;
}

// Network Syncronised versions (only call inside Net-Safe code)
unsigned long syncrand();
inline float syncfrand(float range = 1.0f) { return syncrand() * (range / 4294967296.0f); }
inline float syncsfrand(float range = 1.0f) { return (syncfrand() - 0.5f) * range; }

#ifndef M_PI
#define M_PI 3.1415926535897932384626f
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define sign(a)				((a) < 0 ? -1 : 1)
#define signf(a)			((a) < 0.0f ? -1.0f : 1.0f)

// Performs fast float to int conversion. It may round up or down, depending on
// the current CPU rounding mode - so DON'T USE IT if you want repeatable results
#define Round(a) (int)(a)

#define clamp(a, low, high)	if (a>high) a = high; else if (a<low) a = low;

#define NearlyEquals(a, b)	(fabsf((a) - (b)) < 1e-6 ? 1 : 0)

#define ASSERT_FLOAT_IS_SANE(x) \
	DEBUG_ASSERT((x + 1.0f) != x);
#define ASSERT_VECTOR3_IS_SANE(v) \
	ASSERT_FLOAT_IS_SANE((v).x) \
	ASSERT_FLOAT_IS_SANE((v).y) \
	ASSERT_FLOAT_IS_SANE((v).z)

// Imagine that you have a stationary object that you want to move from
// A to B in _duration. This function helps you do that with linear acceleration
// to the midpoint, then linear deceleration to the end point. It returns the
// fractional distance along the route. Used in the camera MoveToTarget routine.
double RampUpAndDown(double _startTime, double _duration, double _timeNow);

// **********************
// General Geometry Utils
// **********************

void GetPlaneMatrix(const LegacyVector3& t1, const LegacyVector3& t2, const LegacyVector3& t3, Matrix34* mat);
float ProjectPointOntoPlane(const LegacyVector3& point, const Matrix34& planeMat, LegacyVector3* result);
void ConvertWorldSpaceIntoPlaneSpace(const LegacyVector3& point, const Matrix34& plane, Vector2* result);
void ConvertPlaneSpaceIntoWorldSpace(const Vector2& point, const Matrix34& plane, LegacyVector3* result);
float CalcTriArea(const Vector2& t1, const Vector2& t2, const Vector2& t3);

// *********************
// 2D Intersection Tests
// *********************

bool IsPointInTriangle(const Vector2& pos, const Vector2& t1, const Vector2& t2, const Vector2& t3);
float PointSegDist2D(const Vector2& p, // Point
                     const Vector2& l0, const Vector2& l1, // Line seg
                     Vector2* result = nullptr);
bool SegRayIntersection2D(const Vector2& _lineStart, const Vector2& _lineEnd, const Vector2& _rayStart, const Vector2& _rayDir,
                          Vector2* _result = nullptr);

// *********************
// 3D Intersection Tests
// *********************

float RayRayDist(const LegacyVector3& a, const LegacyVector3& aDir, const LegacyVector3& b, const LegacyVector3& bDir,
                 LegacyVector3* posOnA = nullptr, LegacyVector3* posOnB = nullptr);

float RaySegDist(const LegacyVector3& pointOnLine, const LegacyVector3& lineDir, const LegacyVector3& segStart, const LegacyVector3& segEnd,
                 LegacyVector3* posOnRay = nullptr, LegacyVector3* posInSeg = nullptr);

bool RayTriIntersection(const LegacyVector3& orig, const LegacyVector3& dir, const LegacyVector3& vert0, const LegacyVector3& vert1,
                        const LegacyVector3& vert2, float _rayLen = 1e10, LegacyVector3* result = nullptr);

int RayPlaneIntersection(const LegacyVector3& pOnLine, const LegacyVector3& lineDir, const Plane& plane,
                         LegacyVector3* intersectionPoint = nullptr);
//bool RayPlaneIntersection       (LegacyVector3 const &rayStart, LegacyVector3 const &rayDir,
//                                  LegacyVector3 const &planePos, LegacyVector3 const &planeNormal,
//						         float _rayLen=1e10, LegacyVector3 *pos=NULL );

bool RaySphereIntersection(const LegacyVector3& rayStart, const LegacyVector3& rayDir, const LegacyVector3& spherePos, float sphereRadius,
                           float _rayLen = 1e10, LegacyVector3* pos = nullptr, LegacyVector3* normal = nullptr);

bool SphereSphereIntersection(const LegacyVector3& _sphere1Pos, float _sphere1Radius, const LegacyVector3& _sphere2Pos,
                              float _sphere2Radius);

bool SphereTriangleIntersection(const LegacyVector3& sphereCentre, float sphereRadius, const LegacyVector3& t1, const LegacyVector3& t2,
                                const LegacyVector3& t3);

bool TriTriIntersection(const LegacyVector3& v0, const LegacyVector3& v1, const LegacyVector3& v2, const LegacyVector3& u0,
                        const LegacyVector3& u1, const LegacyVector3& u2);

#endif

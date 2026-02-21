#include "pch.h"

#include "plane.h"
#include "LegacyVector3.h"


// *************
//  Class Plane
// *************

// *** Constructors ***

// Three points
Plane::Plane(LegacyVector3 const &a, LegacyVector3 const &b, LegacyVector3 const &c)
{
	LegacyVector3 vectInPlane1 = a - b;
	LegacyVector3 vectInPlane2 = a - c;
	m_normal = vectInPlane1 ^ vectInPlane2;
	m_normal.Normalise();
	m_distFromOrigin = -m_normal * a;
}


// Normal and Point
Plane::Plane(LegacyVector3 const &normal, LegacyVector3 const &pointInPlane)
:	m_normal(normal)
{
	m_distFromOrigin = normal * pointInPlane;
}


// Normal and distance
Plane::Plane(LegacyVector3 const &normal, float const distFromOrigin)
:	m_normal(normal),
	m_distFromOrigin(distFromOrigin)
{
}


// Returns the cartesian definition of the plane in the form:
//   ax + by + cz + d = 0
void Plane::GetCartesianDefinition(float *a, float *b, float *c, float *d) const
{
	*a = m_normal.x;
	*b = m_normal.y;
	*c = m_normal.z;
	*d = m_distFromOrigin;
}

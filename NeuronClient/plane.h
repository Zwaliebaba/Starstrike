#ifndef INCLUDED_PLANE_H
#define INCLUDED_PLANE_H


#include "LegacyVector3.h"


class Plane
{
public:
	LegacyVector3 m_normal;
	float	m_distFromOrigin;

	Plane(LegacyVector3 const &a, LegacyVector3 const &b, LegacyVector3 const &c);
	Plane(LegacyVector3 const &normal, LegacyVector3 const &pointInPlane);
	Plane(LegacyVector3 const &normal, float const distFromOrigin);
	void GetCartesianDefinition(float *x, float *y, float *z, float *d) const;
};


#endif

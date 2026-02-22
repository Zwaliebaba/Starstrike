#ifndef SPHERE_RENDERER_H
#define SPHERE_RENDERER_H

#include "LegacyVector3.h"


class Triangle
{
public:
	LegacyVector3 m_corner[3];

	Triangle() {}
	Triangle(LegacyVector3 const &c1, LegacyVector3 const &c2, LegacyVector3 const &c3 );
};


class Sphere
{
public:
	Sphere();
	void Render(LegacyVector3 const &pos, float radius);
	void RenderLong();

private:
	Triangle	m_topLevelTriangle[20];
	int m_displayListId;

	void ConsiderTriangle(int level, LegacyVector3 const &a, LegacyVector3 const &b, LegacyVector3 const &c);
};


#endif

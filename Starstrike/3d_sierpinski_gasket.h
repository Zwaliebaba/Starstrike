#pragma once


class Sierpinski3D
{
public:
	std::vector<LegacyVector3>	m_points;

	Sierpinski3D(unsigned int _numPoints);

	void Render(float _scale);
};


#ifndef VECTOR3_H
#define VECTOR3_H


#include "math_utils.h"
#include "vector2.h"

class LegacyVector3;

extern LegacyVector3 const g_upVector;
extern LegacyVector3 const g_zeroVector;


class LegacyVector3
{
private:
	bool Compare(LegacyVector3 const &b) const
	{
		return ( NearlyEquals(x, b.x) &&
				 NearlyEquals(y, b.y) &&
				 NearlyEquals(z, b.z) );
	}

public:
	float x, y, z;

	// *** Constructors ***
	LegacyVector3()
	:	x(0.0f), y(0.0f), z(0.0f)
	{
	}

	LegacyVector3(float _x, float _y, float _z)
	:	x(_x), y(_y), z(_z)
	{
	}

	LegacyVector3(Vector2 const &_b)
	:	x(_b.x), y(0.0f), z(_b.y)
	{
	}

	void Zero()
	{
		x = y = z = 0.0f;
	}

	void Set(float _x, float _y, float _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}

	LegacyVector3 operator ^ (LegacyVector3 const &b) const
	{
		return LegacyVector3(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x);
	}

	float operator * (LegacyVector3 const &b) const
	{
		return x*b.x + y*b.y + z*b.z;
	}

	LegacyVector3 operator - () const
	{
		return LegacyVector3(-x, -y, -z);
	}

	LegacyVector3 operator + (LegacyVector3 const &b) const
	{
		return LegacyVector3(x + b.x, y + b.y, z + b.z);
	}

	LegacyVector3 operator - (LegacyVector3 const &b) const
	{
		return LegacyVector3(x - b.x, y - b.y, z - b.z);
	}

	LegacyVector3 operator * (float const b) const
	{
		return LegacyVector3(x * b, y * b, z * b);
	}

	LegacyVector3 operator / (float const b) const
	{
		float multiplier = 1.0f / b;
		return LegacyVector3(x * multiplier, y * multiplier, z * multiplier);
	}

	LegacyVector3 const &operator = (LegacyVector3 const &b)
	{
		x = b.x;
		y = b.y;
		z = b.z;
		return *this;
	}

	LegacyVector3 const &operator *= (float const b)
	{
		x *= b;
		y *= b;
		z *= b;
		return *this;
	}

	LegacyVector3 const &operator /= (float const b)
	{
		float multiplier = 1.0f / b;
		x *= multiplier;
		y *= multiplier;
		z *= multiplier;
		return *this;
	}

	LegacyVector3 const &operator += (LegacyVector3 const &b)
	{
		x += b.x;
		y += b.y;
		z += b.z;
		return *this;
	}

	LegacyVector3 const &operator -= (LegacyVector3 const &b)
	{
		x -= b.x;
		y -= b.y;
		z -= b.z;
		return *this;
	}

	LegacyVector3 const &Normalise()
	{
		float lenSqrd = x*x + y*y + z*z;
		if (lenSqrd > 0.0f)
		{
			float invLen = 1.0f / sqrtf(lenSqrd);
			x *= invLen;
			y *= invLen;
			z *= invLen;
		}
		else
		{
			x = y = 0.0f;
			z = 1.0f;
		}

		return *this;
	}

	LegacyVector3 const &SetLength(float _len)
	{
		float mag = Mag();
		if (NearlyEquals(mag, 0.0f))
		{
			x = _len;
			return *this;
		}

		float scaler = _len / Mag();
		*this *= scaler;
		return *this;
	}


	bool operator == (LegacyVector3 const &b) const;		// Uses FLT_EPSILON
	bool operator != (LegacyVector3 const &b) const;		// Uses FLT_EPSILON

	void	RotateAroundX	(float _angle);
	void	RotateAroundY	(float _angle);
	void	RotateAroundZ	(float _angle);
	void	FastRotateAround(LegacyVector3 const &_norm, float _angle);
	void	RotateAround	(LegacyVector3 const &_norm);

	float Mag() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	float MagSquared() const
	{
		return x * x + y * y + z * z;
	}

	void HorizontalAndNormalise()
	{
		y = 0.0f;
		float invLength = 1.0f / sqrtf(x * x + z * z);
		x *= invLength;
		z *= invLength;
	}

	float *GetData()
	{
		return &x;
	}

	float const *GetDataConst() const
	{
		return &x;
	}
};


// Operator * between float and LegacyVector3
inline LegacyVector3 operator * (	float _scale, LegacyVector3 const &_v )
{
	return _v * _scale;
}


#endif

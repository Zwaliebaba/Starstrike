#ifndef _INCLUDED_MATRIX34_H
#define _INCLUDED_MATRIX34_H


//#include <string.h>
#include <stdlib.h>

#include "matrix33.h"
#include "LegacyVector3.h"


class Matrix33;


class Matrix34
{
public:
	LegacyVector3 r, u, f, pos;

	// Constructors
	Matrix34() {}

	Matrix34(int _ignored)
	{
		SetToIdentity();
	}


	Matrix34( Matrix34 const &_other )
	{
		memcpy(this, &_other, sizeof(Matrix34));
	}


	Matrix34( Matrix33 const &_or, LegacyVector3 const &_pos )
	:	r(_or.r),
		u(_or.u),
		f(_or.f),
		pos(_pos)
	{
	}


	Matrix34( LegacyVector3 const &_f, LegacyVector3 const &_u, LegacyVector3 const &_pos )
	:	f(_f),
		pos(_pos)
	{
		r = _u ^ f;
		r.Normalise();
		u = f ^ r;
		u.Normalise();
	}


	Matrix34( float _yaw, float _dive, float _roll )
	{
		SetToIdentity();
		RotateAroundZ( _roll );
		RotateAroundX( _dive );
		RotateAroundY( _yaw );
	}

    explicit Matrix34(const Neuron::Transform3D& _t)
	:	r(_t.m._11, _t.m._12, _t.m._13),
		u(_t.m._21, _t.m._22, _t.m._23),
		f(_t.m._31, _t.m._32, _t.m._33),
		pos(_t.m._41, _t.m._42, _t.m._43)
	{
	}

	void OrientRU( LegacyVector3 const & _r, LegacyVector3 const & _u );
	void OrientRF( LegacyVector3 const & _r, LegacyVector3 const & _f );
	void OrientUF( LegacyVector3 const & _u, LegacyVector3 const & _f );
	void OrientUR( LegacyVector3 const & _u, LegacyVector3 const & _r );
	void OrientFR( LegacyVector3 const & _f, LegacyVector3 const & _r );
	void OrientFU( LegacyVector3 const & _f, LegacyVector3 const & _u );

	Matrix34 const &RotateAroundR( float angle );
	Matrix34 const &RotateAroundU( float angle );
	Matrix34 const &RotateAroundF( float angle );
	Matrix34 const &RotateAroundX( float angle );
	Matrix34 const &RotateAroundY( float angle );
	Matrix34 const &RotateAroundZ( float angle );
	Matrix34 const &RotateAround( LegacyVector3 const & _rotationAxis );
	Matrix34 const &RotateAround( LegacyVector3 const & _norm, float _angle );
	Matrix34 const &FastRotateAround( LegacyVector3 const & _norm, float _angle );

	Matrix34 const &Transpose();

	LegacyVector3			DecomposeToYDR	() const;	//x = dive, y = yaw, z = roll
	void			DecomposeToYDR	( float *_y, float *_d, float *_r ) const;
	void			SetToIdentity	();

    bool            IsNormalised    ();
    void			Normalise		();

	DirectX::XMFLOAT4X4 ToXMFLOAT4X4() const
	{
		return DirectX::XMFLOAT4X4(
			r.x,   r.y,   r.z,   0.0f,
			u.x,   u.y,   u.z,   0.0f,
			f.x,   f.y,   f.z,   0.0f,
			pos.x, pos.y, pos.z, 1.0f);
	}

  operator Neuron::Transform3D() const
	{
		return Neuron::Transform3D{ ToXMFLOAT4X4() };
	}

	Matrix33 GetOr() const
	{
		return Matrix33(r, u, f);
	}

	LegacyVector3			InverseMultiplyVector(LegacyVector3 const &) const;
    void            WriteToDebugStream();

    void            Test();

	// Operators
	bool operator == (Matrix34 const &b) const;		// Uses FLT_EPSILON
//	Matrix34 const &operator =  ( Matrix34 const &_o );
//	Matrix34 const &Matrix34::operator = ( Matrix34 const &_o )
//	{
//		memcpy(this, &_o, sizeof(Matrix34));
//		return *this;
//	}


	Matrix34 const &operator *= ( Matrix34 const &_o );
	Matrix34		operator *  ( Matrix34 const &b ) const
	{
		return Matrix34(*this) *= b;
	}
};


extern Matrix34 const g_identityMatrix34;


// Operator * between matrix34 and vector3 — inverse affine transform (column-vector convention)
inline LegacyVector3 operator * ( Matrix34 const &m, LegacyVector3 const &v )
{
	float dx = v.x - m.pos.x;
	float dy = v.y - m.pos.y;
	float dz = v.z - m.pos.z;
	return LegacyVector3(m.r.x*dx + m.r.y*dy + m.r.z*dz,
				   m.u.x*dx + m.u.y*dy + m.u.z*dz,
				   m.f.x*dx + m.f.y*dy + m.f.z*dz);
}


// Operator * between vector3 and matrix34
inline LegacyVector3 operator * ( LegacyVector3 const &v, Matrix34 const &m )
{
	return LegacyVector3(m.r.x*v.x + m.u.x*v.y + m.f.x*v.z + m.pos.x,
    			   m.r.y*v.x + m.u.y*v.y + m.f.y*v.z + m.pos.y,
				   m.r.z*v.x + m.u.z*v.y + m.f.z*v.z + m.pos.z);
}


#endif

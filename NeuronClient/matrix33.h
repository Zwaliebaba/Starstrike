#pragma once


#include "LegacyVector3.h"

class Matrix33
{
public:
	LegacyVector3 r, u, f;

	// Constructors
	Matrix33();
	Matrix33( int _ignored );
	Matrix33( Matrix33 const &_other );
	Matrix33( LegacyVector3 const &_r, LegacyVector3 const &_u, LegacyVector3 const &_f );
	Matrix33( float _yaw, float _dive, float _roll );
	inline Matrix33( float _rx, float _ry, float _rz, float _ux, float _uy, float _uz, float _fx, float _fy, float _fz );

    // Conversion bridge: GameMatrix ↔ Matrix33 (MathPlan Phase 2)
	// Extracts rotation rows; ignores translation.
	Matrix33(const Neuron::Math::GameMatrix& _m)
	:	r(_m.m._11, _m.m._12, _m.m._13),
		u(_m.m._21, _m.m._22, _m.m._23),
		f(_m.m._31, _m.m._32, _m.m._33)
	{
	}

	operator Neuron::Math::GameMatrix() const
	{
		return Neuron::Math::GameMatrix(DirectX::XMFLOAT4X4(
			r.x, r.y, r.z, 0.0f,
			u.x, u.y, u.z, 0.0f,
			f.x, f.y, f.z, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f));
	}

	void OrientRU( LegacyVector3 const & _r, LegacyVector3 const & _u );
	void OrientRF( LegacyVector3 const & _r, LegacyVector3 const & _f );
	void OrientUF( LegacyVector3 const & _u, LegacyVector3 const & _f );
	void OrientUR( LegacyVector3 const & _u, LegacyVector3 const & _r );
	void OrientFR( LegacyVector3 const & _f, LegacyVector3 const & _r );
	void OrientFU( LegacyVector3 const & _f, LegacyVector3 const & _u );
	Matrix33 const &RotateAroundR( float angle );
	Matrix33 const &RotateAroundU( float angle );
	Matrix33 const &RotateAroundF( float angle );
	Matrix33 const &RotateAroundX( float angle );
	Matrix33 const &RotateAroundY( float angle );
	Matrix33 const &RotateAroundZ( float angle );
	Matrix33 const &RotateAround( LegacyVector3 const & _rotationAxis );
	Matrix33 const &RotateAround( LegacyVector3 const & _norm, float _angle );
	Matrix33 const &FastRotateAround( LegacyVector3 const & _norm, float _angle );
	Matrix33 const &Transpose();
	Matrix33 const &Invert();
	LegacyVector3 DecomposeToYDR() const;	//x = dive, y = yaw, z = roll
	void DecomposeToYDR( float *_y, float *_d, float *_r ) const;
	void SetToIdentity();
	void Normalise();

	LegacyVector3			InverseMultiplyVector(LegacyVector3 const &) const;

	void OutputToDebugStream();

	DirectX::XMFLOAT4X4 ToXMFLOAT4X4(LegacyVector3 const *_pos = nullptr) const;

	// Operators
	Matrix33 const &operator =  ( Matrix33 const &_o );
	Matrix33 const &operator *= ( Matrix33 const &_o );
	Matrix33		operator *  (Matrix33 const &b) const;
};


// Operator * between matrix33 and vector3
inline LegacyVector3 operator * ( Matrix33 const &_m, LegacyVector3 const &_v )
{
	return LegacyVector3(_m.r.x * _v.x + _m.r.y * _v.y + _m.r.z * _v.z,
				   _m.u.x * _v.x + _m.u.y * _v.y + _m.u.z * _v.z,
				   _m.f.x * _v.x + _m.f.y * _v.y + _m.f.z * _v.z);
}


// Operator * between vector3 and matrix33 — forward rotation (row-vector convention)
inline LegacyVector3 operator * (	LegacyVector3 const & _v, Matrix33 const &_m )
{
	return LegacyVector3(_v.x * _m.r.x + _v.y * _m.u.x + _v.z * _m.f.x,
				   _v.x * _m.r.y + _v.y * _m.u.y + _v.z * _m.f.y,
				   _v.x * _m.r.z + _v.y * _m.u.z + _v.z * _m.f.z);
}


 #ifndef INCLUDED_ENTITY_LEG_H
#define INCLUDED_ENTITY_LEG_H


#include "LegacyVector3.h"

class Entity;
class ShapeMarker;
class Shape;


//*****************************************************************************
// Class EntityFoot
//*****************************************************************************

class EntityFoot
{
public:
	enum FootState
	{
		OnGround,
		Swinging,
		Pouncing
	};

	LegacyVector3			m_pos;
	LegacyVector3			m_targetPos;
	FootState		m_state;
	float			m_leftGroundTimeStamp;
	LegacyVector3			m_lastGroundPos;
	LegacyVector3			m_bodyToFoot;			// Used whilst pouncing
};


//*****************************************************************************
// Class EntityLeg
//*****************************************************************************

class EntityLeg
{
public:
	float			m_legLift;				// How high to lift the foot (in metres)
	float			m_idealLegSlope;		// 0.0 means vertical 1.0 means 45 degrees.
	float			m_legSwingDuration;
	float			m_lookAheadCoef;		// Amount of velocity to add on to foot home pos to calc foot target pos

	int				m_legNum;
	EntityFoot		m_foot;
	ShapeMarker		*m_rootMarker;
	Shape			*m_shapeUpper;
    Shape           *m_shapeLower;
	float			m_thighLen;
	float			m_shinLen;
	Entity			*m_parent;

protected:
	LegacyVector3 GetLegRootPos();
	LegacyVector3 CalcFootHomePos(float _targetHoverHeight);
	LegacyVector3 CalcDesiredFootPos(float _targetHoverHeight);
	LegacyVector3 CalcKneePos(LegacyVector3 const &_footPos, LegacyVector3 const &_rootPos, LegacyVector3 const &_centrePos);
	LegacyVector3	GetIdealSwingingFootPos(float _fractionComplete);

public:
	EntityLeg(int _legNum, Entity *_parent,
                char const *_shapeNameUpper,
                char const *_shapeNameLower,
                char const *_rootMarkerName);

	float	CalcFootsDesireToMove(float _targetHoverHeight);
	void	LiftFoot(float _targetHoverHeight);
	void	PlantFoot();

	bool	Advance(); // Returns true if the foot was planted this frame
	void	AdvanceSpiderPounce(float _fractionComplete);
	void	Render(float _predictionTime, LegacyVector3 const &_predictedMovement);
	bool	RenderPixelEffect(float _predictionTime, LegacyVector3 const &_predictedMovement);
};


#endif

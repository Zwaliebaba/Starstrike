#ifndef INCLUDED_ROUTING_SYSTEM_H
#define INCLUDED_ROUTING_SYSTEM_H


#include "llist.h"
#include "LegacyVector3.h"


// ****************************************************************************
// Class WayPoint
// ****************************************************************************

class WayPoint
{
protected:
	LegacyVector3				m_pos;

public:
	enum
	{
		Type3DPos,
		TypeGroundPos,
		TypeBuilding
	};

	int					m_type;
	int					m_buildingId;

	WayPoint			(int _type, LegacyVector3 const &_pos);
	~WayPoint			();

	LegacyVector3				GetPos();
	void				SetPos(LegacyVector3 const &_pos);
};



// ****************************************************************************
// Class Route
// ****************************************************************************

class Route
{
public:
	int			m_id;
	LList       <WayPoint *>m_wayPoints;

	Route		(int _id);
	~Route		();

    void        AddWayPoint             (LegacyVector3 const &_pos);
    void        AddWayPoint             (int _buildingId);
    WayPoint    *GetWayPoint            (int _id);

	int			GetIdOfNearestWayPoint  (LegacyVector3 const &_pos);
	int			GetIdOfNearestEdge      (LegacyVector3 const &_pos, float *_dist);

    void        Render();
};


#endif

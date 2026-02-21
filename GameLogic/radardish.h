#ifndef _included_radardish_h
#define _included_radardish_h

#define RADARDISH_TRANSPORTPERIOD    0.1f                        // Minimum wait time between sends
#define RADARDISH_TRANSPORTSPEED     50.0f                       // Speed of in-transit entities (m/s)

#include "teleport.h"


class RadarDish : public Teleport
{
protected:
	ShapeFragment *m_dish;
	ShapeFragment *m_upperMount;
	ShapeMarker	  *m_focusMarker;

    LegacyVector3     m_entrancePos;
    LegacyVector3     m_entranceFront;

    LegacyVector3     m_target;
    int         m_receiverId;
    float       m_range;
    float       m_signal;

    bool        m_newlyCreated;

    bool        m_horizontallyAligned;
    bool        m_verticallyAligned;
    bool        m_movementSoundsPlaying;

    LegacyVector3     GetDishPos      ( float _predictionTime );      // Returns the position of the transmission point
    LegacyVector3     GetDishFront    ( float _predictionTime );      // Returns the front vector of the dish

    void RenderSignal   ( float _predictionTime, float _radius, float _alpha );

public:
    RadarDish();
	~RadarDish();

    void SetDetail      ( int _detail );

    bool Advance        ();
    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );

    void Aim            ( LegacyVector3 _worldPos );

    bool    Connected       ();
    bool    ReadyToSend     ();

    int     GetConnectedDishId();

    LegacyVector3 GetStartPoint   ();
    LegacyVector3 GetEndPoint     ();
    bool    GetEntrance     ( LegacyVector3 &_pos, LegacyVector3 &_front );
    bool    GetExit         ( LegacyVector3 &_pos, LegacyVector3 &_front );

    bool    DoesSphereHit   (LegacyVector3 const &_pos, float _radius);

    bool    UpdateEntityInTransit( Entity *_entity );

    void    ListSoundEvents ( LList<char *> *_list );
};



#endif

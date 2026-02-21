
#ifndef _included_staticshape_h
#define _included_staticshape_h

#include "building.h"


class StaticShape : public Building
{
public:
    char    m_shapeName[256];
    float   m_scale;

public:
    StaticShape();

    void Initialise     ( Building *_template );
    void SetDetail      ( int _detail );

    void SetShapeName   ( char *_shapeName );
    void SetStringId    ( char *_stringId );

    bool Advance();
    void Render( float _predictionTime );

    bool DoesSphereHit          (LegacyVector3 const &_pos, float _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (LegacyVector3 const &_rayStart, LegacyVector3 const &_rayDir,
                                 float _rayLen=1e10, LegacyVector3 *_pos=NULL, LegacyVector3 *_norm=NULL);        // pos/norm will not always be available

    void Read( TextReader *_in, bool _dynamic );
    void Write( FileWriter *_out );
};


#endif
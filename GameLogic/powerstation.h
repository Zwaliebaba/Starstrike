#pragma once

#include "building.h"


class ShapeStatic;
class ShapeFragmentData;
class ShapeMarkerData;


class Powerstation : public Building
{
protected:
    int					m_linkedBuildingId;

public:
    Powerstation		();

	void Initialise		(Building *_template);

    bool Advance		();

    int  GetBuildingLink();
    void SetBuildingLink(int _buildingId);

	void Read( TextReader *_in, bool _dynamic );
	void Write( FileWriter *out );
};


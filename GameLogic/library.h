
#pragma once

#include "building.h"

#include "global_world.h"



class Library : public Building
{
public:
    bool m_scrollSpawned[GlobalResearch::NumResearchItems];

public:
    Library();

    bool Advance();
};


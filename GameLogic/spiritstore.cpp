#include "pch.h"

#include "math_utils.h"
#include "profiler.h"
#include "resource.h"

#include "GameContext.h"

#include "spiritstore.h"


SpiritStore::SpiritStore()
:   m_sizeX(0.0f),
    m_sizeY(0.0f),
    m_sizeZ(0.0f)
{
}

void SpiritStore::Initialise ( int _initialCapacity, int _maxCapacity, LegacyVector3 _pos,
                                float _sizeX, float _sizeY, float _sizeZ )
{
    m_spirits.SetSize( _maxCapacity );
    m_spirits.SetStepSize( _maxCapacity / 2 );
    m_pos = _pos;
    m_sizeX = _sizeX;
    m_sizeY = _sizeY;
    m_sizeZ = _sizeZ;

    for( int i = 0; i < _initialCapacity; ++i )
    {
        Spirit s;
        s.m_teamId = 0;
        s.Begin();
        AddSpirit( &s );
    }
}

void SpiritStore::Advance()
{
    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            Spirit *s = m_spirits.GetPointer(i);
            s->Advance();
        }
	}
}


int SpiritStore::NumSpirits ()
{
    return m_spirits.NumUsed();
}


void SpiritStore::AddSpirit ( Spirit *_spirit )
{
    Spirit *target = m_spirits.GetPointer();
    *target = *_spirit;
    target->m_pos = m_pos + LegacyVector3(syncsfrand(m_sizeX*1.5f),
                                    syncsfrand(m_sizeY*1.5f),
                                    syncsfrand(m_sizeZ*1.5f) );
    target->m_state = Spirit::StateInStore;
    target->m_numNearbyEggs = 0;
}

void SpiritStore::RemoveSpirits ( int _quantity )
{
    int numRemoved = 0;
    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            m_spirits.MarkNotUsed(i);
            ++numRemoved;
            if( numRemoved == _quantity ) return;
        }
    }
}

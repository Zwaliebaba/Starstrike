#include "pch.h"
#include "resource.h"
#include "ShapeStatic.h"


#include "library.h"
#include "researchitem.h"

#include "GameContext.h"
#include "location.h"


Library::Library()
:   Building()
{
    m_type = Building::TypeLibrary;
    SetShape( Resource::GetShapeStatic( "library.shp" ) );

    memset( m_scrollSpawned, 0, GlobalResearch::NumResearchItems * sizeof(bool) );
}


bool Library::Advance()
{
    for( int i = 0; i < GlobalResearch::NumResearchItems; ++i )
    {
        if( !m_scrollSpawned[i] &&
            g_context->m_globalWorld->m_research->HasResearch(i) )
        {
            char markerName[256];
            snprintf( markerName, sizeof(markerName), "MarkerResearch%02d", i+1 );
            ShapeMarkerData *scrollMarker = m_shape->GetMarkerData( markerName );
            DEBUG_ASSERT( scrollMarker );

            Matrix34 rootMat(m_front, g_upVector, m_pos);
            Matrix34 scrollPos = m_shape->GetMarkerWorldMatrix(scrollMarker, rootMat);

            ResearchItem *item = new ResearchItem();
            item->m_researchType = i;
            item->m_inLibrary = true;
            item->m_pos = scrollPos.pos;
            item->m_id.SetUniqueId( g_context->m_globalWorld->GenerateBuildingId() );
            g_context->m_location->m_buildings.PutData( item );

            m_scrollSpawned[i] = true;
        }
    }

    return Building::Advance();
}



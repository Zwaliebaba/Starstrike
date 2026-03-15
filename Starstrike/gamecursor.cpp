#include "pch.h"
#include "bitmap.h"
#include "targetcursor.h"
#include "math_utils.h"
#include "resource.h"
#include "profiler.h"
#include "hi_res_time.h"

#include "binary_stream_readers.h"

#include "eclipse.h"

#include "GameApp.h"
#include "global_world.h"
#include "main.h"
#include "gamecursor.h"
#include "gamecursor_2d.h"
#include "renderer.h"
#include "user_input.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "location.h"
#include "team.h"
#include "unit.h"
#include "camera.h"
#include "location_input.h"
#include "radardish.h"
#include "insertion_squad.h"


GameCursor::GameCursor()
:	m_selectionArrowBoost(0.0f),
	m_wasOnScreenLastFrame(true)
{
	m_cursorPlacement = new MouseCursor( "icons/mouse_placement.bmp" );
	m_cursorPlacement->SetHotspot(0.5f, 0.5f);
	m_cursorPlacement->SetSize(40.0f);

	m_cursorDisabled = new MouseCursor( "icons/mouse_disabled.bmp" );
	m_cursorDisabled->SetHotspot(0.5f, 0.5f);
	m_cursorDisabled->SetSize(60.0f);
	m_cursorDisabled->SetColour(RGBAColour(255,0,0,255) );

	m_cursorMoveHere = new MouseCursor( "icons/mouse_movehere.bmp" );
	m_cursorMoveHere->SetHotspot( 0.5f, 0.5f);
	m_cursorMoveHere->SetSize(30.0f);
	m_cursorMoveHere->SetAnimation( true );
	m_cursorMoveHere->SetColour(RGBAColour(255,255,150,255) );

	m_cursorHighlight = new MouseCursor( "icons/mouse_highlight.bmp" );
	m_cursorHighlight->SetHotspot( 0.5f, 0.5f );
	m_cursorHighlight->SetAnimation( true );
}

GameCursor::~GameCursor()
{
	SAFE_DELETE(m_cursorPlacement);
	SAFE_DELETE(m_cursorDisabled);
	SAFE_DELETE(m_cursorMoveHere);
	SAFE_DELETE(m_cursorHighlight);
}

bool GameCursor::GetSelectedObject( WorldObjectId &_id, LegacyVector3 &_pos )
{
    Team *team = g_app->m_location->GetMyTeam();

    if( team )
    {
        Entity *selectedEnt = team->GetMyEntity();
        if( selectedEnt )
        {
            _pos = selectedEnt->m_pos + selectedEnt->m_centerPos + selectedEnt->m_vel * g_predictionTime;
            _id = selectedEnt->m_id;
            return true;
        }
        else if( team->m_currentBuildingId != -1 )
        {
            Building *building = g_app->m_location->GetBuilding( team->m_currentBuildingId );
            if( building )
            {
                _pos = building->m_centerPos;
                _id = building->m_id;
                return true;
            }
        }
        else
        {
            Unit *selected = team->GetMyUnit();
            if( selected )
            {
                _pos = selected->m_centerPos + selected->m_vel * g_predictionTime;
                _id.Set( selected->m_teamId, selected->m_unitId, -1, -1 );

                // Add the center pos
                for( int i = 0; i < selected->m_entities.Size(); ++i )
                {
                    if( selected->m_entities.ValidIndex(i) )
                    {
                        Entity *ent = selected->m_entities[i];
                        _pos += ent->m_centerPos;
                        break;
                    }
                }

                return true;
            }
        }
    }

    return false;
}


bool GameCursor::GetHighlightedObject( WorldObjectId &_id, LegacyVector3 &_pos, float &_radius )
{
    WorldObjectId id;
    bool somethingHighlighted = false;
    bool found = false;

    if( !g_app->m_taskManagerInterface->m_visible )
    {
        somethingHighlighted = g_app->m_locationInput->GetObjectUnderMouse( id, g_app->m_globalWorld->m_myTeamId );
    }
    else
    {
        Task *task = g_app->m_taskManager->GetTask( g_app->m_taskManagerInterface->m_highlightedTaskId );
        if( task && task->m_objId.IsValid() )
        {
            id = task->m_objId;
            somethingHighlighted = true;
        }
    }


    if( somethingHighlighted )
    {
        if( id.GetUnitId() == UNIT_BUILDINGS )
        {
            // Found a building
            Building *building = g_app->m_location->GetBuilding( id.GetUniqueId() );
            if( building->m_type == Building::TypeRadarDish ||
                building->m_type == Building::TypeBridge ||
                building->m_type == Building::TypeGunTurret ||
                building->m_type == Building::TypeFenceSwitch )
            {
                _id = id;
                _pos = building->m_centerPos;
                _radius = building->m_radius;
                found = true;
                if( building->m_type == Building::TypeGunTurret )
                {
                    _pos.y += _radius * 0.5f;
                }
            }
        }
        else if( id.GetIndex() == -1 )
        {
            // Found a unit
            Unit *unit = g_app->m_location->GetUnit( id );
            if( unit )
            {
                _id = id;
                _pos = unit->m_centerPos + unit->m_vel * g_predictionTime;
                _radius = unit->m_radius;
                found = true;

                // Add the center pos
                for( int i = 0; i < unit->m_entities.Size(); ++i )
                {
                    if( unit->m_entities.ValidIndex(i) )
                    {
                        Entity *ent = unit->m_entities[i];
                        _pos += ent->m_centerPos;
                        break;
                    }
                }
            }
        }
        else
        {
            // Found an entity
            Entity *entity = g_app->m_location->GetEntity(id);
            _id = id;
            _pos = entity->m_pos + entity->m_vel * g_predictionTime + entity->m_centerPos;
            _radius = entity->m_radius*1.5f;
            if( entity->m_type == Entity::TypeDarwinian ) _radius = entity->m_radius*2.0f;
            found = true;
        }
    }

    return found;
}


void GameCursor::CreateMarker( LegacyVector3 const &_pos )
{
    LegacyVector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(_pos.x, _pos.z);
    LegacyVector3 front = (landNormal ^ g_upVector).Normalise();

    MouseCursorMarker *marker = new MouseCursorMarker();
    marker->m_pos = _pos;
    marker->m_front = front;
    marker->m_up = landNormal;
    marker->m_startTime = GetHighResTime();

    m_markers.PutData( marker );
}


void GameCursor::BoostSelectionArrows( float _seconds )
{
	m_selectionArrowBoost = max( m_selectionArrowBoost, _seconds );
}


void GameCursor::RenderMarkers()
{
    for( int i = 0; i < m_markers.Size(); ++i )
    {
        MouseCursorMarker *marker = m_markers[i];
        float timeSync = GetHighResTime() - marker->m_startTime;
        if( timeSync > 0.5f )
        {
            m_markers.RemoveData(i);
            delete marker;
            --i;
        }
        else
        {
            m_cursorPlacement->SetSize( 20.0f - timeSync * 40.0f );
            m_cursorPlacement->SetAnimation( false );
            m_cursorPlacement->Render3D( marker->m_pos, marker->m_front, marker->m_up, false );
        }
    }
}


void GameCursor::PrepareCursorState()
{
	START_PROFILE( g_app->m_profiler, "Prepare GameCursor" );

	m_frame = {};

	// Input state
	m_frame.screenX = g_target->X();
	m_frame.screenY = g_target->Y();
	m_frame.mousePos = g_app->m_userInput->GetMousePos3d();
	m_frame.mousePos.y = max( 1.0f, m_frame.mousePos.y );

	// Context flags
	m_frame.isEditing = g_app->m_editing || EclGetWindows()->Size() > 0;
	m_frame.isInteractive = g_app->m_camera->IsInteractive();
	m_frame.inLocation = g_app->m_location != nullptr;
	m_frame.inGlobalWorld = g_app->m_locationId == -1;

	// Mode-based suppressions
	if( !m_frame.isEditing && !m_frame.isInteractive )
	{
		// Cut scene or non-interactive camera: suppress standard cursor
		m_frame.suppressStandardCursor = true;
	}

	// Early-out: if editing or non-interactive, no further queries needed
	if( m_frame.isEditing || !m_frame.isInteractive )
	{
		END_PROFILE( g_app->m_profiler, "Prepare GameCursor" );
		return;
	}

	// Global world: no selection arrows or screen projections needed
	if( !m_frame.inLocation )
	{
     // Query highlighted location while perspective matrices are active.
		// GetHighlightedLocation calls GetClickRay which reads the GL
		// projection stack via gluUnProject — must run before the 2D pass
		// overwrites it with ortho.
		GlobalLocation *highlightedLocation = g_app->m_globalWorld->GetHighlightedLocation();
		m_frame.globalWorldLocAvailable = highlightedLocation &&
										  highlightedLocation->m_missionFilename != "null" &&
										  highlightedLocation->m_available;

		// The global world path always renders a placement cursor; suppress the
		// default standard cursor so it doesn't draw on top.
		m_frame.suppressStandardCursor = true;

		END_PROFILE( g_app->m_profiler, "Prepare GameCursor" );
		return;
	}

	// ---- Location-specific queries ----

	Task *task = g_app->m_taskManager->GetCurrentTask();
	m_frame.currentTask = task;
	if( task ) m_frame.taskType = task->m_type;

	m_frame.somethingSelected = GetSelectedObject( m_frame.selectedId, m_frame.selectedWorldPos );
	m_frame.somethingHighlighted = GetHighlightedObject( m_frame.highlightedId, m_frame.highlightedWorldPos, m_frame.highlightedRadius );

	// Entity track mode suppression
	if( g_app->m_camera->IsInMode( Camera::ModeEntityTrack ) &&
		!g_app->m_taskManagerInterface->m_visible &&
		!( task && task->m_state == Task::StateStarted &&
		   task->m_type != GlobalResearch::TypeOfficer &&
		   !m_frame.somethingHighlighted ) )
	{
		m_frame.suppressStandardCursor = true;
	}

	// ---- Pre-compute screen projections (while perspective matrices are active) ----

	int screenH = g_app->m_renderer->ScreenH();
	int screenW = g_app->m_renderer->ScreenW();

	// Selected object screen position (for selection arrows)
	if( m_frame.somethingSelected && m_frame.selectedId.GetUnitId() != UNIT_BUILDINGS )
	{
		// Entity/unit alive checks (moved from RenderSelectionArrows draw path)
		Entity *ent = g_app->m_location->GetEntity( m_frame.selectedId );
		Unit *unit = g_app->m_location->GetUnit( m_frame.selectedId );
		bool alive = (ent || unit);
		if( ent && ent->m_dead ) alive = false;
		if( unit && unit->NumAliveEntities() == 0 ) alive = false;
		m_frame.selectionArrowsValid = alive;

		if( alive )
		{
			float rawScreenX, rawScreenY;
			g_app->m_camera->Get2DScreenPos( m_frame.selectedWorldPos, &rawScreenX, &rawScreenY );
			// Flip Y to match screen coordinates (Get2DScreenPos returns GL-convention Y)
			m_frame.selectedScreenX = rawScreenX;
			m_frame.selectedScreenY = screenH - rawScreenY;

			// Determine if on-screen
			LegacyVector3 toCam = g_app->m_camera->GetPos() - m_frame.selectedWorldPos;
			float angle = toCam * g_app->m_camera->GetFront();

			m_frame.selectedOnScreen = ( angle <= 0.0f &&
										 m_frame.selectedScreenX >= 0 && m_frame.selectedScreenX < screenW &&
										 m_frame.selectedScreenY >= 0 && m_frame.selectedScreenY < screenH );

			// Off→on screen transition detection (was static bool onScreen in RenderSelectionArrows)
			if( m_frame.selectedOnScreen && !m_wasOnScreenLastFrame )
			{
				BoostSelectionArrows( 2.0f );
			}
			m_wasOnScreenLastFrame = m_frame.selectedOnScreen;

			// Pre-compute off-screen edge data
			if( !m_frame.selectedOnScreen )
			{
				LegacyVector3 camPos = g_app->m_camera->GetPos() + g_app->m_camera->GetFront() * 1000;
				LegacyVector3 camToTarget = ( m_frame.selectedWorldPos - camPos ).SetLength( 100 );

				float camX = screenW / 2.0f;
				float camY = screenH / 2.0f;
				float posX, posY;
				g_app->m_camera->Get2DScreenPos( camPos + camToTarget, &posX, &posY );

				Vector2 lineNormal( posX - camX, posY - camY );
				lineNormal.Normalise();

				FindScreenEdge( lineNormal, &m_frame.selectedEdgeX, &m_frame.selectedEdgeY );

				lineNormal.x *= -1;
				m_frame.selectedEdgeNormal = lineNormal;
			}
		}
	}

	// Highlighted object screen position (for halo cursor)
	if( m_frame.somethingHighlighted )
	{
		g_app->m_camera->Get2DScreenPos( m_frame.highlightedWorldPos, &m_frame.highlightedScreenX, &m_frame.highlightedScreenY );
		m_frame.highlightedOnScreen = true;

		// Pre-compute camera distance for halo sizing
		m_frame.highlightedCamDist = ( g_app->m_camera->GetPos() - m_frame.highlightedWorldPos ).Mag();
	}

	// Radar dish entrance screen position
	if( m_frame.somethingSelected && m_frame.somethingHighlighted &&
		m_frame.selectedId.GetUnitId() != UNIT_BUILDINGS &&
		m_frame.highlightedId.GetUnitId() == UNIT_BUILDINGS )
	{
		int entityType = Entity::TypeInvalid;
		if( m_frame.selectedId.GetIndex() == -1 )
			entityType = g_app->m_location->GetUnit( m_frame.selectedId )->m_troopType;
		else
			entityType = g_app->m_location->GetEntity( m_frame.selectedId )->m_type;

		if( entityType == Entity::TypeInsertionSquadie || entityType == Entity::TypeOfficer )
		{
			Building *building = g_app->m_location->GetBuilding( m_frame.highlightedId.GetUniqueId() );
			if( building && building->m_type == Building::TypeRadarDish )
			{
				RadarDish *dish = (RadarDish *) building;
				if( dish->Connected() )
				{
					LegacyVector3 entrancePos, entranceFront;
					dish->GetEntrance( entrancePos, entranceFront );
					float posX, posY;
					g_app->m_camera->Get2DScreenPos( entrancePos, &posX, &posY );
					m_frame.radarDishEntranceValid = true;
					m_frame.radarDishEntranceScreenX = posX;
					m_frame.radarDishEntranceScreenY = posY;
				}
			}
		}
	}

	// Copy selection arrow boost into frame state
	m_frame.selectionArrowBoost = m_selectionArrowBoost;

	END_PROFILE( g_app->m_profiler, "Prepare GameCursor" );
}


void GameCursor::Render3D()
{
	START_PROFILE( g_app->m_profiler, "Render GameCursor 3D" );

	float nearPlaneStart = g_app->m_renderer->GetNearPlane();
	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.05f,
										   g_app->m_renderer->GetFarPlane());

	// Set mip mapping for game cursor (3D pass)
	glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	if( m_frame.isEditing || !m_frame.isInteractive || !m_frame.inLocation )
	{
		// No 3D cursors needed in editing, cut-scene, or global world modes
	}
	else
	{
		if( m_frame.currentTask &&
			m_frame.currentTask->m_state == Task::StateStarted &&
			m_frame.currentTask->m_type != GlobalResearch::TypeOfficer &&
			!m_frame.somethingHighlighted &&
			!g_app->m_taskManagerInterface->m_visible )
		{
			// The player is placing a task
			bool validPlacement = g_app->m_taskManager->IsValidTargetArea( m_frame.currentTask->m_id, m_frame.mousePos );
			m_cursorPlacement->SetAnimation( validPlacement );
			m_cursorPlacement->SetSize( 40.0f );
			LegacyVector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue( m_frame.mousePos.x, m_frame.mousePos.z );
			LegacyVector3 front = (landNormal ^ g_upVector).Normalise();
			m_cursorPlacement->Render3D( m_frame.mousePos, front, landNormal );
			if( !validPlacement )
				m_cursorDisabled->Render3D( m_frame.mousePos, front, landNormal );

			m_frame.suppressStandardCursor = true;
			m_frame.cursorRendered3D = true;
		}
		else if( !g_app->m_taskManagerInterface->m_visible &&
				 !g_app->m_camera->IsInMode( Camera::ModeEntityTrack ) )
		{
			if( m_frame.somethingSelected && m_frame.selectedId.GetUnitId() != UNIT_BUILDINGS )
			{
				// Selected a unit OR an entity — move-here marker on terrain
				if( !m_frame.somethingHighlighted || m_frame.highlightedId.GetUnitId() == UNIT_BUILDINGS )
				{
					LegacyVector3 targetFront = (m_frame.mousePos - m_frame.selectedWorldPos).Normalise();
					LegacyVector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue( m_frame.mousePos.x, m_frame.mousePos.z );
					LegacyVector3 targetRight = (targetFront ^ landNormal).Normalise();
					targetFront = (targetRight ^ landNormal).Normalise();
					m_cursorMoveHere->SetSize( 30.0f );
					m_cursorMoveHere->Render3D( m_frame.mousePos, targetFront, landNormal );
					m_frame.suppressStandardCursor = true;
					m_frame.cursorRendered3D = true;
				}
			}

			if( !m_frame.somethingSelected && !m_frame.somethingHighlighted )
			{
				// Looking at empty landscape
				LegacyVector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue( m_frame.mousePos.x, m_frame.mousePos.z );
				LegacyVector3 front = (landNormal ^ g_upVector).Normalise();
				m_cursorHighlight->SetAnimation( false );
				m_cursorHighlight->SetSize( 30.0f );
				m_cursorHighlight->Render3D( m_frame.mousePos, front, landNormal );
				m_frame.suppressStandardCursor = true;
				m_frame.cursorRendered3D = true;
			}
		}
	}

	RenderMarkers();

	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart,
										   g_app->m_renderer->GetFarPlane());

	END_PROFILE( g_app->m_profiler, "Render GameCursor 3D" );
}


void GameCursor::WriteBackArrowBoost( float _boost )
{
	m_selectionArrowBoost = _boost;
}


void GameCursor::RenderWeaponMarker ( LegacyVector3 _pos, LegacyVector3 _front, LegacyVector3 _up )
{
	m_cursorPlacement->SetSize(40.0f);
	m_cursorPlacement->SetShadowed( true );
	m_cursorPlacement->SetAnimation( true );
	m_cursorPlacement->Render3D( _pos, _front, _up );
}


void GameCursor::FindScreenEdge( Vector2 const &_line, float *_posX, float *_posY )
{
//    y = mx + c
//    c = y - mx
//    x = (y - c) / m

	int screenH = g_app->m_renderer->ScreenH();
	int screenW = g_app->m_renderer->ScreenW();

	float m = _line.y / _line.x;
	float c = ( screenH / 2.0f ) - m * ( screenW / 2.0f );

	if( _line.y < 0 )
	{
		// Intersect with top view plane
		float x = ( 0 - c ) / m;
		if( x >= 0 && x <= screenW )
		{
			*_posX = x;
			*_posY = 0;
			return;
		}
	}
	else
	{
		// Intersect with the bottom view plane
		float x = ( screenH - c ) / m;
		if( x >= 0 && x <= screenW )
		{
			*_posX = x;
			*_posY = screenH;
			return;
		}
	}

	if( _line.x < 0 )
	{
		// Intersect with left view plane
		float y = m * 0 + c;
		if( y >= 0 && y <= screenH )
		{
			*_posX = 0;
			*_posY = y;
			return;
		}
	}
	else
	{
		// Intersect with right view plane
		float y = m * screenW + c;
		if( y >= 0 && y <= screenH )
		{
			*_posX = screenW;
			*_posY = y;
			return;
		}
	}

	// We should never ever get here
	DEBUG_ASSERT( false );
	*_posX = 0;
	*_posY = 0;
}



// ****************************************************************************
//  Class MouseCursor
// ****************************************************************************

MouseCursor::MouseCursor( char const *_filename )
:   m_hotspotX(0.0f),
    m_hotspotY(0.0f),
    m_size(20.0f),
    m_animating(false),
    m_shadowed(true)
{
    char fullFilename[512];
	snprintf( fullFilename, sizeof(fullFilename), "%s", _filename);
	m_mainFilename = strdup(fullFilename);

    BinaryReader *binReader = g_app->m_resource->GetBinaryReader( m_mainFilename );
	ASSERT_TEXT(binReader, "Failed to open mouse cursor resource %s", _filename);
    BitmapRGBA bmp( binReader, "bmp" );
	SAFE_DELETE(binReader);

	g_app->m_resource->AddBitmap(m_mainFilename, bmp);

	snprintf(fullFilename, sizeof(fullFilename), "shadow_%s", _filename);
	m_shadowFilename = strdup(fullFilename);
    bmp.ApplyBlurFilter( 10.0f );
	g_app->m_resource->AddBitmap(m_shadowFilename, bmp);

    m_colour.Set(255,255,255,255);
}

MouseCursor::~MouseCursor()
{
	free(m_mainFilename);
	free(m_shadowFilename);
}

float MouseCursor::GetSize()
{
	float size = m_size;

	if (m_animating)
	{
		size += fabs(sinf(g_gameTime * 4.0f)) * size * 0.6f;
	}

	return size;
}


void MouseCursor::SetSize(float _size)
{
	m_size = _size;
}


void MouseCursor::SetAnimation(bool _onOrOff)
{
	m_animating = _onOrOff;
}


void MouseCursor::SetShadowed(bool _onOrOff)
{
    m_shadowed = _onOrOff;
}


void MouseCursor::SetHotspot(float x, float y)
{
	m_hotspotX = x;
	m_hotspotY = y;
}


void MouseCursor::SetColour(const RGBAColour &_col )
{
    m_colour = _col;
}


void MouseCursor::Render(float _x, float _y)
{
	float s = GetSize();
	float x = (float)_x - s * m_hotspotX;
	float y = (float)_y - s * m_hotspotY;

	glEnable        (GL_TEXTURE_2D);
	glEnable        (GL_BLEND);
    glDisable       (GL_CULL_FACE);
    glDepthMask     (false);

    if( m_shadowed )
    {
		glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(m_shadowFilename));
	    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
        glColor4f       ( 1.0f, 1.0f, 1.0f, 0.0f );
	    glBegin         (GL_QUADS);
		    glTexCoord2i(0,1);			glVertex2f(x, y);
		    glTexCoord2i(1,1);			glVertex2f(x + s, y);
		    glTexCoord2i(1,0);			glVertex2f(x + s, y + s);
		    glTexCoord2i(0,0);			glVertex2f(x, y + s);
	    glEnd();
    }

    glColor4ubv     (m_colour.GetData() );
	glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(m_mainFilename));
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE);
	glBegin         (GL_QUADS);
		glTexCoord2i(0,1);			glVertex2f(x, y);
		glTexCoord2i(1,1);			glVertex2f(x + s, y);
		glTexCoord2i(1,0);			glVertex2f(x + s, y + s);
		glTexCoord2i(0,0);			glVertex2f(x, y + s);
	glEnd();

    glDepthMask     (true);
    glDisable       (GL_BLEND);
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_CULL_FACE);
}


void MouseCursor::Render3D( LegacyVector3 const &_pos, LegacyVector3 const &_front, LegacyVector3 const &_up, bool _cameraScale)
{
    LegacyVector3 rightAngle = (_front ^ _up).Normalise();

	glEnable        (GL_TEXTURE_2D);

	glEnable        (GL_BLEND);
    glDisable       (GL_CULL_FACE );
    //glDisable       (GL_DEPTH_TEST );
    glDepthMask     (false);

    float scale = GetSize();
    if( _cameraScale )
    {
        float camDist = ( g_app->m_camera->GetPos() - _pos ).Mag();
        scale *= sqrtf(camDist) / 40.0f;
    }

    LegacyVector3 pos = _pos;
    pos -= m_hotspotX * rightAngle * scale;
    pos += m_hotspotY * _front * scale;

    if( m_shadowed )
    {
		glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(m_shadowFilename));
	    glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
        glColor4f       (1.0f, 1.0f, 1.0f, 0.0f);

        glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex3fv( (pos).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (pos + rightAngle * scale).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (pos - _front * scale + rightAngle * scale).GetData() );
            glTexCoord2i(0,0);      glVertex3fv( (pos - _front * scale).GetData() );
        glEnd();
    }

    glColor4ubv     (m_colour.GetData());
	glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(m_mainFilename));
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE);

    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex3fv( (pos).GetData() );
        glTexCoord2i(1,1);      glVertex3fv( (pos + rightAngle * scale).GetData() );
        glTexCoord2i(1,0);      glVertex3fv( (pos - _front * scale + rightAngle * scale).GetData() );
        glTexCoord2i(0,0);      glVertex3fv( (pos - _front * scale).GetData() );
    glEnd();

    glDepthMask     (true);
    glEnable        (GL_DEPTH_TEST );
    glDisable       (GL_BLEND);
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_CULL_FACE);
}


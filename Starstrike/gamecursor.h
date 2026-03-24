#pragma once

/*
    Renders all mouse cursors of any kind in-game.
    Responsible for figuring out which mouse cursor to render.
    Also renders selection arrows to inform the player which units are selected

 */

#include "llist.h"
#include "rgb_colour.h"
#include "worldobject.h"

// ============================================================================

class MouseCursor
{
public:
    MouseCursor( char const *_filename );
    ~MouseCursor();

    float GetSize();
    void  SetSize       ( float _size );
    void  SetAnimation  ( bool _onOrOff );
    void  SetShadowed   ( bool _onOrOff );
    void  SetHotspot    ( float x, float y );
    void  SetColour     ( const RGBAColour &_col );

    void  Render        ( float _x, float _y );
    void  Render3D      ( LegacyVector3 const &_pos, LegacyVector3 const &_front, LegacyVector3 const &_up, bool _cameraScale = true );

private:
    char  *m_mainFilename;
    char  *m_shadowFilename;
    float  m_hotspotX;
    float  m_hotspotY;
    float  m_size;
    bool   m_animating;
    bool   m_shadowed;
    RGBAColour m_colour;
};

class MouseCursorMarker;
class Task;


// ============================================================================

struct CursorFrameState
{
    // Input state
    int screenX = 0, screenY = 0;
    LegacyVector3 mousePos;

    // Gameplay queries (filled by PrepareCursorState)
    bool somethingSelected = false;
    bool somethingHighlighted = false;
    WorldObjectId selectedId, highlightedId;
    LegacyVector3 selectedWorldPos, highlightedWorldPos;
    float highlightedRadius = 0.0f;
    bool isEditing = false;
    bool isInteractive = false;
    bool inLocation = false;
    bool inGlobalWorld = false;
    // Global world: whether the cursor is over an available, playable location.
    // Computed during PrepareCursorState (3D pass) because GetHighlightedLocation
    // calls GetClickRay which reads the perspective projection matrix.
    bool globalWorldLocAvailable = false;
    int taskType = 0;
    Task* currentTask = nullptr;

    // Pre-computed screen projections (computed during 3D pass while
    // perspective matrices are active — Get2DScreenPos reads the current
    // GL projection/modelview stacks, so it must run before the 2D pass
    // overwrites them with ortho).
    float selectedScreenX = 0.0f, selectedScreenY = 0.0f;
    bool selectedOnScreen = false;
    float highlightedScreenX = 0.0f, highlightedScreenY = 0.0f;
    bool highlightedOnScreen = false;
    // Off-screen edge data for FindScreenEdge
    float selectedEdgeX = 0.0f, selectedEdgeY = 0.0f;
    Vector2 selectedEdgeNormal;
    float highlightedEdgeX = 0.0f, highlightedEdgeY = 0.0f;
    Vector2 highlightedEdgeNormal;
    // Radar dish entrance (only valid when radarDishEntranceValid == true).
    // This is a third world position (from dish->GetEntrance()) that is neither
    // selectedWorldPos nor highlightedWorldPos.  Must be projected via
    // Get2DScreenPos during the 3D pass.
    bool radarDishEntranceValid = false;
    float radarDishEntranceScreenX = 0.0f, radarDishEntranceScreenY = 0.0f;
    // Pre-computed camera distance for highlighted-entity halo sizing.
    // Used as: highlightedRadius * 100 / sqrt(highlightedCamDist).
    float highlightedCamDist = 0.0f;
    // Selection arrow alive checks (moved from RenderSelectionArrows draw path
    // to avoid gameplay queries inside GameCursor2D::Render).
    bool selectionArrowsValid = false;

    // Selection arrow state (written by GameCursor, read + updated by GameCursor2D)
    float selectionArrowBoost = 0.0f;

    // Cross-pass flags
    bool suppressStandardCursor = false;
    bool cursorRendered3D = false;
};


// ============================================================================


class GameCursor
{
protected:
    MouseCursor *m_cursorPlacement;
    MouseCursor *m_cursorHighlight;
    MouseCursor *m_cursorMoveHere;
    MouseCursor *m_cursorDisabled;

	float		m_selectionArrowBoost;

    CursorFrameState m_frame;
    bool        m_wasOnScreenLastFrame;

    bool GetSelectedObject      ( WorldObjectId &_id, LegacyVector3 &_pos );
    bool GetHighlightedObject   ( WorldObjectId &_id, LegacyVector3 &_pos, float &_radius );

    void RenderMarkers          ();
    void FindScreenEdge         ( Vector2 const &_line, float *_posX, float *_posY );

    void RenderWeaponMarker     ( LegacyVector3 _pos, LegacyVector3 _front, LegacyVector3 _up );

    LList<MouseCursorMarker *>  m_markers;

public:
    GameCursor();
	~GameCursor();

    void CreateMarker           ( LegacyVector3 const &_pos );
	void BoostSelectionArrows	( float _seconds );
    void PrepareCursorState     ();
    void Render3D               ();

    const CursorFrameState& GetFrameState() const { return m_frame; }
    void WriteBackArrowBoost    ( float _boost );

    // Shared cursor accessors — GameCursor2D reads these for 2D rendering of
    // cursors that are used in both 2D and 3D paths (placement, disabled).
    MouseCursor* GetCursorPlacement() const { return m_cursorPlacement; }
    MouseCursor* GetCursorDisabled()  const { return m_cursorDisabled; }
};


// ============================================================================

class MouseCursorMarker
{
public:
    LegacyVector3     m_pos;
    LegacyVector3     m_front;
    LegacyVector3     m_up;
    float       m_startTime;
};


// ============================================================================


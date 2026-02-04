#ifndef _included_gamecursor_h
#define _included_gamecursor_h

/*
    Renders all mouse cursors of any kind in-game.
    Responsible for figuring out which mouse cursor to render.
    Also renders selection arrows to inform the player which units are selected

 */

#include "llist.h"
#include "rgb_colour.h"
#include "worldobject.h"

class MouseCursor;
class MouseCursorMarker;

// ============================================================================

class GameCursor
{
  public:
    GameCursor();
    ~GameCursor();

    void CreateMarker(const LegacyVector3& _pos);
    void BoostSelectionArrows(float _seconds);	// Show selection arrows for a specified time
    void Render();

    void RenderStandardCursor(float _screenX, float _screenY);
    void RenderWeaponMarker(LegacyVector3 _pos, LegacyVector3 _front, LegacyVector3 _up);

    bool AdviseHighlightingSomething() { return m_highlightingSomething; }
    bool AdvisePlacementOpportunity() { return m_validPlacementOpportunity; }
    bool AdviseMoveableEntitySelected() { return m_moveableEntitySelected; }

  protected:
    MouseCursor* m_cursorStandard;
    MouseCursor* m_cursorPlacement;
    MouseCursor* m_cursorHighlight;
    MouseCursor* m_cursorSelection;
    MouseCursor* m_cursorMoveHere;
    MouseCursor* m_cursorDisabled;
    MouseCursor* m_cursorMissile;
    MouseCursor* m_cursorTurretTarget;

    char m_selectionArrowFilename[256];
    char m_selectionArrowShadowFilename[256];
    float m_selectionArrowBoost;

    bool m_highlightingSomething;
    bool m_validPlacementOpportunity;
    bool m_moveableEntitySelected;

    bool GetSelectedObject(WorldObjectId& _id, LegacyVector3& _pos);
    bool GetHighlightedObject(WorldObjectId& _id, LegacyVector3& _pos, float& _radius);

    void RenderSelectionArrows(WorldObjectId _id, const LegacyVector3& _pos);
    void RenderSelectionArrow(float _screenX, float _screenY, float _screenDX, float _screenDY, float _size, float _alpha);

    void RenderMarkers();
    void FindScreenEdge(const LegacyVector2& _line, float* _posX, float* _posY);

    LList<MouseCursorMarker*> m_markers;
};

// ============================================================================

class MouseCursorMarker
{
  public:
    LegacyVector3 m_pos;
    LegacyVector3 m_front;
    LegacyVector3 m_up;
    float m_startTime;
};

// ============================================================================

class MouseCursor
{
  protected:
    char* m_mainFilename;
    char* m_shadowFilename;

    float m_size;
    bool m_animating;
    bool m_shadowed;
    float m_hotspotX;		// 0,0 means hotspot is in top left of bitmap
    float m_hotspotY;		// 1,1 means hotspot is in bottom right of bitmap
    RGBAColor m_colour;

    float GetSize();

  public:
    MouseCursor(const char* _filename);
    ~MouseCursor();

    void SetSize(float _size);
    void SetAnimation(bool _onOrOff);
    void SetShadowed(bool _onOrOff);
    void SetHotspot(float x, float y);
    void SetColor(const RGBAColor& _col);

    void Render(float _x, float _y);
    void Render3D(const LegacyVector3& _pos, const LegacyVector3& _front, const LegacyVector3& _up, bool _cameraScale = true);
};

#endif

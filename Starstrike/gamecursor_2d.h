#pragma once

/*
    Screen-space (2D) cursor rendering — selection halos, selection arrows,
    turret crosshair, standard arrow, global-world placement cursor.

    Reads pre-computed per-frame state from GameCursor via CursorFrameState.
    All rendering assumes the caller has already set up an orthographic
    projection and the 2D GL state contract (depth off, blend on, cull off).
*/

class GameCursor;
class MouseCursor;

class GameCursor2D
{
public:
    explicit GameCursor2D( GameCursor& _parent );
    ~GameCursor2D();

    void Render();
    void RenderStandardCursor( float _screenX, float _screenY );

    // Returns the updated selectionArrowBoost after decrement during Render().
    // Caller writes this back to GameCursor via WriteBackArrowBoost().
    float GetUpdatedArrowBoost() const { return m_updatedArrowBoost; }

private:
    void RenderSelectionArrows();
    void RenderSelectionArrow( float _screenX, float _screenY, float _screenDX, float _screenDY, float _size, float _alpha );

    GameCursor& m_parent;

    MouseCursor* m_cursorStandard;
    MouseCursor* m_cursorSelection;
    MouseCursor* m_cursorTurretTarget;

    char m_selectionArrowFilename[256];
    char m_selectionArrowShadowFilename[256];

    float m_updatedArrowBoost;
};

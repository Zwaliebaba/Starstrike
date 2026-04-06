#pragma once

class Flag;

// Rendering helper for the Flag cloth object.
// Flag is not a WorldObject — it is a visual helper owned by Armour and
// Officer entities.  This class extracts the GL rendering (and cloth
// advance step) out of GameLogic so that flag.cpp compiles headless.
class FlagRenderer
{
public:
    static void Render(Flag& _flag);
    static void RenderText(Flag& _flag, int _posX, int _posY, const char* _caption);
};

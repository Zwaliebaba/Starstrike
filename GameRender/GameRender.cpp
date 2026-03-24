#include "pch.h"

#include "GameRender.h"

// Forward declarations for category init functions (defined in separate translation units).
void InitBuildingRenderers();
void InitEntityRenderers();
void InitWorldObjectRenderers();

void InitGameRenderers()
{
    InitBuildingRenderers();
    InitEntityRenderers();
    InitWorldObjectRenderers();
}

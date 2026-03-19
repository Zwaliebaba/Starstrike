// GameLogic/GameLogicPlatform.h
//
// Conditional include shim for GameLogic.
// Client builds define GAMELOGIC_HAS_RENDERER and get the full
// NeuronClient header set (GL translation layer, shapes, renderer,
// resources).  Server / headless builds omit that define and compile
// against NeuronCore only, using forward declarations for the
// rendering types that GameLogic headers still reference as pointers
// during the transition period (Phases 2-4 will eliminate these).

#pragma once

#include "NeuronCore.h"

// Forward declarations for rendering types that GameLogic headers
// reference as pointer / reference types.  These match the actual
// types defined in NeuronClient/ShapeStatic.h.
//
// Safe in the client build: forward declarations before a full
// definition are a no-op once the complete type is seen via
// NeuronClient.h below.
class ShapeStatic;
class ShapeFragmentData;
class ShapeMarkerData;

#if defined(GAMELOGIC_HAS_RENDERER)
// Client build -- full NeuronClient headers available (GL translation
// layer, shapes, renderer, resources).  Preserves current behaviour.
#include "NeuronClient.h"
#else
// Server / headless build -- no rendering headers.
// Phase 2+ will eliminate remaining rendering dependencies from
// GameLogic source files, making this branch compilable.
#endif

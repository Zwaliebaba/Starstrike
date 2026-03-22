// GameLogic/GameLogicPlatform.h
//
// Platform include shim for GameLogic.
// GameLogic compiles against NeuronCore only.  Forward declarations
// cover rendering types that headers still reference as pointers.

#pragma once

#include "NeuronCore.h"

#if defined(GAMELOGIC_HAS_RENDERER)
// Client build — full rendering headers available.
#include "NeuronClient.h"
#endif

// Forward declarations for rendering types that GameLogic headers
// reference as pointer / reference types.  These match the actual
// types defined in NeuronClient/ShapeStatic.h.
class ShapeStatic;
class ShapeFragmentData;
class ShapeMarkerData;

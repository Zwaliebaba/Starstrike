#pragma once

#include "SimEventQueue.h" // NeuronCore generic template
#include "GameSimEvent.h"

using GameSimEventQueue = SimEventQueue<GameSimEvent, 1024>;

extern GameSimEventQueue g_simEventQueue;

#ifndef INCLUDED_MAIN_H
#define INCLUDED_MAIN_H

#include "NetworkTypes.h"
#include "NetworkPackets.h"
#include "entity.h"

extern double g_gameTime;					  // Updated from GetHighResTime every frame
extern double g_startTime;
extern float g_advanceTime;                // How long the last frame took
extern double g_lastServerAdvance;          // Time of last server advance
extern float g_predictionTime;             // Time between last server advance and start of render
extern float g_targetFrameRate;
extern uint32_t g_lastServerTick;          // Last server tick received
#ifdef TARGET_OS_VISTA
extern char     g_saveFile[128];       // The profile name extracted from the save file that was used to launch darwinia
extern bool		g_mediaCenter;			// Darwinia is being run from Media Center
#endif
extern bool IsRunningVista();

// Network command helpers
void SendCreateUnitCommand(uint8_t teamId, EntityType entityType, int count, int buildingId, const LegacyVector3& pos);
void SendSelectUnitCommand(uint8_t teamId, NetEntityId entityId);
void SendAimBuildingCommand(uint8_t teamId, int buildingId, const LegacyVector3& pos);
void SendToggleFenceCommand(int buildingId);
void SendRunProgramCommand(uint8_t teamId, uint8_t programId);
void SendTargetProgramCommand(uint8_t teamId, uint8_t programId, const LegacyVector3& pos);

void AppMain();

#endif

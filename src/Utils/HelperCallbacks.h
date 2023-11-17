#pragma once

#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Packets/StoC.h>

#include "ActionTypes.h"

bool ExplorableLoadCallback();

bool OnChatMessagePlayerReady(GW::HookStatus *, GW::Packet::StoC::PacketBase *packet, const TriggerRole trigger_role);

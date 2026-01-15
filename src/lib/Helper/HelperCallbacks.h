#pragma once

#include "stl_includes.h"

#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Packets/StoC.h>

#include "ActionTypes.h"

bool IsExplorableInstance();

bool OnChatMessagePlayerReady(GW::HookStatus *, GW::Packet::StoC::PacketBase *packet, const TriggerRole trigger_role);

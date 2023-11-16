#include <cstdint>

#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/CtoSMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Packets/Opcodes.h>

#include "HelperMaps.h"

#include "Helper.h"

bool HelperActivationConditions(const bool need_party_loaded)
{
    if (!IsMapReady())
        return false;

    if (need_party_loaded && !GW::PartyMgr::GetIsPartyLoaded())
        return false;

    if (!IsMapReady())
        return false;

    return true;
}

uint32_t QuestAcceptDialog(GW::Constants::QuestID quest)
{
    return static_cast<int>(quest) << 8 | 0x800001;
}

uint32_t QuestRewardDialog(GW::Constants::QuestID quest)
{
    return static_cast<int>(quest) << 8 | 0x800007;
}

void CancelMovement()
{
    GW::CtoS::SendPacket(0x4, GAME_CMSG_CANCEL_MOVEMENT);
}

void AttackAgent(const GW::Agent *agent)
{
    if (!agent)
        return;
    GW::CtoS::SendPacket(0xC, GAME_CMSG_ATTACK_AGENT, agent->agent_id, 0);
}

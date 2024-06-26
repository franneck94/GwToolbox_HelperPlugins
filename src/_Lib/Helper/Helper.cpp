#include <cstdint>

#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Managers/UIMgr.h>

#include "Helper.h"
#include "HelperDialogs.h"
#include "HelperMaps.h"

bool HelperActivationConditions(const bool need_party_loaded)
{
    if (!IsMapReady())
        return false;

    if (need_party_loaded && !GW::PartyMgr::GetIsPartyLoaded())
        return false;

    return true;
}

void CancelMovement()
{
    GW::UI::Keypress(GW::UI::ControlAction_MoveBackward); // TODO - former: GAME_CMSG_CANCEL_MOVEMENT
}

void AttackAgent(const GW::Agent *agent)
{
    if (!agent)
        return;

    GW::Agents::ChangeTarget(agent->agent_id);
    GW::UI::Keypress(GW::UI::ControlAction_Interact);
}

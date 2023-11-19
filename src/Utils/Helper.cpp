#include <cstdint>

#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/CtoSMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/MemoryMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Packets/Opcodes.h>

#include "Helper.h"
#include "HelperMaps.h"
#include "HelperQuests.h"

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

void SendDialog(const wchar_t *, const int argc, const LPWSTR *argv)
{
    const auto IsMapReady = [] {
        return GW::Map::GetInstanceType() != GW::Constants::InstanceType::Loading && !GW::Map::GetIsObserving() &&
               GW::MemoryMgr::GetGWWindowHandle() == GetActiveWindow();
    };
    const auto ParseUInt = [](const wchar_t *str, unsigned int *val, const int base = 0) {
        wchar_t *end;
        if (!str)
        {
            return false;
        }
        *val = wcstoul(str, &end, base);
        if (str == end || errno == ERANGE)
        {
            return false;
        }
        return true;
    };
    if (!IsMapReady())
    {
        return;
    }
    if (argc <= 1)
    {
        return;
    }
    uint32_t id = 0;
    if (!(ParseUInt(argv[1], &id) && id))
    {
        return;
    }
    GW::GameThread::Enqueue([id] { GW::CtoS::SendPacket(0x8, GAME_CMSG_SEND_DIALOG, id); });
}

#pragma once

#include <array>
#include <cstdint>

#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Packets/StoC.h>
#include <GWCA/Utilities/Hook.h>

#include "ActionsBase.h"
#include "HelperPlayer.h"

#include "UwMetadata.h"

void UwMetadata::Initialize()
{
    GW::StoC::RegisterPacketCallback(&SendChat_Entry,
                                     GAME_SMSG_CHAT_MESSAGE_LOCAL,
                                     [this](GW::HookStatus *status, GW::Packet::StoC::PacketBase *packet) -> void {
                                         lt_is_ready = OnChatMessagePlayerReady(status, packet, TriggerRole::LT);
                                         emo_is_ready = OnChatMessagePlayerReady(status, packet, TriggerRole::EMO);
                                         db_is_ready = OnChatMessagePlayerReady(status, packet, TriggerRole::DB);
                                     });

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::MapLoaded>(
        &MapLoaded_Entry,
        [this](GW::HookStatus *, GW::Packet::StoC::MapLoaded *) -> void {
            load_cb_triggered = IsExplorableInstance();
            num_finished_objectives = 0U;
        });

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::ObjectiveDone>(
        &ObjectiveDone_Entry,
        [this](GW::HookStatus *, GW::Packet::StoC::ObjectiveDone *) { ++num_finished_objectives; });
}

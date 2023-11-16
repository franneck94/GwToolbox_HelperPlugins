#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Context/WorldContext.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/CtoSMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Packets/Opcodes.h>
#include <GWCA/Packets/StoC.h>
#include <GWCA/Utilities/Hook.h>

#include <ActionsBase.h>
#include <HelperUw.h>
#include <Logger.h>

#include "HelperCallbacks.h"

bool ExplorableLoadCallback(GW::HookStatus *, GW::Packet::StoC::MapLoaded *)
{
    switch (GW::Map::GetInstanceType())
    {
    case GW::Constants::InstanceType::Explorable:
        return true;
        break;
    case GW::Constants::InstanceType::Outpost:
    case GW::Constants::InstanceType::Loading:
    default:
        return false;
        break;
    }
}

static GW::Array<wchar_t> *GetMessageBuffer()
{
    const auto w = GW::WorldContext::instance();
    return w && w->message_buff.valid() ? &w->message_buff : nullptr;
}

static const wchar_t *GetMessageCore()
{
    auto *buf = GetMessageBuffer();
    return buf ? buf->begin() : nullptr;
}

static bool ClearMessageCore()
{
    auto *buf = GetMessageBuffer();
    if (!buf)
        return false;
    buf->clear();
    return true;
}

static std::wstring DataToUtf8String(const wchar_t *message)
{
    const wchar_t *start = nullptr;
    const wchar_t *end = nullptr;
    size_t i = 0;
    while (start == nullptr && message[i])
    {
        if (message[i] == 0x107)
            start = &message[i + 1];
        i++;
    }
    while (!end && message[i])
    {
        if (message[i] == 0x1)
            end = &message[i];
        i++;
    }
    if (!start)
        start = &message[0];
    if (!end)
        end = &message[i];

    return std::wstring(start, end);
}

bool OnChatMessagePlayerReady(GW::HookStatus *, GW::Packet::StoC::PacketBase *packet, const TriggerRole trigger_role)
{
    const wchar_t *message = nullptr;
    uint32_t channel = 0;
    uint32_t player_number = 0;

    switch (packet->header)
    {
    case GAME_SMSG_CHAT_MESSAGE_LOCAL:
    {
        const auto p = static_cast<GW::Packet::StoC::MessageLocal *>(packet);
        channel = p->channel;
        player_number = p->player_number;
        message = GetMessageCore();
        break;
    }
    default:
        break;
    }

    if (!message || !channel || static_cast<uint32_t>(GW::Chat::Channel::CHANNEL_GROUP) != channel)
        return false;

    const auto trigger_id = GetUwTriggerRoleId(trigger_role);
    if (!trigger_id)
        return false;
    const auto trigger_agent = GW::Agents::GetAgentByID(trigger_id);
    if (!trigger_agent)
        return false;

    const auto trigger_living = trigger_agent->GetAsAgentLiving();
    if (!trigger_living || trigger_living->player_number != player_number)
        return false;

    const auto message_utf8 = DataToUtf8String(message);

    if (message_utf8.size() == 2 && message_utf8[0] == 33025 && message_utf8[1] == 32644)
    {
        Log::Info("HM ping by: %d", player_number);
        return true;
    }

    return false;
}

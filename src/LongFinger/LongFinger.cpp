#include "LongFinger.h"

#include <GWCA/GWCA.h>

#include <GWCA/Packets/Opcodes.h>
#include <GWCA/Utilities/Hooker.h>

#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/CtoSMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>


DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static LongFinger instance;
    return &instance;
}

void LongFinger::Initialize(ImGuiContext* ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxPlugin::Initialize(ctx, fns, toolbox_dll);

    GW::Initialize();

    GW::Chat::CreateCommand(L"longfinger", [this](const wchar_t*, int, LPWSTR*) {
        GW::GameThread::Enqueue([this] {
            const auto target = GW::Agents::GetTarget();
            if (target && target->GetIsGadgetType()) {
                bool res = true;
                res = res && GW::CtoS::SendPacket(0xC, GAME_CMSG_INTERACT_GADGET, target->agent_id, 0); // GW::Agents::GoSignpost(target);
                res = res && GW::CtoS::SendPacket(0x8, GAME_CMSG_SEND_SIGNPOST_DIALOG, 0x2);            // GW::Items::OpenLockedChest();
                if (!res) {
                    WriteChat(GW::Chat::CHANNEL_GWCA1, L"Failed to open target locked chest", L"LongFinger");
                }
            }
            else {
                WriteChat(GW::Chat::CHANNEL_GWCA1, L"Target is not a locked chest", L"LongFinger");
            }
        });
    });
    WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized\nUse /longfinger to open locked chests from afar.", L"LongFinger");
}

void LongFinger::SignalTerminate()
{
    GW::Chat::DeleteCommand(L"longfinger");
    GW::DisableHooks();
}

bool LongFinger::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void LongFinger::Terminate()
{
    ToolboxPlugin::Terminate();
    GW::Terminate();
}

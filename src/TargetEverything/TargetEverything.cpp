#include "TargetEverything.h"

#include <GWCA/GWCA.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <ToolboxPlugin.h>

#include <GWCA/Utilities/Hooker.h>
#include <GWCA/Utilities/Scanner.h>

using GetIsAgentTargettableFn = decltype(&GW::Agents::GetIsAgentTargettable);
GetIsAgentTargettableFn GetIsAgentTargettable_Func = nullptr;
GetIsAgentTargettableFn RetGetIsAgentTargettable = nullptr;

bool GetIsAgentTargettableOverride(const GW::Agent* agent)
{
    return agent != nullptr;
}

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static TargetEverything instance;
    return &instance;
}

void TargetEverything::Initialize(ImGuiContext* ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxPlugin::Initialize(ctx, fns, toolbox_dll);

    GW::Initialize();
    GW::Scanner::Initialize(toolbox_dll);
    GetIsAgentTargettable_Func = reinterpret_cast<GetIsAgentTargettableFn>(GW::Scanner::Find("\x55\x8B\xEC\x8B\x45\x08\x85\xC0\x74\x0\xF6\x80\x9C\x00\x00\x00\x0\x74", "xxxxxxxxx?xxxxxx?x"));
    if (!GetIsAgentTargettable_Func) {
        // DEBUG toolbox?
        GetIsAgentTargettable_Func = reinterpret_cast<GetIsAgentTargettableFn>(
            GW::Scanner::Find("\x55\x8B\xEC\x83\xEC\x0\xC7\x45\xF8\x0\x0\x0\x0\xC7\x45\xFC\x0\x0\x0\x0\x83\x7D\x08\x0\x75\x0\x32\xC0", "xxxxx?xxx????xxx????xxx?x?xx")
        );
    }
    if (!GetIsAgentTargettable_Func) {
        WriteChat(GW::Chat::CHANNEL_GWCA1, L"Failed to initialize", L"TargetEverything");
        return;
    }
    GW::HookBase::CreateHook(GetIsAgentTargettable_Func, GetIsAgentTargettableOverride, reinterpret_cast<void**>(&RetGetIsAgentTargettable));
    GW::HookBase::EnableHooks(GetIsAgentTargettable_Func);
    GW::Scanner::Initialize();
    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"TargetEverything");
}

void TargetEverything::SignalTerminate()
{
    ToolboxPlugin::SignalTerminate();
    GW::DisableHooks();
}

bool TargetEverything::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void TargetEverything::Terminate()
{
    ToolboxPlugin::Terminate();
    GW::Terminate();
}

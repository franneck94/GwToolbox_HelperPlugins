#include <map>
#include <vector>

#include "AllowAllDialogs.h"

#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/CtoSMgr.h>
#include <GWCA/Utilities/Hooker.h>
#include <GWCA/Utilities/Scanner.h>

namespace
{
using DialogCallback = GW::HookCallback<uint32_t>;

uint32_t last_dialog_id = 0;
std::vector<std::unordered_map<GW::HookEntry *, GW::CtoS::PacketCallback>> packets_callbacks;
std::unordered_map<GW::HookEntry *, DialogCallback> OnDialog_callbacks;

using SendDialog_pt = bool(__cdecl *)(unsigned int dialog_id);
using SendPacket_pt = void(__cdecl *)(uint32_t context, uint32_t size, void *packet);

SendDialog_pt SendDialog_Func = nullptr;
SendDialog_pt SendDialog_Ret = nullptr;

SendPacket_pt SendPacket_Func = 0;
SendPacket_pt SendPacket_Ret = 0;

bool OnIsDialogAvailable(unsigned int)
{
    return true;
}

bool CtoSHandler_Func(uint32_t context, uint32_t size, void *packet)
{
    GW::HookBase::EnterHook();
    GW::HookStatus status;
    uint32_t header = *(uint32_t *)packet;
    if (header < packets_callbacks.size())
    {
        for (auto &it : packets_callbacks[header])
        {
            it.second(&status, packet);
            ++status.altitude;
        }
    }
    if (!status.blocked)
    {
        SendPacket_Ret(context, size, packet);
    }
    GW::HookBase::LeaveHook();
    return true;
}
} // namespace

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static AllowAllDialogs instance;
    return &instance;
}

void AllowAllDialogs::SignalTerminate()
{
    if (SendDialog_Func)
    {
        GW::HookBase::DisableHooks(SendDialog_Func);
    }
    if (SendPacket_Func)
    {
        GW::HookBase::DisableHooks(SendPacket_Func);
    }
}

bool AllowAllDialogs::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void AllowAllDialogs::Terminate()
{
    ToolboxPlugin::Terminate();
    GW::HookBase::RemoveHook(SendDialog_Func);
    GW::HookBase::RemoveHook(SendPacket_Func);
    GW::HookBase::Deinitialize();
}

void AllowAllDialogs::Initialize(ImGuiContext *ctx, ImGuiAllocFns fns, HMODULE toolbox_dll)
{
    ToolboxPlugin::Initialize(ctx, fns, toolbox_dll);
    GW::HookBase::Initialize();
    GW::Scanner::Initialize(toolbox_dll);
    GW::Scanner::Initialize();

    // DIALOG STUFF
    uintptr_t address = GW::Scanner::Find("\x89\x4b\x24\x8b\x4b\x28\x83\xe9\x00", "xxxxxxxxx");
    if (address)
    {
        SendDialog_Func = (SendDialog_pt)GW::Scanner::FunctionFromNearCall(address + 0x15);
    }
    if (!SendDialog_Func)
    {
        WriteChat(GW::Chat::CHANNEL_GWCA1, L"Failed to find", L"SendDialog_Func");
    }
    if (SendDialog_Func)
    {
        GW::HookBase::CreateHook(SendDialog_Func, OnIsDialogAvailable, (void **)&SendDialog_Ret);
        GW::HookBase::EnableHooks(SendDialog_Func);
    }
    if (!SendDialog_Func)
    {
        WriteChat(GW::Chat::CHANNEL_GWCA1, L"Failed to hook", L"SendDialog_Func");
    }

    // CTOS STUFF
    SendPacket_Func =
        (SendPacket_pt)GW::Scanner::FindAssertion("p:\\code\\net\\msg\\msgconn.cpp", "bytes >= sizeof(dword)", -0x67);
    ;
    if (!SendPacket_Func)
    {
        WriteChat(GW::Chat::CHANNEL_GWCA1, L"Failed to scan", L"SendPacket_Func");
    }
    if (SendPacket_Func)
    {
        GW::HookBase::CreateHook(SendPacket_Func, CtoSHandler_Func, reinterpret_cast<void **>(&SendPacket_Ret));
        GW::HookBase::EnableHooks(SendPacket_Func);
    }
    if (!SendPacket_Func)
    {
        WriteChat(GW::Chat::CHANNEL_GWCA1, L"Failed to hook", L"SendPacket_Func");
    }

    GW::Scanner::Initialize();
    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"AllowAllDialogs");
}

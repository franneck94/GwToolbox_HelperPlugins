#define GWCA_CTOS_ENABLED 1
#include "RawDialogs.h"

#include "GWCA/Managers/CtoSMgr.h"
#include "GWCA/Packets/Opcodes.h"

#include <cstdint>
#include <filesystem>

#include <imgui.h>
#include <Defines.h>
#include <GWCA/Constants/Constants.h>
#include <GWCA/GWCA.h>

#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/MemoryMgr.h>

#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static RawDialogs instance;
    return &instance;
}

auto ParseUInt(const wchar_t* str, unsigned int* val, const int base = 0)
{
    wchar_t* end;
    if (!str) {
        return false;
    }
    *val = std::wcstoul(str, &end, base);
    if (str == end || errno == ERANGE) {
        return false;
    }
    return true;
}

auto ParseUInt(const char* str, unsigned int* val, const int base = 0)
{
    char* end;
    if (!str) {
        return false;
    }
    *val = std::strtoul(str, &end, base);
    if (str == end || errno == ERANGE) {
        return false;
    }
    return true;
}

auto ParseInt(const wchar_t* str, int* val, const int base = 0)
{
    wchar_t* end;
    if (!str) {
        return false;
    }
    *val = std::wcstol(str, &end, base);
    if (str == end || errno == ERANGE) {
        return false;
    }
    return true;
}

auto ParseInt(const char* str, int* val, const int base = 0)
{
    char* end;
    if (!str) {
        return false;
    }
    *val = std::strtol(str, &end, base);
    if (str == end || errno == ERANGE) {
        return false;
    }
    return true;
}

void SendDialog(const wchar_t*, const int argc, const LPWSTR* argv)
{
    const auto IsMapReady = [] {
        return GW::Map::GetInstanceType() != GW::Constants::InstanceType::Loading && !GW::Map::GetIsObserving() && GW::MemoryMgr::GetGWWindowHandle() == GetActiveWindow();
    };
    const auto ParseUInt = [](const wchar_t* str, unsigned int* val, const int base = 0) {
        wchar_t* end;
        if (!str) {
            return false;
        }
        *val = wcstoul(str, &end, base);
        if (str == end || errno == ERANGE) {
            return false;
        }
        return true;
    };
    if (!IsMapReady()) {
        return;
    }
    if (argc <= 1) {
        return;
    }
    uint32_t id = 0;
    if (!(ParseUInt(argv[1], &id) && id)) {
        return;
    }
    GW::GameThread::Enqueue([id] {
        GW::CtoS::SendPacket(0x8, GAME_CMSG_SEND_DIALOG, id);
    });
}

void RawDialogs::Initialize(ImGuiContext* ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);

    GW::Chat::CreateCommand(L"rawdialog", SendDialog);
    WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"RawDialogs");
}

void RawDialogs::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::Chat::DeleteCommand(L"dialog");
    GW::Chat::DeleteCommand(L"rawdialog");
    GW::DisableHooks();
}

namespace {
    constexpr const char* const questnames[] = {
        "UW - Chamber", "UW - Wastes", "UW - UWG", "UW - Mnt", "UW - Pits", "UW - Planes", "UW - Pools", "UW - Escort", "UW - Restore", "UW - Vale", "FoW - Defend", "FoW - Army Of Darkness", "FoW - WailingLord",
        "FoW - Griffons", "FoW - Slaves", "FoW - Restore", "FoW - Hunt", "FoW - Forgemaster", "FoW - Tos", "FoW - Toc", "FoW - Khobay", "DoA - Gloom 1: Deathbringer Company", "DoA - Gloom 2: The Rifts Between Us",
        "DoA - Gloom 3: To The Rescue",
        "DoA - City", "DoA - Veil 1: Breaching Stygian Veil", "DoA - Veil 2: Brood Wars", "DoA - Foundry 1: Foundry Of Failed Creations", "DoA - Foundry 2: Foundry Breakout"
    };
    constexpr const char* const dialognames[] = {
        "Craft fow armor",
        "Prof Change - Warrior",
        "Prof Change - Ranger",
        "Prof Change - Monk",
        "Prof Change - Necro",
        "Prof Change - Mesmer",
        "Prof Change - Elementalist",
        "Prof Change - Assassin",
        "Prof Change - Ritualist",
        "Prof Change - Paragon",
        "Prof Change - Dervish",
        "Kama -> Docks @ Hahnna",
        "Docks -> Kaineng @ Mhenlo",
        "Docks -> LA Gate @ Mhenlo",
        "LA Gate -> LA @ Neiro",
        "Faction mission outpost",
        "Nightfall mission outpost",
    };

    GW::Constants::QuestID IndexToQuestID(const int index)
    {
        switch (index) {
            case 0:
                return GW::Constants::QuestID::UW_Chamber;
            case 1:
                return GW::Constants::QuestID::UW_Wastes;
            case 2:
                return GW::Constants::QuestID::UW_UWG;
            case 3:
                return GW::Constants::QuestID::UW_Mnt;
            case 4:
                return GW::Constants::QuestID::UW_Pits;
            case 5:
                return GW::Constants::QuestID::UW_Planes;
            case 6:
                return GW::Constants::QuestID::UW_Pools;
            case 7:
                return GW::Constants::QuestID::UW_Escort;
            case 8:
                return GW::Constants::QuestID::UW_Restore;
            case 9:
                return GW::Constants::QuestID::UW_Vale;
            case 10:
                return GW::Constants::QuestID::Fow_Defend;
            case 11:
                return GW::Constants::QuestID::Fow_ArmyOfDarknesses;
            case 12:
                return GW::Constants::QuestID::Fow_WailingLord;
            case 13:
                return GW::Constants::QuestID::Fow_Griffons;
            case 14:
                return GW::Constants::QuestID::Fow_Slaves;
            case 15:
                return GW::Constants::QuestID::Fow_Restore;
            case 16:
                return GW::Constants::QuestID::Fow_Hunt;
            case 17:
                return GW::Constants::QuestID::Fow_Forgemaster;
            case 18:
                return GW::Constants::QuestID::Fow_Tos;
            case 19:
                return GW::Constants::QuestID::Fow_Toc;
            case 20:
                return GW::Constants::QuestID::Fow_Khobay;
            case 21:
                return GW::Constants::QuestID::Doa_DeathbringerCompany;
            case 22:
                return GW::Constants::QuestID::Doa_RiftBetweenUs;
            case 23:
                return GW::Constants::QuestID::Doa_ToTheRescue;
            case 24:
                return GW::Constants::QuestID::Doa_City;
            case 25:
                return GW::Constants::QuestID::Doa_BreachingStygianVeil;
            case 26:
                return GW::Constants::QuestID::Doa_BroodWars;
            case 27:
                return GW::Constants::QuestID::Doa_FoundryOfFailedCreations;
            case 28:
                return GW::Constants::QuestID::Doa_FoundryBreakout;
            default:
                return static_cast<GW::Constants::QuestID>(0);
        }
    }

    uint32_t IndexToDialogID(const int index)
    {
        switch (index) {
            case 0:
                return GW::Constants::DialogID::FowCraftArmor;
            case 1:
                return GW::Constants::DialogID::ProfChangeWarrior;
            case 2:
                return GW::Constants::DialogID::ProfChangeRanger;
            case 3:
                return GW::Constants::DialogID::ProfChangeMonk;
            case 4:
                return GW::Constants::DialogID::ProfChangeNecro;
            case 5:
                return GW::Constants::DialogID::ProfChangeMesmer;
            case 6:
                return GW::Constants::DialogID::ProfChangeEle;
            case 7:
                return GW::Constants::DialogID::ProfChangeAssassin;
            case 8:
                return GW::Constants::DialogID::ProfChangeRitualist;
            case 9:
                return GW::Constants::DialogID::ProfChangeParagon;
            case 10:
                return GW::Constants::DialogID::ProfChangeDervish;
            case 11:
                return GW::Constants::DialogID::FerryKamadanToDocks;
            case 12:
                return GW::Constants::DialogID::FerryDocksToKaineng;
            case 13:
                return GW::Constants::DialogID::FerryDocksToLA;
            case 14:
                return GW::Constants::DialogID::FerryGateToLA;
            case 15:
                return GW::Constants::DialogID::FactionMissionOutpost;
            case 16:
                return GW::Constants::DialogID::NightfallMissionOutpost;
            default:
                return 0;
        }
    }
} // namespace

void RawDialogs::Draw(IDirect3DDevice9*)
{
    auto DialogButton = [](const int x_idx, const int x_qty, const char* text, const char* help, const DWORD dialog) -> void {
        if (x_idx != 0) {
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
        }
        const float w = (ImGui::GetWindowWidth() - ImGui::GetStyle().ItemInnerSpacing.x * (x_qty - 1)) / x_qty;
        if (ImGui::Button(text, ImVec2(w, 0))) {
            GW::Agents::SendDialog(dialog);
        }
        if (text != nullptr && ImGui::IsItemHovered()) {
            ImGui::SetTooltip(help);
        }
    };

    const auto& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(), can_close && show_closebutton ? GetVisiblePtr() : nullptr, GetWinFlags(ImGuiWindowFlags_NoScrollbar))) {
        if (show_common) {
            DialogButton(0, 2, "Four Horseman", "Take quest in Planes", QuestAcceptDialog(GW::Constants::QuestID::UW_Planes));
            DialogButton(1, 2, "Demon Assassin", "Take quest in Mountains", QuestAcceptDialog(GW::Constants::QuestID::UW_Mnt));
            DialogButton(0, 2, "Tower of Strength", "Take quest", QuestAcceptDialog(GW::Constants::QuestID::Fow_Tos));
            DialogButton(1, 2, "Foundry Reward", "Accept quest reward", QuestRewardDialog(GW::Constants::QuestID::Doa_FoundryBreakout));
            ImGui::Separator();
        }
        if (show_uwteles) {
            DialogButton(0, 4, "Lab", "Teleport Lab", GW::Constants::DialogID::UwTeleLab);
            DialogButton(1, 4, "Vale", "Teleport Vale", GW::Constants::DialogID::UwTeleVale);
            DialogButton(2, 4, "Pits", "Teleport Pits", GW::Constants::DialogID::UwTelePits);
            DialogButton(3, 4, "Pools", "Teleport Pools", GW::Constants::DialogID::UwTelePools);

            DialogButton(0, 3, "Planes", "Teleport Planes", GW::Constants::DialogID::UwTelePlanes);
            DialogButton(1, 3, "Wastes", "Teleport Wastes", GW::Constants::DialogID::UwTeleWastes);
            DialogButton(2, 3, "Mountains", "Teleport Mountains\nThis is NOT the mountains quest", GW::Constants::DialogID::UwTeleMnt);
            ImGui::Separator();
        }
        constexpr size_t n_quests = _countof(questnames);
        if (show_favorites) {
            for (int i = 0; i < fav_count; ++i) {
                const auto index = static_cast<size_t>(i);
                ImGui::PushID(i);
                ImGui::PushItemWidth(-100.0f - ImGui::GetStyle().ItemInnerSpacing.x * 2);
                ImGui::Combo("", &fav_index[index], questnames, n_quests);
                ImGui::PopItemWidth();
                ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
                if (ImGui::Button("Take", ImVec2(40.0f, 0))) {
                    GW::Agents::SendDialog(QuestAcceptDialog(IndexToQuestID(fav_index[index])));
                }
                ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
                if (ImGui::Button("Reward", ImVec2(60.0f, 0))) {
                    GW::Agents::SendDialog(QuestRewardDialog(IndexToQuestID(fav_index[index])));
                }
                ImGui::PopID();
            }
            ImGui::Separator();
        }
        if (show_custom) {
            constexpr int n_dialogs = _countof(dialognames);
            static int dialogindex = 0;
            ImGui::PushItemWidth(-60.0f - ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::Combo("###dialogcombo", &dialogindex, dialognames, n_dialogs);
            ImGui::PopItemWidth();
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            if (ImGui::Button("Send##1", ImVec2(60.0f, 0))) {
                GW::Agents::SendDialog(IndexToDialogID(dialogindex));
            }

            ImGui::PushItemWidth(-60.0f - ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::InputText("###dialoginput", customdialogbuf, 64, ImGuiInputTextFlags_None);
            ImGui::PopItemWidth();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("You can prefix the number by \"0x\" to specify an hexadecimal number");
            }
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            if (ImGui::Button("Send##2", ImVec2(60.0f, 0))) {
                int iid;
                if (ParseInt(customdialogbuf, &iid) && 0 <= iid) {
                    const auto id = static_cast<uint32_t>(iid);
                    GW::Agents::SendDialog(id);
                }
            }
        }
    }
    ImGui::End();
}

void RawDialogs::DrawSettings()
{
    ToolboxUIPlugin::DrawSettings();
    ImGui::PushItemWidth(100.0f);
    if (ImGui::InputInt("Number of favorites", &fav_count)) {
        if (fav_count < 0) {
            fav_count = 0;
        }
        if (fav_count > 100) {
            fav_count = 100;
        }
        const auto count = static_cast<size_t>(fav_count);
        fav_index.resize(count, 0);
    }
    ImGui::PopItemWidth();
    ImGui::Text("Show:");
    ImGui::Checkbox("Common 4", &show_common);
    ImGui::Checkbox("UW Teles", &show_uwteles);
    ImGui::Checkbox("Favorites", &show_favorites);
    ImGui::Checkbox("Custom", &show_custom);
}

void RawDialogs::LoadSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::LoadSettings(folder);
    ini.LoadFile(GetSettingFile(folder).c_str());
    fav_count = ini.GetLongValue(Name(), VAR_NAME(fav_count), 3);
    const auto count = static_cast<size_t>(fav_count);
    fav_index.resize(count, 0);
    for (size_t i = 0; i < count; ++i) {
        char key[32];
        snprintf(key, 32, "Quest%zu", i);
        fav_index[i] = ini.GetLongValue(Name(), key, 0);
    }
    show_common = ini.GetBoolValue(Name(), VAR_NAME(show_common), show_common);
    show_uwteles = ini.GetBoolValue(Name(), VAR_NAME(show_uwteles), show_uwteles);
    show_favorites = ini.GetBoolValue(Name(), VAR_NAME(show_favorites), show_favorites);
    show_custom = ini.GetBoolValue(Name(), VAR_NAME(show_custom), show_custom);
}

void RawDialogs::SaveSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::SaveSettings(folder);
    ini.SetLongValue(Name(), "fav_count", fav_count);
    const auto count = static_cast<size_t>(fav_count);
    for (size_t i = 0; i < count; ++i) {
        char key[32];
        snprintf(key, 32, "Quest%zu", i);
        ini.SetLongValue(Name(), key, fav_index[i]);
    }
    ini.SetBoolValue(Name(), VAR_NAME(show_common), show_common);
    ini.SetBoolValue(Name(), VAR_NAME(show_uwteles), show_uwteles);
    ini.SetBoolValue(Name(), VAR_NAME(show_favorites), show_favorites);
    ini.SetBoolValue(Name(), VAR_NAME(show_custom), show_custom);
    PLUGIN_ASSERT(ini.SaveFile(GetSettingFile(folder).c_str()) == SI_OK);
}

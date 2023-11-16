#include <string>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/CtoSMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/Managers/MemoryMgr.h>
#include <GWCA/Packets/Opcodes.h>

#include "Hotkeys.h"

#include <HelperMaps.h>
#include <Keys.h>
#include "Logger.h"
#include "Utils.h"

bool TBHotkey::show_active_in_header = true;
bool TBHotkey::hotkeys_changed = false;
WORD *TBHotkey::key_out = nullptr;
DWORD *TBHotkey::mod_out = nullptr;
unsigned int TBHotkey::cur_ui_id = 0;

TBHotkey *TBHotkey::HotkeyFactory(CSimpleIni *ini, const char *section)
{
    std::string str(section);
    if (str.compare(0, 7, "hotkey-") != 0)
        return nullptr;
    size_t first_sep = 6;
    size_t second_sep = str.find(L':', first_sep);
    std::string id = str.substr(first_sep + 1, second_sep - first_sep - 1);
    std::string type = str.substr(second_sep + 1);

    if (type.compare(HotkeyChestOpen::IniSection()) == 0)
    {
        return new HotkeyChestOpen(ini, section);
    }
    else if (type.compare(HotkeyTargetMinipet::IniSection()) == 0)
    {
        return new HotkeyTargetMinipet(ini, section);
    }

    return nullptr;
}

TBHotkey::TBHotkey(CSimpleIni *ini, const char *section) : ui_id(++cur_ui_id)
{
    if (!ini)
        return;

    hotkey = ini->GetLongValue(section, VAR_NAME(hotkey), hotkey);
    modifier = ini->GetLongValue(section, VAR_NAME(modifier), modifier);
    active = ini->GetBoolValue(section, VAR_NAME(active), active);
    instance_type = ini->GetLongValue(section, VAR_NAME(instance_type), instance_type);
    show_message_in_emote_channel =
        ini->GetBoolValue(section, VAR_NAME(show_message_in_emote_channel), show_message_in_emote_channel);
    trigger_on_explorable = ini->GetBoolValue(section, VAR_NAME(trigger_on_explorable), trigger_on_explorable);
    trigger_on_outpost = ini->GetBoolValue(section, VAR_NAME(trigger_on_outpost), trigger_on_outpost);
}

bool TBHotkey::IsValid(const GW::Constants::InstanceType _instance_type)
{
    return instance_type == -1 || static_cast<GW::Constants::InstanceType>(instance_type) == _instance_type;
}

bool TBHotkey::CanUse()
{
    return IsMapReady();
}

void TBHotkey::Save(CSimpleIni *ini, const char *section) const
{
    if (!ini)
        return;

    ini->SetLongValue(section, VAR_NAME(hotkey), hotkey);
    ini->SetLongValue(section, VAR_NAME(modifier), modifier);
    ini->SetLongValue(section, VAR_NAME(instance_type), instance_type);
    ini->SetBoolValue(section, VAR_NAME(active), active);
    ini->SetBoolValue(section, VAR_NAME(show_message_in_emote_channel), show_message_in_emote_channel);
    ini->SetBoolValue(section, VAR_NAME(trigger_on_explorable), trigger_on_explorable);
    ini->SetBoolValue(section, VAR_NAME(trigger_on_outpost), trigger_on_outpost);
}

void TBHotkey::HotkeySelector(WORD *key, DWORD *modifier)
{
    key_out = key;
    mod_out = modifier;
    ImGui::OpenPopup("Select Hotkey");
}

bool TBHotkey::Draw(Op *op)
{
    bool hotkey_changed = false;
    const float scale = ImGui::GetIO().FontGlobalScale;
    auto ShowHeaderButtons = [&]() {
        if (show_active_in_header)
        {
            ImGui::PushID(static_cast<int>(ui_id));
            ImGui::PushID("header");
            if (show_active_in_header)
            {
                ImGui::SameLine();
                hotkey_changed |= ImGui::Checkbox("", &active);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("The hotkey can trigger only when selected");
            }
            ImGui::PopID();
            ImGui::PopID();
        }
    };

    // === Header ===
    char header[256];
    int written = 0;
    written += Description(&header[written], _countof(header) - written);
    char keybuf[64];
    ModKeyName(keybuf, _countof(keybuf), modifier, hotkey, "<None>");
    snprintf(&header[written], _countof(header) - written, " [%s]###header%u", keybuf, ui_id);

    ImGuiTreeNodeFlags flags = (show_active_in_header) ? ImGuiTreeNodeFlags_AllowItemOverlap : 0;
    if (!ImGui::CollapsingHeader(header, flags))
    {
        ShowHeaderButtons();
    }
    else
    {
        ShowHeaderButtons();
        ImGui::Indent();
        ImGui::PushID(static_cast<int>(ui_id));
        ImGui::PushItemWidth(-140.0f * scale);
        // === Specific section ===
        hotkey_changed |= Draw();

        // === Hotkey section ===
        hotkey_changed |= ImGui::Checkbox("Trigger hotkey when entering explorable area", &trigger_on_explorable);
        hotkey_changed |= ImGui::Checkbox("Trigger hotkey when entering outpost", &trigger_on_outpost);
        ImGui::Separator();
        ImGui::Text("Instance Type: ");
        ImGui::SameLine();
        if (ImGui::RadioButton("Any ##instance_type_any", instance_type == -1))
        {
            instance_type = -1;
            hotkey_changed = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Outpost ##instance_type_outpost", instance_type == 0))
        {
            instance_type = 0;
            hotkey_changed = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Explorable ##instance_type_explorable", instance_type == 1))
        {
            instance_type = 1;
            hotkey_changed = true;
        }

        ImGui::Separator();
        hotkey_changed |= ImGui::Checkbox("###active", &active);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("The hotkey can trigger only when selected");
        ImGui::SameLine();
        char keybuf2[_countof(keybuf) + 8];
        snprintf(keybuf2, _countof(keybuf2), "Hotkey: %s", keybuf);
        if (ImGui::Button(keybuf2, ImVec2(-140.0f * scale, 0)))
        {
            HotkeySelector((WORD *)&hotkey, (DWORD *)&modifier);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Click to change hotkey");
        if (ImGui::BeginPopup("Select Hotkey"))
        {
            static WORD newkey = 0;
            *op = Op_BlockInput;
            ImGui::Text("Press key");
            DWORD newmod = 0;
            bool *keysdown = ImGui::GetIO().KeysDown;
            if (mod_out)
            {
                if (keysdown[VK_CONTROL])
                    newmod |= ModKey_Control;
                if (keysdown[VK_MENU])
                    newmod |= ModKey_Alt;
                if (keysdown[VK_SHIFT])
                    newmod |= ModKey_Shift;
            }


            if (newkey == 0)
            { // we are looking for the key
                for (WORD i = 0; i < 512; ++i)
                {
                    if (i == VK_CONTROL)
                        continue;
                    if (i == VK_SHIFT)
                        continue;
                    if (i == VK_MENU)
                        continue;
                    if (keysdown[i])
                    {
                        newkey = i;
                    }
                }
            }
            else
            { // key was pressed, close if it's released
                if (!keysdown[newkey])
                {
                    *key_out = newkey;
                    if (mod_out)
                        *mod_out = newmod;
                    newkey = 0;
                    ImGui::CloseCurrentPopup();
                    hotkey_changed = true;
                }
            }

            // write the key
            char newkey_buf[256];
            ModKeyName(newkey_buf, _countof(newkey_buf), newmod, newkey);
            ImGui::Text("%s", newkey_buf);
            if (ImGui::Button("Clear"))
            {
                *key_out = 0;
                if (mod_out)
                    *mod_out = 0;
                newkey = 0;
                ImGui::CloseCurrentPopup();
                hotkey_changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                newkey = 0;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Execute the hotkey now");
        ImGui::NewLine();
        // === Move and delete buttons ===
        if (ImGui::Button("Move Up", ImVec2(ImGui::GetWindowContentRegionWidth() / 3.0f, 0)))
        {
            *op = Op_MoveUp;
            hotkey_changed = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Move the hotkey up in the list");
        ImGui::SameLine();
        const auto width = ImGui::GetWindowWidth();
        ImGui::PushItemWidth(width / 3.0F - 50.0F);
        if (ImGui::Button("Move Down", ImVec2(ImGui::GetWindowContentRegionWidth() / 3.0f, 0)))
        {
            *op = Op_MoveDown;
            hotkey_changed = true;
        }
        ImGui::PopItemWidth();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Move the hotkey down in the list");
        ImGui::SameLine();
        ImGui::PushItemWidth(width / 3.0F - 50.0F);
        if (ImGui::Button("Delete", ImVec2(ImGui::GetWindowContentRegionWidth() / 3.0f, 0)))
        {
            ImGui::OpenPopup("Delete Hotkey?");
        }
        ImGui::PopItemWidth();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Delete the hotkey");
        if (ImGui::BeginPopupModal("Delete Hotkey?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Are you sure?\nThis operation cannot be undone\n\n", Name());
            if (ImGui::Button("OK", ImVec2(120.f * scale, 0)))
            {
                *op = Op_Delete;
                hotkey_changed = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120.f * scale, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::PopItemWidth();
        ImGui::PopID();
        ImGui::Unindent();
    }

    return hotkey_changed;
}

bool HotkeyChestOpen::GetText(void *, int idx, const char **out_text)
{
    switch (static_cast<ChestType>(idx))
    {
    case ChestType::OpenXunlaiChest:
    {
        *out_text = "Open Xunlai Chest";
        return true;
    }
    case ChestType::OpenLockedChest:
    {
        *out_text = "Open Locked Chest";
        return true;
    }
    default:
    {
        return false;
    }
    }
}

HotkeyChestOpen::HotkeyChestOpen(CSimpleIni *ini, const char *section) : TBHotkey(ini, section)
{
    action =
        static_cast<ChestType>(ini->GetLongValue(section, "ActionID", static_cast<long>(ChestType::OpenXunlaiChest)));
}

void HotkeyChestOpen::Save(CSimpleIni *ini, const char *section) const
{
    TBHotkey::Save(ini, section);
    ini->SetLongValue(section, "ActionID", (long)action);
}

int HotkeyChestOpen::Description(char *buf, size_t bufsz)
{
    const char *name;
    GetText(nullptr, (int)action, &name);
    return snprintf(buf, bufsz, "%s", name);
}

bool HotkeyChestOpen::Draw()
{
    return ImGui::Combo("ChestType###combo", (int *)&action, &GetText, nullptr, n_actions);
}

void HotkeyChestOpen::Execute()
{
    if (!CanUse())
        return;
    switch (action)
    {
    case ChestType::OpenXunlaiChest:
    {
        if (!IsExplorable() && !IsLoading())
        {
            GW::GameThread::Enqueue([]() { GW::Items::OpenXunlaiWindow(); });
        }
        break;
    }
    case ChestType::OpenLockedChest:
    {
        if (IsExplorable())
        {
            const auto *target = GW::Agents::GetTarget();
            if (target && target->type == 0x200)
            {
                GW::Agents::GoSignpost(target);
                GW::Items::OpenLockedChest();
            }
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

HotkeyTargetMinipet::HotkeyTargetMinipet(CSimpleIni *ini, const char *section) : TBHotkey(ini, section)
{
    show_message_in_emote_channel = false;
    name[0] = 0;
    if (!ini)
        return;

    const auto ini_name = std::string{ini->GetValue(section, "TargetID", "")};
    strcpy_s(id, ini_name.substr(0, sizeof(id) - 1).c_str());
    id[sizeof(id) - 1] = 0;
    strcpy_s(name, ini_name.substr(0, sizeof(name) - 1).c_str());
    name[sizeof(name) - 1] = 0;

    ini->GetBoolValue(section, VAR_NAME(show_message_in_emote_channel), show_message_in_emote_channel);
}

void HotkeyTargetMinipet::Save(CSimpleIni *ini, const char *section) const
{
    TBHotkey::Save(ini, section);
    ini->SetValue(section, "TargetID", id);
}

int HotkeyTargetMinipet::Description(char *buf, size_t bufsz)
{
    if (!name[0])
        return snprintf(buf, bufsz, "Target Minipet %s", id);
    return snprintf(buf, bufsz, "Target Minipet");
}

bool HotkeyTargetMinipet::Draw()
{
    const float w = ImGui::GetContentRegionAvail().x / 1.5f;
    ImGui::PushItemWidth(w);
    bool hotkey_changed = ImGui::InputText("Model ID", id, _countof(id));
    ImGui::PopItemWidth();
    hotkey_changed |= ImGui::Checkbox("Display message when triggered", &show_message_in_emote_channel);
    return hotkey_changed;
}

void HotkeyTargetMinipet::Execute()
{
    if (!CanUse())
        return;

    constexpr auto len = size_t{122};
    auto *message = new wchar_t[len];
    message[0] = 0;
    swprintf(message, len, L"target npc %S", id);
    GW::GameThread::Enqueue([message]() {
        GW::Chat::SendChat('/', message);
        delete[] message;
    });

    if (show_message_in_emote_channel)
    {
        char buf[256];
        Description(buf, 256);
        Log::Info("Triggered %s", buf);
    }
}

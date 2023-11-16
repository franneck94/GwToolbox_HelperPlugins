#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/MemoryMgr.h>

#include <Base/HelperBoxWindow.h>
#include <HelperMaps.h>

#include "HotkeysWindow.h"
#include "Keys.h"
#include <Logger.h>


bool HotkeysWindow::CheckSetValidHotkeys()
{
    const auto instance_type = GW::Map::GetInstanceType();

    valid_hotkeys.clear();
    for (auto *hotkey : hotkeys)
    {
        if (hotkey->IsValid(instance_type))
            valid_hotkeys.push_back(hotkey);
    }

    return true;
}

void HotkeysWindow::Draw()
{
    if (!visible)
        return;
    auto hotkeys_changed = false;
    // === hotkey panel ===
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(), GetVisiblePtr(), GetWinFlags()))
    {
        if (ImGui::Button("Create Hotkey...", ImVec2(ImGui::GetWindowContentRegionWidth(), 0)))
            ImGui::OpenPopup("Create Hotkey");

        if (ImGui::BeginPopup("Create Hotkey"))
        {
            TBHotkey *new_hotkey = nullptr;

            if (ImGui::Selectable("Chest..."))
                new_hotkey = new HotkeyChestOpen(nullptr, nullptr);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Execute opening a chests");

            if (ImGui::Selectable("Target..."))
                new_hotkey = new HotkeyTargetMinipet(nullptr, nullptr);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Targets an agent...");

            ImGui::EndPopup();
            if (new_hotkey)
                hotkeys.push_back(new_hotkey);
        }

        // === each hotkey ===
        block_hotkeys = false;
        auto draw_hotkeys_vec = [&](std::vector<TBHotkey *> &in) {
            auto these_hotkeys_changed = false;
            for (auto i = 0U; i < in.size(); ++i)
            {
                TBHotkey::Op op = TBHotkey::Op_None;
                these_hotkeys_changed |= in[i]->Draw(&op);
                switch (op)
                {
                case TBHotkey::Op_None:
                    break;
                case TBHotkey::Op_MoveUp:
                {
                    auto it = std::find(hotkeys.begin(), hotkeys.end(), in[i]);
                    if (it != hotkeys.end() && it != hotkeys.begin())
                    {
                        std::swap(*it, *(it - 1));
                        these_hotkeys_changed = true;
                        return true;
                    }
                }
                break;
                case TBHotkey::Op_MoveDown:
                {
                    auto it = std::find(hotkeys.begin(), hotkeys.end(), in[i]);
                    if (it != hotkeys.end() && it != hotkeys.end() - 1)
                    {
                        std::swap(*it, *(it + 1));
                        these_hotkeys_changed = true;
                        return true;
                    }
                }
                break;
                case TBHotkey::Op_Delete:
                {
                    auto *hk = hotkeys[i];
                    hotkeys.erase(hotkeys.begin() + static_cast<int>(i));
                    delete hk;
                    return true;
                }
                break;
                case TBHotkey::Op_BlockInput:
                    block_hotkeys = true;
                    break;
                default:
                    break;
                }
            }
            return these_hotkeys_changed;
        };

        hotkeys_changed |= draw_hotkeys_vec(hotkeys);
    }

    if (hotkeys_changed)
    {
        CheckSetValidHotkeys();
        TBHotkey::hotkeys_changed = true;
    }

    ImGui::End();
}

void HotkeysWindow::DrawSettingInternal()
{
    HelperBoxWindow::DrawSettingInternal();
    ImGui::Checkbox("Show 'Active' checkbox in header", &TBHotkey::show_active_in_header);
}

void HotkeysWindow::LoadSettings(CSimpleIni *ini)
{
    HelperBoxWindow::LoadSettings(ini);
    TBHotkey::show_active_in_header = ini->GetBoolValue(Name(), "show_active_in_header", false);

    for (auto *hotkey : hotkeys)
        delete hotkey;
    hotkeys.clear();

    // then load again
    CSimpleIni::TNamesDepend entries;
    ini->GetAllSections(entries);
    for (CSimpleIni::Entry &entry : entries)
    {
        auto *hk = TBHotkey::HotkeyFactory(ini, entry.pItem);
        if (hk)
            hotkeys.push_back(hk);
    }

    TBHotkey::hotkeys_changed = false;
}

void HotkeysWindow::SaveSettings(CSimpleIni *ini)
{
    HelperBoxWindow::SaveSettings(ini);
    ini->SetBoolValue(Name(), "show_active_in_header", TBHotkey::show_active_in_header);

    if (TBHotkey::hotkeys_changed)
    {
        CSimpleIni::TNamesDepend entries;
        ini->GetAllSections(entries);
        for (CSimpleIni::Entry &entry : entries)
        {
            if (strncmp(entry.pItem, "hotkey-", 7) == 0)
            {
                ini->Delete(entry.pItem, nullptr);
            }
        }

        char buf[256];
        for (auto i = 0U; i < hotkeys.size(); ++i)
        {
            snprintf(buf, 256, "hotkey-%03d:%s", i, hotkeys[i]->Name());
            hotkeys[i]->Save(ini, buf);
        }
    }
}

bool HotkeysWindow::WndProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    if (GW::Chat::GetIsTyping())
        return false;
    if (GW::MemoryMgr::GetGWWindowHandle() != GetActiveWindow())
        return false;
    long keyData = 0;
    switch (Message)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
        keyData = static_cast<long>(wParam);
        break;
    case WM_XBUTTONDOWN:
    case WM_MBUTTONDOWN:
        if (LOWORD(wParam) & MK_MBUTTON)
            keyData = VK_MBUTTON;
        if (LOWORD(wParam) & MK_XBUTTON1)
            keyData = VK_XBUTTON1;
        if (LOWORD(wParam) & MK_XBUTTON2)
            keyData = VK_XBUTTON2;
        break;
    case WM_XBUTTONUP:
    case WM_MBUTTONUP:
        // leave keydata to none, need to handle special case below
        break;
    default:
        break;
    }

    switch (Message)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_XBUTTONDOWN:
    case WM_MBUTTONDOWN:
    {
        if (block_hotkeys)
            return true;
        long modifier = 0;
        if (GetKeyState(VK_CONTROL) < 0)
            modifier |= ModKey_Control;
        if (GetKeyState(VK_SHIFT) < 0)
            modifier |= ModKey_Shift;
        if (GetKeyState(VK_MENU) < 0)
            modifier |= ModKey_Alt;

        auto triggered = false;
        for (auto *hk : valid_hotkeys)
        {
            if (!block_hotkeys && !hk->pressed && keyData == hk->hotkey && modifier == hk->modifier)
            {
                hk->pressed = true;
                current_hotkey = hk;
                hk->Toggle();
                current_hotkey = nullptr;
            }
        }
        return triggered;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
        for (auto *hk : hotkeys)
        {
            if (hk->pressed && keyData == hk->hotkey)
                hk->pressed = false;
        }
        return false;

    case WM_XBUTTONUP:
        for (auto *hk : hotkeys)
        {
            if (hk->pressed && (hk->hotkey == VK_XBUTTON1 || hk->hotkey == VK_XBUTTON2))
                hk->pressed = false;
        }
        return false;
    case WM_MBUTTONUP:
        for (auto *hk : hotkeys)
        {
            if (hk->pressed && hk->hotkey == VK_MBUTTON)
                hk->pressed = false;
        }
    default:
        return false;
    }
}

bool HotkeysWindow::HandleMapChange()
{
    if (!IsMapReady())
        return false;
    if (!GW::Agents::GetPlayerAsAgentLiving())
        return false;
    const auto _instance_type = GW::Map::GetInstanceType();
    if (_instance_type == GW::Constants::InstanceType::Loading)
        return false;
    if (!CheckSetValidHotkeys())
        return false;
    for (auto *hk : valid_hotkeys)
    {
        if (!block_hotkeys &&
            ((hk->trigger_on_explorable && _instance_type == GW::Constants::InstanceType::Explorable) ||
             (hk->trigger_on_outpost && _instance_type == GW::Constants::InstanceType::Outpost)) &&
            !hk->pressed)
        {

            hk->pressed = true;
            current_hotkey = hk;
            hk->Execute();
            current_hotkey = nullptr;
            hk->pressed = false;
        }
    }
    return true;
}

void HotkeysWindow::Update(float, const AgentLivingData &)
{
    if (GW::Map::GetInstanceType() == GW::Constants::InstanceType::Loading)
    {
        if (map_change_triggered)
            map_change_triggered = false;
        return;
    }
    if (!map_change_triggered)
        map_change_triggered = HandleMapChange();
}

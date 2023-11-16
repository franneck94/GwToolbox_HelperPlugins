#include <GWCA/Managers/MapMgr.h>

#include "Utils.h"
#include <Base/HelperBox.h>
#include <Base/HelperBoxSettings.h>
#include <Base/MainWindow.h>

#include <imgui.h>

#include "SettingsWindow.h"

void SettingsWindow::Draw()
{
    static auto last_instance_type = GW::Constants::InstanceType::Loading;
    auto instance_type = GW::Map::GetInstanceType();

    if (instance_type == GW::Constants::InstanceType::Loading)
        return;

    if (instance_type != last_instance_type)
        last_instance_type = instance_type;

    if (!visible)
        return;

    ImGui::SetNextWindowSize(ImVec2(768, 768), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(), GetVisiblePtr(), GetWinFlags()))
    {
        drawn_settings.clear();
        ImGui::Text("HelperBox");

        DrawSettingsSection(HelperBoxSettings::Instance().SettingsName());

        const auto &optional_modules = HelperBoxSettings::Instance().GetOptionalModules();
        for (size_t i = 0; i < optional_modules.size(); ++i)
        {
            if (i == sep)
                ImGui::Text("Components:");
            if (strcmp(optional_modules[i]->SettingsName(), "Chat Settings") != 0)
                DrawSettingsSection(optional_modules[i]->SettingsName());
        }
    }

    const auto w = ImGui::GetWindowContentRegionWidth();

    if (ImGui::Button("Save Now", ImVec2(w, 0)))
        HelperBox::Instance().SaveSettings();

    if (ImGui::Button("Load Now", ImVec2(w, 0)))
    {
        HelperBox::Instance().OpenSettingsFile();
        HelperBox::Instance().LoadModuleSettings();
    }

    ImGui::End();
}

bool SettingsWindow::DrawSettingsSection(const char *section)
{
    const auto &callbacks = HelperBoxModule::GetSettingsCallbacks();
    const auto &icons = HelperBoxModule::GetSettingsIcons();

    const auto &settings_section = callbacks.find(section);
    if (settings_section == callbacks.end())
        return false;
    if (drawn_settings.find(section) != drawn_settings.end())
        return true; // Already drawn
    drawn_settings[section] = true;

    static char buf[128];
    sprintf(buf, "      %s", section);
    auto pos = ImGui::GetCursorScreenPos();
    const auto is_showing = ImGui::CollapsingHeader(buf, ImGuiTreeNodeFlags_AllowItemOverlap);

    const char *icon = nullptr;
    auto it = icons.find(section);
    if (it != icons.end())
        icon = it->second;
    if (icon)
    {
        const auto &style = ImGui::GetStyle();
        const auto text_offset_x = ImGui::GetTextLineHeightWithSpacing() + 4.0f;
        ImGui::GetWindowDrawList()->AddText(ImVec2(pos.x + text_offset_x, pos.y + style.ItemSpacing.y / 2),
                                            ImColor(style.Colors[ImGuiCol_Text]),
                                            icon);
    }

    if (is_showing)
        ImGui::PushID(section);
    auto i = size_t{0};
    for (auto &entry : settings_section->second)
    {
        ImGui::PushID(&settings_section->second);
        if (i && is_showing)
            ImGui::Separator();
        entry.second(&settings_section->first, is_showing);
        i++;
        ImGui::PopID();
    }
    if (is_showing)
        ImGui::PopID();
    return true;
}

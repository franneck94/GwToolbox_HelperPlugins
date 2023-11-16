#include <Base/HelperBox.h>
#include <Utils.h>

#include <d3d9.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "HelperBoxUIElement.h"

const char *HelperBoxUIElement::UIName() const
{
    return Name();
}

void HelperBoxUIElement::Initialize()
{
    HelperBoxModule::Initialize();
}

void HelperBoxUIElement::Terminate()
{
    HelperBoxModule::Terminate();
    if (button_texture)
        button_texture->Release();
    button_texture = nullptr;
}

void HelperBoxUIElement::LoadSettings(CSimpleIni *ini)
{
    HelperBoxModule::LoadSettings(ini);
    visible = ini->GetBoolValue(Name(), VAR_NAME(visible), visible);
}

void HelperBoxUIElement::SaveSettings(CSimpleIni *ini)
{
    HelperBoxModule::SaveSettings(ini);
    ini->SetBoolValue(Name(), VAR_NAME(visible), visible);
}

void HelperBoxUIElement::RegisterSettingsContent()
{
    HelperBoxModule::RegisterSettingsContent(SettingsName(), [this](const std::string *, bool is_showing) {
        ShowVisibleRadio();
        if (!is_showing)
            return;
        DrawSizeAndPositionSettings();
        DrawSettingInternal();
    });
}

void HelperBoxUIElement::DrawSizeAndPositionSettings()
{
    auto pos = ImVec2(0.0F, 0.0F);
    auto size = ImVec2(100.0F, 100.0F);
    auto window = ImGui::FindWindowByName(Name());
    if (window)
    {
        pos = window->Pos;
        size = window->Size;
    }
    if (is_movable || is_resizable)
    {
        if (is_movable)
        {
            if (ImGui::DragFloat2("Position", reinterpret_cast<float *>(&pos), 1.0f, 0.0f, 0.0f, "%.0f"))
                ImGui::SetWindowPos(Name(), pos);
        }
        if (is_resizable)
        {
            if (ImGui::DragFloat2("Size", reinterpret_cast<float *>(&size), 1.0f, 0.0f, 0.0f, "%.0f"))
                ImGui::SetWindowSize(Name(), size);
        }
    }
    auto new_line = false;
    if (is_movable)
    {
        if (new_line)
            ImGui::SameLine();
        new_line = true;
        ImGui::Checkbox("Lock Position", &lock_move);
    }
    if (is_resizable)
    {
        if (new_line)
            ImGui::SameLine();
        new_line = true;
        ImGui::Checkbox("Lock Size", &lock_size);
    }
    if (has_closebutton)
    {
        if (new_line)
            ImGui::SameLine();
        new_line = true;
        ImGui::Checkbox("Show close button", &show_closebutton);
    }
    if (can_show_in_main_window)
    {
        if (new_line)
            ImGui::SameLine();
        new_line = true;
    }

    ImGui::NewLine();
}

void HelperBoxUIElement::ShowVisibleRadio()
{
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetTextLineHeight() -
                    ImGui::GetStyle().FramePadding.y * 2);
    ImGui::PushID(Name());
    ImGui::Checkbox("##check", &visible);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Visible");
    ImGui::PopID();
}

bool HelperBoxUIElement::DrawTabButton()
{
    ImGui::PushStyleColor(ImGuiCol_Button, visible ? ImGui::GetStyle().Colors[ImGuiCol_Button] : ImVec4(0, 0, 0, 0));
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 textsize = ImGui::CalcTextSize(Name());
    const auto width = ImGui::GetWindowWidth();

    auto img_size = 0.0F;

    const auto text_x = pos.x + img_size + ImGui::GetStyle().ItemSpacing.x;
    const auto clicked = ImGui::Button("", ImVec2(width, ImGui::GetTextLineHeightWithSpacing()));

    ImGui::GetWindowDrawList()->AddText(ImVec2(text_x, pos.y + ImGui::GetStyle().ItemSpacing.y / 2),
                                        ImColor(ImGui::GetStyle().Colors[ImGuiCol_Text]),
                                        Name());

    if (clicked)
        visible = !visible;
    ImGui::PopStyleColor();
    return clicked;
}

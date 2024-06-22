#include <cmath>
#include <string_view>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Camera.h>

#include "ActionsBase.h"
#include "HelperMaps.h"

#include <imgui.h>

#include "UtilsGui.h"

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923 // pi/2
#endif

namespace
{
void _PlotLine(const ImVec2 &p1,
               const ImVec2 &p2,
               const float thickness = 1.0F,
               const ImVec4 &color = ImVec4{0.0F, 0.0F, 0.0F, 1.0F})
{
    const auto colorU32 = ImGui::ColorConvertFloat4ToU32(color);

    auto *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddLine(p1, p2, colorU32, thickness);
}

void _PlotPoint(const ImVec2 point, const float radius = 1.0F, const ImVec4 &color = ImVec4{0.0F, 0.0F, 0.0F, 1.0F})
{
    const auto colorU32 = ImGui::ColorConvertFloat4ToU32(color);

    auto *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddCircleFilled(point, radius, colorU32);
}
} // namespace

void DrawButton(ActionState &action_state, const ImVec4 color, std::string_view text, const ImVec2 button_size)
{
    auto pushed_style = false;

    if (action_state != ActionState::INACTIVE)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, color);
        pushed_style = true;
    }

    if (ImGui::Button(text.data(), button_size))
    {
        if (IsExplorable())
            action_state = StateNegation(action_state);
    }
    if (pushed_style)
        ImGui::PopStyleColor();
}

#ifdef _DEBUG
void PlotRectangleLine(const GW::GamePos &player_pos, const GW::GamePos &p1, const GW::GamePos &p2)
{
    auto cam = GW::CameraMgr::GetCamera();
    if (!cam)
        return;
    const auto angle = (cam->GetCurrentYaw() + static_cast<float>(M_PI_2));
    const auto p1_ = RotatePoint(player_pos, p1, angle);
    const auto p2_ = RotatePoint(player_pos, p2, angle);

    const auto v1 = ImVec2{p1_.x * -1.0F, p1_.y};
    const auto v2 = ImVec2{p2_.x * -1.0F, p2_.y};
    const auto v3 = ImVec2{p2_.x * -1.0F, p2_.y};
    const auto v4 = ImVec2{p2_.x * -1.0F, p2_.y};
    // TODO
    _PlotLine(v1, v2, 2.0F, ImVec4{1.0F, 0.7F, 0.1F, 1.0F});
    _PlotLine(v1, v3, 2.0F, ImVec4{1.0F, 0.7F, 0.1F, 1.0F});
    _PlotLine(v2, v4, 2.0F, ImVec4{1.0F, 0.7F, 0.1F, 1.0F});
    _PlotLine(v3, v4, 2.0F, ImVec4{1.0F, 0.7F, 0.1F, 1.0F});
}

void PlotPoint(const GW::GamePos &player_pos, const GW::GamePos &p, const ImVec4 &color, const float width)
{
    auto cam = GW::CameraMgr::GetCamera();
    if (!cam)
        return;
    const auto angle = (cam->GetCurrentYaw() + static_cast<float>(M_PI_2));
    const auto p_ = RotatePoint(player_pos, p, angle);

    const auto v = ImVec2{p_.x * -1.0F, p_.y};
    _PlotPoint(v, 1.0F, color);
}

void PlotCircle(const GW::GamePos &player_pos, const ImVec4 &color)
{
    for (int i = 0; i < 360; i++)
    {
        const auto x_p = player_pos.x + 1050.0F * std::sin((float)i);
        const auto y_p = player_pos.y + 1050.0F * std::cos((float)i);
        const auto pos = GW::GamePos{x_p, y_p, 0};
        PlotPoint(player_pos, pos, color, 1.0F);
    }
}

void PloLine(const GW::GamePos &player_pos, const GW::GamePos p1, const GW::GamePos p2)
{
    auto cam = GW::CameraMgr::GetCamera();
    if (!cam)
        return;
    const auto angle = (cam->GetCurrentYaw() + static_cast<float>(M_PI_2));
    const auto p1_ = RotatePoint(player_pos, p1, angle);
    const auto p2_ = RotatePoint(player_pos, p2, angle);

    const auto v1 = ImVec2{p1_.x, p1_.y};
    const auto v2 = ImVec2{p2_.x, p2_.y};
    _PlotLine(v1, v2, 2.0F, ImVec4{1.0F, 0.7F, 0.1F, 1.0F});
}

void PlotEnemies(const GW::GamePos &player_pos,
                 const std::vector<const GW::AgentLiving *> &living_agents,
                 const ImVec4 &color)
{
    auto idx = 0U;
    for (const auto living : living_agents)
    {
        if (!living)
            continue;
        if (living->login_number == GW::Constants::ModelID::UW::SkeletonOfDhuum1 ||
            living->login_number == GW::Constants::ModelID::UW::SkeletonOfDhuum2)
            PlotPoint(player_pos, living->pos, ImVec4{0.0F, 0.0F, 1.0f, 1.0F});
        else
            PlotPoint(player_pos, living->pos, color);
        ++idx;
    }
}

void DrawMap(const GW::GamePos &player_pos,
             const std::vector<const GW::AgentLiving *> &enemies,
             const GW::GamePos &move_pos,
             std::string_view label)
{
    const auto cam = GW::CameraMgr::GetCamera();
    if (!cam)
        return;
    const auto theta = cam->GetCurrentYaw() - static_cast<float>(M_PI_2);
    if (std::isnan(theta))
        return;

    ImGui::SetNextWindowSize(ImVec2{450.0F, 450.0F}, ImGuiCond_FirstUseEver);
    const auto label_window = std::format("{}Window", label.data());
    if (ImGui::Begin(label_window.data(), nullptr, ImGuiWindowFlags_None))
    {
        const auto next_pos = move_pos;
        const auto rect = GameRectangle(player_pos, next_pos, GW::Constants::Range::Spellcast);

        const auto border_thickness = 1.5F;
        const auto canvasPos = ImGui::GetCursorPos();
        const auto border_min = canvasPos;
        const auto border_max =
            ImVec2(canvasPos.x + button_size.x - border_thickness, canvasPos.y + button_size.y - border_thickness);
        draw_list->AddRect(border_min,
                           border_max,
                           IM_COL32(255, 255, 255, 255),
                           0.0F,
                           ImDrawCornerFlags_All,
                           border_thickness);

        PlotPoint(player_pos, player_pos, ImVec4{1.0F, 1.0F, 1.0F, 1.0F}, 5.0F);
        PlotPoint(player_pos, next_pos, ImVec4{0.5F, 0.5F, 0.0F, 1.0F}, 5.0F);

        PlotRectangleLine(player_pos, rect.v1, rect.v2);
        PlotRectangleLine(player_pos, rect.v1, rect.v3);
        PlotRectangleLine(player_pos, rect.v4, rect.v2);
        PlotRectangleLine(player_pos, rect.v4, rect.v3);

        PlotCircle(player_pos, ImVec4{0.0, 0.0, 1.0, 1.0});

        PlotEnemies(player_pos, enemies, ImVec4{1.0F, 0.65F, 0.0, 1.0});

        const auto filtered_livings = GetEnemiesInGameRectangle(rect, enemies);
        PlotEnemies(player_pos, filtered_livings, ImVec4{1.0, 0.0, 0.0, 1.0});
    }
    ImGui::End();
}

void DrawFlaggingFeature(const GW::GamePos &player_pos, std::string_view label)
{
    const auto cam = GW::CameraMgr::GetCamera();
    if (!cam)
        return;
    const auto theta = cam->GetCurrentYaw() - static_cast<float>(M_PI_2);
    if (std::isnan(theta))
        return;

    ImGui::SetNextWindowSize(ImVec2{450.0F, 450.0F}, ImGuiCond_FirstUseEver);
    const auto label_window = std::format("{}Window", label.data());
    if (ImGui::Begin(label_window.data(), nullptr, ImGuiWindowFlags_None))
    {
        PlotPoint(player_pos, player_pos, ImVec4{1.0F, 1.0F, 1.0F, 1.0F}, 5.0F);
    }
    ImGui::End();
}
#endif

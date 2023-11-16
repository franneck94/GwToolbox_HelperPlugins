#include <string_view>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Camera.h>

#include <ActionsBase.h>
#include <HelperMaps.h>

#include <fmt/format.h>
#include <imgui.h>
#include <implot.h>

#include "UtilsGui.h"

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

void PlotRectangleLine(const GW::GamePos &player_pos,
                       const GW::GamePos &p1,
                       const GW::GamePos &p2,
                       std::string_view label)
{
    auto cam = GW::CameraMgr::GetCamera();
    if (!cam)
        return;
    const auto angle = (cam->GetCurrentYaw() + static_cast<float>(M_PI_2));
    const auto p1_ = RotatePoint(player_pos, p1, angle);
    const auto p2_ = RotatePoint(player_pos, p2, angle);

    const float xs[2] = {p1_.x * -1.0F, p2_.x * -1.0F};
    const float ys[2] = {p1_.y, p2_.y};
    ImPlot::SetNextLineStyle(ImVec4{1.0F, 0.7F, 0.1F, 1.0F}, 2.0F);
    ImPlot::PlotLine(label.data(), xs, ys, 2);
}

void PlotPoint(const GW::GamePos &player_pos,
               const GW::GamePos &p,
               std::string_view label,
               const ImVec4 &color,
               const float width)
{
    auto cam = GW::CameraMgr::GetCamera();
    if (!cam)
        return;
    const auto angle = (cam->GetCurrentYaw() + static_cast<float>(M_PI_2));
    const auto p_ = RotatePoint(player_pos, p, angle);

    const float xs[1] = {p_.x * -1.0F};
    const float ys[1] = {p_.y};
    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, width, color, 1.0F, color);
    ImPlot::PlotScatter(label.data(), xs, ys, 1);
}

void PlotCircle(const GW::GamePos &player_pos, std::string_view label, const ImVec4 &color)
{
    for (int i = 0; i < 360; i++)
    {
        const auto label_ = fmt::format("{}###{}", label.data(), i);
        const auto x_p = player_pos.x + 1050.0F * std::sin((float)i);
        const auto y_p = player_pos.y + 1050.0F * std::cos((float)i);
        const auto pos = GW::GamePos{x_p, y_p, 0};
        PlotPoint(player_pos, pos, label_, color, 1.0F);
    }
}

void PlotEnemies(const GW::GamePos &player_pos,
                 const std::vector<GW::AgentLiving *> &living_agents,
                 std::string_view label,
                 const ImVec4 &color)
{
    auto idx = 0U;
    for (const auto living : living_agents)
    {
        if (!living)
            continue;
        const auto label_ = fmt::format("{}##{}", label.data(), idx);
        if (living->login_number == GW::Constants::ModelID::UW::SkeletonOfDhuum1 ||
            living->login_number == GW::Constants::ModelID::UW::SkeletonOfDhuum2)
            PlotPoint(player_pos, living->pos, label_, ImVec4{0.0F, 0.0F, 1.0f, 1.0F});
        else
            PlotPoint(player_pos, living->pos, label_, color);
        ++idx;
    }
}

void DrawMap(const GW::GamePos &player_pos,
             const std::vector<GW::AgentLiving *> &enemies,
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
    const auto label_window = fmt::format("{}Window", label.data());
    if (ImGui::Begin(label_window.data(), nullptr, ImGuiWindowFlags_None))
    {
        const auto label_plot = fmt::format("{}Plot", label.data());
        if (ImPlot::BeginPlot(label_plot.data(), ImVec2{400.0F, 400.0F}, ImPlotFlags_CanvasOnly))
        {
            const auto next_pos = move_pos;
            const auto rect = GameRectangle(player_pos, next_pos, GW::Constants::Range::Spellcast);

            const auto flags_axis =
                ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels;
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_None, ImPlotAxisFlags_Lock);
            ImPlot::SetupAxis(ImAxis_X1, nullptr, flags_axis);
            ImPlot::SetupAxis(ImAxis_Y1, nullptr, flags_axis);
            ImPlot::SetupAxisLimits(ImAxis_X1,
                                    -GW::Constants::Range::Compass,
                                    GW::Constants::Range::Compass,
                                    ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1,
                                    -GW::Constants::Range::Compass,
                                    GW::Constants::Range::Compass,
                                    ImGuiCond_Always);

            PlotPoint(player_pos, player_pos, "player_pos", ImVec4{1.0F, 1.0F, 1.0F, 1.0F}, 5.0F);
            PlotPoint(player_pos, next_pos, "target", ImVec4{0.5F, 0.5F, 0.0F, 1.0F}, 5.0F);

            PlotRectangleLine(player_pos, rect.v1, rect.v2, "line1");
            PlotRectangleLine(player_pos, rect.v1, rect.v3, "line2");
            PlotRectangleLine(player_pos, rect.v4, rect.v2, "line3");
            PlotRectangleLine(player_pos, rect.v4, rect.v3, "line4");

            PlotCircle(player_pos, "circle", ImVec4{0.0, 0.0, 1.0, 1.0});

            PlotEnemies(player_pos, enemies, "enemiesAll", ImVec4{1.0F, 0.65F, 0.0, 1.0});

            const auto filtered_livings = GetEnemiesInGameRectangle(rect, enemies);
            PlotEnemies(player_pos, filtered_livings, "enemyInside", ImVec4{1.0, 0.0, 0.0, 1.0});
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}

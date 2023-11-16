#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/PartyMgr.h>

#include <ActionsBase.h>
#include <ActionsUw.h>
#include <Base/HelperBox.h>
#include <DataPlayer.h>
#include <DataSkillbar.h>
#include <HelperAgents.h>
#include <HelperMaps.h>
#include <HelperUw.h>
#include <HelperUwPos.h>
#include <Utils.h>
#include <UtilsGui.h>
#include <UtilsMath.h>

#include <fmt/format.h>

#include "UwRanger.h"

namespace
{
static constexpr auto HEALING_SPRING_U16 = static_cast<uint16_t>(GW::Constants::SkillID::Healing_Spring);
static constexpr auto MAX_TABLE_LENGTH = 6U;
static auto auto_target_active = false;

static const auto T1_IDS = std::array<uint32_t, 1U>{GW::Constants::ModelID::UW::ColdfireNight};
static const auto T2_IDS = std::array<uint32_t, 1U>{GW::Constants::ModelID::UW::ObsidianBehemoth};
static const auto GENERAL_IDS = std::array<uint32_t, 4U>{GW::Constants::ModelID::UW::TerrorwebDryder,
                                                         GW::Constants::ModelID::UW::FourHorseman,
                                                         GW::Constants::ModelID::UW::SkeletonOfDhuum1,
                                                         GW::Constants::ModelID::UW::SkeletonOfDhuum2};

const auto KING_PATH_LEFT = GW::GamePos{4278.60F, 15898.68F, 0};
const auto KING_PATH_RIGHT = GW::GamePos{5939.21F, 20276.94F, 0};
} // namespace

void AutoTargetAction::Update()
{
    if (!IsExplorable())
    {
        auto_target_active = false;
        action_state = ActionState::INACTIVE;
    }

    if (action_state == ActionState::ACTIVE)
    {
        auto_target_active = true;

        if (behemoth_livings && behemoth_livings->size() == 0)
        {
            auto_target_active = false;
            action_state = ActionState::INACTIVE;
        }
    }
    else
    {
        auto_target_active = false;
        action_state = ActionState::INACTIVE;
    }
}

RoutineState AutoTargetAction::Routine()
{
    return RoutineState::NONE;
}

void UwRanger::DrawSplittedAgents(std::vector<GW::AgentLiving *> livings,
                                  const ImVec4 color,
                                  std::string_view label,
                                  const bool draw_time)
{
    auto idx = uint32_t{0};

    for (const auto living : livings)
    {
        if (!living || living->hp == 0.0F || living->GetIsDead())
            continue;

        ImGui::TableNextRow();

        if (living->player_number == static_cast<uint32_t>(GW::Constants::ModelID::UW::ObsidianBehemoth) &&
            living->GetIsCasting() && living->skill == HEALING_SPRING_U16)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1F, 0.9F, 0.1F, 1.0F));
            last_casted_times_ms[living->agent_id] = clock();
        }
        else if (!draw_time &&
                 living->player_number == static_cast<uint32_t>(GW::Constants::ModelID::UW::ColdfireNight) &&
                 (GW::GetDistance(player_data.pos, living->pos) > 1800.0F || living->GetIsAttacking()))
        {
            if (GW::GetDistance(player_data.pos, living->pos) > 1800.0F)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.0F, 0.0F, 1.0F));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1F, 0.9F, 0.1F, 1.0F));
            last_casted_times_ms[living->agent_id] = clock();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, color);
        }
        const auto distance = GW::GetDistance(player_data.pos, living->pos);
        ImGui::TableNextColumn();
        ImGui::Text("%3.0f%%", living->hp * 100.0F);
        ImGui::TableNextColumn();
        ImGui::Text("%4.0f", distance);

        if (draw_time)
        {
            ImGui::TableNextColumn();
            const auto timer_diff_ms = TIMER_DIFF(last_casted_times_ms[living->agent_id]);
            const auto timer_diff_s = timer_diff_ms / 1000;
            if (timer_diff_s > 40 ||
                living->player_number != static_cast<uint32_t>(GW::Constants::ModelID::UW::ObsidianBehemoth))
            {
                ImGui::Text(" - ");
            }
            else
            {
                ImGui::Text("%2ds", timer_diff_s);
            }
        }

        ImGui::PopStyleColor();
        ImGui::TableNextColumn();
        const auto _label = fmt::format("Target##{}{}", label.data(), idx);
        if (ImGui::Button(_label.data()))
            player_data.ChangeTarget(living->agent_id);

        ++idx;

        if (idx >= MAX_TABLE_LENGTH)
            break;
    }
}

void UwRanger::Draw()
{
    if (!visible)
        return;

    if (!UwHelperActivationConditions(false))
        return;
    if (!IsRangerTerra(player_data))
        return;

    ImGui::SetNextWindowSize(ImVec2(200.0F, 240.0F), ImGuiCond_FirstUseEver);

    if (ImGui::Begin(Name(), nullptr, GetWinFlags()))
    {
        const auto width = ImGui::GetWindowWidth();
        auto_target.Draw(ImVec2(width, 35.0F));

        if (ImGui::BeginTable("RangerTable", 4))
        {
            ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthFixed, width * 0.15F);
            ImGui::TableSetupColumn("Dist.", ImGuiTableColumnFlags_WidthFixed, width * 0.2F);
            ImGui::TableSetupColumn("Cast.", ImGuiTableColumnFlags_WidthFixed, width * 0.2F);
            ImGui::TableSetupColumn("Target", ImGuiTableColumnFlags_WidthFixed, width * 0.4F);

            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            ImGui::TableNextColumn();
            ImGui::Text("HP");
            ImGui::TableNextColumn();
            ImGui::Text("Dist.");
            ImGui::TableNextColumn();
            ImGui::Text("Cast.");
            ImGui::TableNextColumn();
            ImGui::Text("Target");

            DrawSplittedAgents(horseman_livings, ImVec4(0.568F, 0.239F, 1.0F, 1.0F), "Horseman");
            DrawSplittedAgents(coldfire_livings, ImVec4(0.7F, 0.7F, 1.0F, 1.0F), "Coldfire");
            DrawSplittedAgents(behemoth_livings, ImVec4(1.0F, 1.0F, 1.0F, 1.0F), "Behemoth");
            DrawSplittedAgents(dryder_livings, ImVec4(0.94F, 0.31F, 0.09F, 1.0F), "Dryder");
            DrawSplittedAgents(skele_livings, ImVec4(0.1F, 0.8F, 0.9F, 1.0F), "Skele");
        }
        ImGui::EndTable();
    }
    ImGui::End();

    if (king_path_coldfire_ids.size())
    {
        auto king_coldfire_livings = std::vector<GW::AgentLiving *>{};
        for (const auto id : king_path_coldfire_ids)
        {
            const auto agent = GW::Agents::GetAgentByID(id);
            if (!agent)
                continue;

            const auto living = agent->GetAsAgentLiving();
            if (!living)
                continue;

            king_coldfire_livings.push_back(living);
        }

#ifdef _DEBUG
        if (king_coldfire_livings.size())
        {
            if (ImGui::Begin("KingColdies"))
            {
                const auto width = ImGui::GetWindowWidth();
                if (ImGui::BeginTable("KingColdiesTable", 3))
                {
                    ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthFixed, width * 0.25F);
                    ImGui::TableSetupColumn("Dist.", ImGuiTableColumnFlags_WidthFixed, width * 0.25F);
                    ImGui::TableSetupColumn("Target", ImGuiTableColumnFlags_WidthFixed, width * 0.50F);

                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::Text("HP");
                    ImGui::TableNextColumn();
                    ImGui::Text("Dist.");
                    ImGui::TableNextColumn();
                    ImGui::Text("Target");

                    DrawSplittedAgents(king_coldfire_livings, ImVec4(0.7F, 0.7F, 1.0F, 1.0F), "Coldfire", false);
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }
#endif
    }
}

static bool AddColdfire(const std::vector<uint32_t> &king_path_coldfire_ids,
                        const GW::AgentLiving *coldfire,
                        const GameRectangle &rectangle)
{
    const auto find_it = std::find(king_path_coldfire_ids.begin(), king_path_coldfire_ids.end(), coldfire->agent_id);
    if (!rectangle.PointInGameRectangle(coldfire->pos) && find_it != king_path_coldfire_ids.end())
        return false;

    if (king_path_coldfire_ids.size() == 0)
        return true;

    if (king_path_coldfire_ids.size() >= 3)
        return false;

    const auto center_id = king_path_coldfire_ids[0];
    const auto center_agent = GW::Agents::GetAgentByID(center_id);
    if (!center_agent)
        return false;

    const auto dist_to_center = GW::GetDistance(coldfire->pos, center_agent->pos);
    if (dist_to_center < GW::Constants::Range::Earshot)
        return true;

    return false;
}

void UwRanger::Update(float, const AgentLivingData &livings_data)
{
    filtered_livings.clear();
    coldfire_livings.clear();
    behemoth_livings.clear();
    dryder_livings.clear();
    skele_livings.clear();
    horseman_livings.clear();

    if (!player_data.ValidateData(UwHelperActivationConditions, false))
    {
        last_casted_times_ms.clear();
        return;
    }
    player_data.Update();

    if (!IsRangerTerra(player_data) || IsLoading())
    {
        last_casted_times_ms.clear();
        return;
    }

    const auto &pos = player_data.pos;
    FilterByIdsAndDistances(pos, livings_data.enemies, filtered_livings, T1_IDS, GW::Constants::Range::Compass);
    FilterByIdsAndDistances(pos, livings_data.enemies, filtered_livings, T2_IDS, 800.0F);
    FilterByIdsAndDistances(pos, livings_data.enemies, filtered_livings, GENERAL_IDS, 1500.0F);
    FilterByIdAndDistance(pos, filtered_livings, coldfire_livings, GW::Constants::ModelID::UW::ColdfireNight);
    FilterByIdAndDistance(pos, filtered_livings, behemoth_livings, GW::Constants::ModelID::UW::ObsidianBehemoth);
    FilterByIdAndDistance(pos, filtered_livings, dryder_livings, GW::Constants::ModelID::UW::TerrorwebDryder);
    FilterByIdAndDistance(pos, filtered_livings, skele_livings, GW::Constants::ModelID::UW::SkeletonOfDhuum1);
    FilterByIdAndDistance(pos, filtered_livings, skele_livings, GW::Constants::ModelID::UW::SkeletonOfDhuum2);
    FilterByIdAndDistance(pos, filtered_livings, horseman_livings, GW::Constants::ModelID::UW::FourHorseman);
    SortByDistance(player_data, coldfire_livings);
    SortByDistance(player_data, behemoth_livings);
    SortByDistance(player_data, dryder_livings);
    SortByDistance(player_data, skele_livings);
    SortByDistance(player_data, horseman_livings);

    auto_target.behemoth_livings = &behemoth_livings;
    auto_target.Update();

    if (IsInWastes(player_data.pos, 6000.0F))
    {
        const auto king_path_rectangle =
            GameRectangle(KING_PATH_LEFT, KING_PATH_RIGHT, GW::Constants::Range::Spellcast);

        if (king_path_coldfire_ids.size() < 3)
        {
            for (const auto coldfire : coldfire_livings)
            {
                if (AddColdfire(king_path_coldfire_ids, coldfire, king_path_rectangle))
                    king_path_coldfire_ids.push_back(coldfire->agent_id);
            }
        }

        const auto remove_it = std::remove_if(king_path_coldfire_ids.begin(),
                                              king_path_coldfire_ids.end(),
                                              [](const auto id) { return !GW::Agents::GetAgentByID(id); });
        king_path_coldfire_ids.erase(remove_it, king_path_coldfire_ids.end());
    }

    if (!auto_target_active)
        return;

    for (const auto living : behemoth_livings)
    {
        if (!living)
            continue;

        const auto dist = GW::GetDistance(player_data.pos, living->pos);
        if (dist < GW::Constants::Range::Earshot && living->GetIsCasting() && living->skill == HEALING_SPRING_U16)
        {
            player_data.ChangeTarget(living->agent_id);
            if (attack_at_auto_target && (!player_data.living->GetIsMoving() && !player_data.living->GetIsCasting()) &&
                player_data.target)
                AttackAgent(player_data.target);
        }
    }
}

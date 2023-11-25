#include <array>
#include <cstdint>
#include <ranges>
#include <string_view>
#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>

#include "ActionsBase.h"
#include "ActionsUw.h"
#include "DataPlayer.h"
#include "Helper.h"
#include "HelperAgents.h"
#include "HelperItems.h"
#include "HelperUw.h"
#include "HelperUwPos.h"
#include "Logger.h"
#include "UtilsGui.h"
#include "UtilsMath.h"

#include "UwMesmer.h"

namespace
{
static auto finished_skill = false;

static constexpr auto SPAWN_SPIRIT_ID = uint32_t{2374};
static constexpr auto MAX_TABLE_LENGTH = uint32_t{6U};
static const auto IDS = std::array<uint32_t, 6U>{GW::Constants::ModelID::UW::BladedAatxe,
                                                 GW::Constants::ModelID::UW::DyingNightmare,
                                                 GW::Constants::ModelID::UW::TerrorwebDryder,
                                                 GW::Constants::ModelID::UW::FourHorseman,
                                                 GW::Constants::ModelID::UW::SkeletonOfDhuum1,
                                                 GW::Constants::ModelID::UW::SkeletonOfDhuum2};
} // namespace

bool LtRoutine::DoNeedVisage() const
{
    const auto closest_id = GetClosestToPosition(player_data->pos, enemies_in_aggro, 0);
    const auto closest_enemy = GW::Agents::GetAgentByID(closest_id);
    if (!closest_enemy)
        return false;

    const auto closest_enemy_living = closest_enemy->GetAsAgentLiving();
    if (!closest_enemy_living)
        return false;

    const auto enemies_in_range = (aatxes.size() || graspings.size());
    const auto spike_almost_done = closest_enemy_living->hp < 0.30F;

    return (enemies_in_range && spike_almost_done);
}

bool LtRoutine::RoutineSelfEnches() const
{
    const auto need_obsi = nightmares.size();
    const auto need_stoneflesh = (aatxes.size() || graspings.size());
    const auto need_mantra = (aatxes.size() || graspings.size());
    const auto need_visage = DoNeedVisage();

    if (need_obsi && DoNeedEnchNow(player_data, GW::Constants::SkillID::Obsidian_Flesh, 2.0F) &&
        (RoutineState::FINISHED == skillbar->obsi.Cast(player_data->energy)))
    {
        return true;
    }

    if (need_stoneflesh && DoNeedEnchNow(player_data, GW::Constants::SkillID::Stoneflesh_Aura, 2.0F) &&
        (RoutineState::FINISHED == skillbar->stoneflesh.Cast(player_data->energy)))
    {
        return true;
    }

    if (need_mantra && DoNeedEnchNow(player_data, GW::Constants::SkillID::Mantra_of_Resolve, 0.0F) &&
        (RoutineState::FINISHED == skillbar->mantra_of_resolve.Cast(player_data->energy)))
    {
        return true;
    }

    if (need_visage && DoNeedEnchNow(player_data, GW::Constants::SkillID::Sympathetic_Visage, 0.0F) &&
        (RoutineState::FINISHED == skillbar->visage.Cast(player_data->energy)))
    {
        return true;
    }

    return false;
}

RoutineState LtRoutine::Routine()
{
    static auto gone_to_npc = false;
    static auto took_quest = false;

    if (!IsUw())
    {
        return RoutineState::FINISHED;
    }

    if (gone_to_npc)
    {
        delay_ms = 475L;
    }

    if (!ActionABC::HasWaitedLongEnough(delay_ms))
    {
        return RoutineState::ACTIVE;
    }

    if (starting_active)
    {
        const auto agent_id = GetClosestNpcbyId(*player_data, livings_data->npcs, SPAWN_SPIRIT_ID);
        if (!agent_id)
        {
            starting_active = false;
            gone_to_npc = false;
            took_quest = false;
            return RoutineState::FINISHED;
        }

        if (!player_data->target)
        {
            if (agent_id)
            {
                player_data->ChangeTarget(agent_id);
                if (RoutineState::FINISHED == skillbar->ebon.Cast(player_data->energy, agent_id))
                {
                    return RoutineState::FINISHED;
                }
            }
        }
        else
        {
            if (!gone_to_npc)
            {
                const auto agent = GW::Agents::GetAgentByID(agent_id);
                GW::Agents::GoNPC(agent, 0U);
                gone_to_npc = true;
                took_quest = false;
                return RoutineState::ACTIVE;
            }
            if (gone_to_npc && !took_quest)
            {
                const auto took_chamber = TakeChamber();
                took_quest = true;
                return RoutineState::ACTIVE;
            }
            const auto took_chamber = TakeChamber();
            Log::Info("Took chamber: %u", (int)took_chamber);

            if (skillbar->stoneflesh.CanBeCasted(player_data->energy) &&
                RoutineState::FINISHED == skillbar->stoneflesh.Cast(player_data->energy))
            {
                return RoutineState::FINISHED;
            }
            if (skillbar->mantra_of_resolve.CanBeCasted(player_data->energy) &&
                RoutineState::FINISHED == skillbar->mantra_of_resolve.Cast(player_data->energy))
            {
                return RoutineState::FINISHED;
            }
            if (skillbar->visage.CanBeCasted(player_data->energy) &&
                RoutineState::FINISHED == skillbar->visage.Cast(player_data->energy))
            {
                return RoutineState::FINISHED;
            }
            if (skillbar->obsi.CanBeCasted(player_data->energy) &&
                RoutineState::FINISHED == skillbar->obsi.Cast(player_data->energy))
            {
                return RoutineState::FINISHED;
            }

            starting_active = false;
            gone_to_npc = false;
            took_quest = false;
            action_state = ActionState::INACTIVE;
        }
    }

    return RoutineState::FINISHED;
}

void LtRoutine::Update()
{
    static auto paused = false;

    if (GW::PartyMgr::GetIsPartyDefeated())
    {
        action_state = ActionState::INACTIVE;
    }

    if (action_state == ActionState::ACTIVE && PauseRoutine())
    {
        paused = true;
        action_state = ActionState::ON_HOLD;
    }

    if (action_state == ActionState::ON_HOLD && ResumeRoutine())
    {
        paused = false;
        action_state = ActionState::ACTIVE;
    }

#ifdef _DEBUG
    if (IsOnSpawnPlateau(player_data->pos) && !TankIsFullteamLT() && !player_data->target && load_cb_triggered)
    {
        starting_active = true;
        action_state = ActionState::ACTIVE;
    }
#endif

    if (action_state == ActionState::ACTIVE)
    {
        (void)Routine();
    }
}

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static UwMesmer instance;
    return &instance;
}

void UwMesmer::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);
    WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"UwMesmer");

    skillbar.Initialize();
    uw_metadata.Initialize();
}

void UwMesmer::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

void UwMesmer::DrawSplittedAgents(std::vector<GW::AgentLiving *> livings, const ImVec4 color, std::string_view label)
{
    auto idx = uint32_t{0};

    for (const auto living : livings)
    {
        if (!living)
        {
            continue;
        }

        ImGui::TableNextRow();

        if (living->hp == 0.0F || living->GetIsDead())
        {
            continue;
        }

        if ((living->player_number == static_cast<uint32_t>(GW::Constants::ModelID::UW::BladedAatxe) ||
             living->player_number == static_cast<uint32_t>(GW::Constants::ModelID::UW::FourHorseman)) &&
            living->GetIsHexed())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8F, 0.0F, 0.2F, 1.0F));
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
        ImGui::PopStyleColor();

        const auto _label = std::format("Target##{}{}", label.data(), idx);
        ImGui::TableNextColumn();
        if (ImGui::Button(_label.data()))
        {
            player_data.ChangeTarget(living->agent_id);
        }

        ++idx;

        if (idx >= MAX_TABLE_LENGTH)
        {
            break;
        }
    }
}

void UwMesmer::Draw(IDirect3DDevice9 *)
{
    if (!player_data.ValidateData(UwHelperActivationConditions, false) || !IsUwMesmer(player_data))
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(200.0F, 240.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(),
                     can_close && show_closebutton ? GetVisiblePtr() : nullptr,
                     GetWinFlags(ImGuiWindowFlags_NoResize)))
    {
        const auto width = ImGui::GetWindowWidth();

        if (ImGui::BeginTable("AatxeTable", 3))
        {
            ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthFixed, width * 0.27F);
            ImGui::TableSetupColumn("Dist.", ImGuiTableColumnFlags_WidthFixed, width * 0.25F);
            ImGui::TableSetupColumn("Target", ImGuiTableColumnFlags_WidthFixed, width * 0.48F);

            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            ImGui::TableNextColumn();
            ImGui::Text("HP");
            ImGui::TableNextColumn();
            ImGui::Text("Dist.");
            ImGui::TableNextColumn();
            ImGui::Text("Target");

            DrawSplittedAgents(horseman_livings, ImVec4(0.568F, 0.239F, 1.0F, 1.0F), "Horseman");
            DrawSplittedAgents(aatxe_livings, ImVec4(1.0F, 1.0F, 1.0F, 1.0F), "Aatxe");
            DrawSplittedAgents(nightmare_livings, ImVec4(0.6F, 0.4F, 1.0F, 1.0F), "Nightmare");
            DrawSplittedAgents(keeper_livings, ImVec4(0.90F, 0.35F, 0.09F, 1.0F), "Keeper");
            DrawSplittedAgents(dryder_livings, ImVec4(0.94F, 0.31F, 0.09F, 1.0F), "Dryder");
            DrawSplittedAgents(skele_livings, ImVec4(0.1F, 0.8F, 0.9F, 1.0F), "Skele");
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void UwMesmer::Update(float)
{
    filtered_livings.clear();
    aatxe_livings.clear();
    nightmare_livings.clear();
    dryder_livings.clear();
    skele_livings.clear();
    horseman_livings.clear();
    keeper_livings.clear();

    if (!player_data.ValidateData(UwHelperActivationConditions, false))
    {
        return;
    }

    livings_data.Update();
    player_data.Update();

    if (!IsSpiker(player_data) && !IsLT(player_data))
    {
        return;
    }

    if (TankIsSoloLT())
    {
        if (!skillbar.ValidateData())
        {
            return;
        }

        skillbar.Update();
    }

    const auto &pos = player_data.pos;
    lt_routine.livings_data = &livings_data;
    lt_routine.load_cb_triggered = uw_metadata.load_cb_triggered;

    FilterByIdsAndDistances(pos, livings_data.enemies, filtered_livings, IDS, 1600.0F);
    FilterByIdAndDistance(pos, filtered_livings, aatxe_livings, GW::Constants::ModelID::UW::BladedAatxe);
    FilterByIdAndDistance(pos, filtered_livings, nightmare_livings, GW::Constants::ModelID::UW::DyingNightmare);
    FilterByIdAndDistance(pos, filtered_livings, dryder_livings, GW::Constants::ModelID::UW::TerrorwebDryder);
    FilterByIdAndDistance(pos, filtered_livings, horseman_livings, GW::Constants::ModelID::UW::FourHorseman);
    FilterByIdAndDistance(pos, filtered_livings, keeper_livings, GW::Constants::ModelID::UW::KeeperOfSouls);
    FilterByIdAndDistance(pos, filtered_livings, skele_livings, GW::Constants::ModelID::UW::SkeletonOfDhuum1);
    FilterByIdAndDistance(pos, filtered_livings, skele_livings, GW::Constants::ModelID::UW::SkeletonOfDhuum2);
    SortByDistance(player_data, aatxe_livings);
    SortByDistance(player_data, nightmare_livings);
    SortByDistance(player_data, horseman_livings);
    SortByDistance(player_data, keeper_livings);
    SortByDistance(player_data, dryder_livings);
    SortByDistance(player_data, skele_livings);

    if (TankIsSoloLT())
    {
        lt_routine.Update();
    }
}

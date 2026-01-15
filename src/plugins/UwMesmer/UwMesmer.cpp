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
#include <GWCA/Utilities/Hooker.h>

#include "ActionsBase.h"
#include "ActionsUw.h"
#include "HelperPlayer.h"
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

constexpr auto SPAWN_SPIRIT_ID = uint32_t{2374};
constexpr auto MAX_TABLE_LENGTH = uint32_t{6U};
static const auto IDS = std::array<uint32_t, 6U>{GW::Constants::ModelID::UW::BladedAatxe,
                                                 GW::Constants::ModelID::UW::DyingNightmare,
                                                 GW::Constants::ModelID::UW::TerrorwebDryder,
                                                 GW::Constants::ModelID::UW::FourHorseman,
                                                 GW::Constants::ModelID::UW::SkeletonOfDhuum1,
                                                 GW::Constants::ModelID::UW::SkeletonOfDhuum2};
} // namespace

bool LtRoutine::DoNeedVisage() const
{
    const auto player_pos = GetPlayerPos();
    const auto closest_id = GetClosestToPosition(player_pos, enemies_in_aggro, 0);
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

    const auto player_energy = GetEnergy();
    if (need_obsi && DoNeedEnchNow(GW::Constants::SkillID::Obsidian_Flesh, 2.0F) &&
        (RoutineState::FINISHED == skillbar->obsi.Cast(player_energy)))
    {
        return true;
    }

    if (need_stoneflesh && DoNeedEnchNow(GW::Constants::SkillID::Stoneflesh_Aura, 2.0F) &&
        (RoutineState::FINISHED == skillbar->stoneflesh.Cast(player_energy)))
    {
        return true;
    }

    if (need_mantra && DoNeedEnchNow(GW::Constants::SkillID::Mantra_of_Resolve, 0.0F) &&
        (RoutineState::FINISHED == skillbar->mantra_of_resolve.Cast(player_energy)))
    {
        return true;
    }

    if (need_visage && DoNeedEnchNow(GW::Constants::SkillID::Sympathetic_Visage, 0.0F) &&
        (RoutineState::FINISHED == skillbar->visage.Cast(player_energy)))
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
        return RoutineState::FINISHED;

    if (gone_to_npc)
        delay_ms = 475L;

    if (!ActionABC::HasWaitedLongEnough(delay_ms))
        return RoutineState::ACTIVE;

    if (starting_active)
    {
        const auto agent_id = GetClosestNpcbyId(livings_data->npcs, SPAWN_SPIRIT_ID);
        if (!agent_id)
        {
            starting_active = false;
            gone_to_npc = false;
            took_quest = false;
            return RoutineState::FINISHED;
        }

        const auto target = GW::Agents::GetTarget();
        if (!target)
        {
            if (agent_id)
            {
                const auto player_energy = GetEnergy();
                ChangeTarget(agent_id);
                if (RoutineState::FINISHED == skillbar->ebon.Cast(player_energy, agent_id))
                    return RoutineState::FINISHED;
            }
        }
        else
        {
            if (!gone_to_npc)
            {
                const auto agent = GW::Agents::GetAgentByID(agent_id);
                GW::Agents::InteractAgent(agent, 0U);
                gone_to_npc = true;
                took_quest = false;
                return RoutineState::ACTIVE;
            }
            if (gone_to_npc && !took_quest)
            {
                TakeChamber();
                took_quest = true;
                return RoutineState::ACTIVE;
            }
            const auto took_chamber = TakeChamber();
            Log::Info("Took chamber: %u", (int)took_chamber);

            const auto player_energy = GetEnergy();
            if (skillbar->stoneflesh.CanBeCasted(player_energy) &&
                RoutineState::FINISHED == skillbar->stoneflesh.Cast(player_energy))
            {
                return RoutineState::FINISHED;
            }
            if (skillbar->mantra_of_resolve.CanBeCasted(player_energy) &&
                RoutineState::FINISHED == skillbar->mantra_of_resolve.Cast(player_energy))
            {
                return RoutineState::FINISHED;
            }
            if (skillbar->visage.CanBeCasted(player_energy) &&
                RoutineState::FINISHED == skillbar->visage.Cast(player_energy))
            {
                return RoutineState::FINISHED;
            }
            if (skillbar->obsi.CanBeCasted(player_energy) &&
                RoutineState::FINISHED == skillbar->obsi.Cast(player_energy))
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
        action_state = ActionState::INACTIVE;

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
    const auto target = GW::Agents::GetTarget();
    const auto player_pos = GetPlayerPos();
    if (IsOnSpawnPlateau(player_pos) && !TankIsFullteamLT() && !target) // && load_cb_triggered)
    {
        starting_active = true;
        action_state = ActionState::ACTIVE;
    }
#endif

    if (action_state == ActionState::ACTIVE)
        (void)Routine();
}

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static UwMesmer instance;
    return &instance;
}

void UwMesmer::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);
    if (!GW::Initialize())
    {
        GW::Terminate();
        return;
    }

    skillbar.Initialize();
    uw_metadata.Initialize();

    WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"UwMesmer");
}

void UwMesmer::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

bool UwMesmer::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void UwMesmer::Terminate()
{
    ToolboxPlugin::Terminate();
    GW::StoC::RemoveCallbacks(&uw_metadata.MapLoaded_Entry);
    GW::StoC::RemoveCallbacks(&uw_metadata.SendChat_Entry);
    GW::StoC::RemoveCallbacks(&uw_metadata.ObjectiveDone_Entry);
}

void UwMesmer::DrawSplittedAgents(std::vector<GW::AgentLiving *> livings, const ImVec4 color, std::string_view label)
{
    auto idx = uint32_t{0};

    for (const auto living : livings)
    {
        if (!living)
            continue;

        ImGui::TableNextRow();

        if (living->hp == 0.0F || living->GetIsDead())
            continue;

        if ((living->player_number == static_cast<uint32_t>(GW::Constants::ModelID::UW::BladedAatxe) ||
             living->player_number == static_cast<uint32_t>(GW::Constants::ModelID::UW::FourHorseman)) &&
            living->GetIsHexed())
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8F, 0.0F, 0.2F, 1.0F));
        else
            ImGui::PushStyleColor(ImGuiCol_Text, color);

        const auto player_pos = GetPlayerPos();
        const auto distance = GW::GetDistance(player_pos, living->pos);
        ImGui::TableNextColumn();
        ImGui::Text("%3.0f%%", living->hp * 100.0F);
        ImGui::TableNextColumn();
        ImGui::Text("%4.0f", distance);
        ImGui::PopStyleColor();

        const auto _label = std::format("Target##{}{}", label.data(), idx);
        ImGui::TableNextColumn();
        if (ImGui::Button(_label.data()))
            ChangeTarget(living->agent_id);

        ++idx;

        if (idx >= MAX_TABLE_LENGTH)
            break;
    }
}

void UwMesmer::Draw(IDirect3DDevice9 *)
{
    if (!ValidateData(UwHelperActivationConditions, true) || !IsUwMesmer())
        return;

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

    if (!ValidateData(UwHelperActivationConditions, true))
        return;

    livings_data.Update();

    if (!IsSpiker() && !IsLT())
        return;

    if (TankIsSoloLT())
    {
        if (!skillbar.ValidateData())
            return;

        skillbar.Update();
    }

    const auto player_pos = GetPlayerPos();
    const auto &pos = player_pos;
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
    SortByDistance(aatxe_livings);
    SortByDistance(nightmare_livings);
    SortByDistance(horseman_livings);
    SortByDistance(keeper_livings);
    SortByDistance(dryder_livings);
    SortByDistance(skele_livings);

    if (TankIsSoloLT())
        lt_routine.Update();
}

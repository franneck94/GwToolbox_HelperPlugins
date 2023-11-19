#include <cmath>
#include <cstdint>
#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/Context/ItemContext.h>
#include <GWCA/Context/WorldContext.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/CtoSMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Packets/Opcodes.h>

#include "ActionsBase.h"
#include "ActionsUw.h"
#include "DataPlayer.h"
#include "DataSkillbar.h"
#include "Helper.h"
#include "HelperAgents.h"
#include "HelperItems.h"
#include "HelperUw.h"
#include "HelperUwPos.h"
#include "Logger.h"
#include "Utils.h"
#include "UtilsGui.h"
#include "UtilsMath.h"

#include <SimpleIni.h>
#include <fmt/format.h>
#include <imgui.h>

#include "UwDhuumBitch.h"

namespace
{
static auto move_ongoing = false;
static ActionState *damage_action_state = nullptr;

constexpr static auto COOKIE_ID = uint32_t{28433};
constexpr static auto VAMPIRISMUS_AGENT_ID = uint32_t{5723};
constexpr static auto SOS1_AGENT_ID = uint32_t{4229};
constexpr static auto SOS2_AGENT_ID = uint32_t{4230};
constexpr static auto SOS3_AGENT_ID = uint32_t{4231};

const static auto reaper_moves =
    std::map<std::string, uint32_t>{{"Lab", 20}, {"Pits", 43}, {"Planes", 50}, {"Wastes", 58}};

const static auto full_team_moves = std::array<uint32_t, 10>{19, 20, 21, 22, 58, 59, 60, 61, 62, 63};
}; // namespace

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static UwDhuumBitch instance;
    return &instance;
}

void UwDhuumBitch::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);

    WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"UwDhuumBitch");
}

void UwDhuumBitch::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

void UwDhuumBitch::LoadSettings(const wchar_t *folder)
{
    ToolboxUIPlugin::LoadSettings(folder);
    show_debug_map = ini.GetBoolValue(Name(), VAR_NAME(show_debug_map), show_debug_map);
}

void UwDhuumBitch::SaveSettings(const wchar_t *folder)
{
    ToolboxUIPlugin::SaveSettings(folder);
    ini.SetBoolValue(Name(), VAR_NAME(show_debug_map), show_debug_map);
    PLUGIN_ASSERT(ini.SaveFile(GetSettingFile(folder).c_str()) == SI_OK);
}

void UwDhuumBitch::DrawSettings()
{
    ToolboxUIPlugin::DrawSettings();
    const auto width = ImGui::GetWindowWidth();
    ImGui::Text("Show Debug Map:");
    ImGui::SameLine(width * 0.5F);
    ImGui::PushItemWidth(width * 0.5F);
    ImGui::Checkbox("debugMapActive", &show_debug_map);
    ImGui::PopItemWidth();
}

UwDhuumBitch::UwDhuumBitch() : skillbar({}), db_routine(&player_data, &skillbar, &livings_data)
{
    if (skillbar.ValidateData())
        skillbar.Load();
};

void UwDhuumBitch::Draw(IDirect3DDevice9 *)
{
    if (!player_data.ValidateData(UwHelperActivationConditions, true) || !IsDhuumBitch(player_data))
        return;

    ImGui::SetNextWindowSize(ImVec2(115.0F, 178.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(),
                     can_close && show_closebutton ? GetVisiblePtr() : nullptr,
                     GetWinFlags(ImGuiWindowFlags_NoResize)))
    {
        db_routine.Draw();
        DrawMovingButtons(moves, move_ongoing, move_idx);
    }
    ImGui::End();

#ifdef _DEBUG
    // if (IsUw() && show_debug_map && livings_data)
    //     DrawMap(player_data.pos, livings_data->enemies, moves[move_idx]->pos, "DbMap");
#endif
}

void UwDhuumBitch::UpdateUw()
{
    UpdateUwEntry();

    MoveABC::SkipNonFullteamMoves(TankIsFullteamLT(), full_team_moves, moves.size(), move_idx);

    MoveABC::UpdatedUwMoves(player_data, &livings_data, moves, move_idx, move_ongoing);

    if (uw_metadata.num_finished_objectives == 10U && !move_ongoing &&
        (moves[move_idx]->name == "Go To Dhuum 1" || moves[move_idx]->name == "Go To Dhuum 2"))
    {
        moves[move_idx]->Execute();
        if (player_data.living->GetIsMoving())
            move_ongoing = true;
    }

    const auto is_hm_trigger_take = (moves[move_idx]->name == "Talk Lab" || moves[move_idx]->name == "Take Planes");
    const auto is_hm_trigger_move =
        (moves[move_idx]->name == "Go Vale Door" || moves[move_idx]->name == "Go Vale Bridge" ||
         moves[move_idx]->name == "Go Vale Entry" || moves[move_idx]->name == "Go Vale House" ||
         moves[move_idx]->name == "Go Vale Center" || moves[move_idx]->name == "Cast EoE 5" ||
         moves[move_idx]->name == "Go To Dhuum 1" || moves[move_idx]->name == "Go To Dhuum 2" ||
         moves[move_idx]->name == "Go To Dhuum 6");
    const auto is_moving = player_data.living->GetIsMoving();

    Move_PositionABC::LtMoveTrigger(uw_metadata.lt_is_ready,
                                    move_ongoing,
                                    is_hm_trigger_take,
                                    is_hm_trigger_move,
                                    is_moving,
                                    moves[move_idx]);
}

void UwDhuumBitch::UpdateUwEntry()
{
    if (uw_metadata.load_cb_triggered && !TankIsSoloLT())
    {
        uw_metadata.load_cb_triggered = false;
        move_idx = 0;
        move_ongoing = false;
    }

    if (uw_metadata.load_cb_triggered)
    {
        move_idx = 0;
        move_ongoing = false;
        moves[0]->Execute();
        uw_metadata.load_cb_triggered = false;
        move_ongoing = true;

        *damage_action_state = ActionState::ACTIVE;
    }
}

void UwDhuumBitch::Update(float)
{
    if (!player_data.ValidateData(UwHelperActivationConditions, true))
    {
        move_idx = 0;
        move_ongoing = false;
        db_routine.action_state = ActionState::INACTIVE;
        return;
    }
    player_data.Update();
    db_routine.livings_data = &livings_data;
    db_routine.num_finished_objectives = uw_metadata.num_finished_objectives;

    if (!IsDhuumBitch(player_data))
        return;

    if (IsUw() && first_frame)
    {
        UpdateUwInfo(reaper_moves, player_data, moves, move_idx, true, move_ongoing);
        first_frame = false;
    }

    if (!skillbar.ValidateData())
        return;
    skillbar.Update();

    damage_action_state = &db_routine.action_state;

    if (IsUw())
    {
        UpdateUwInfo(reaper_moves, player_data, moves, move_idx, false, move_ongoing);
        UpdateUw();
    }

    db_routine.Update();
}

bool DbRoutine::CastPiOnTarget() const
{
    if (!player_data->target)
        return false;

    const auto target_living = player_data->target->GetAsAgentLiving();
    if (!target_living || target_living->allegiance != GW::Constants::Allegiance::Enemy)
        return false;

    const auto dist = GW::GetDistance(player_data->pos, target_living->pos);
    if (dist > GW::Constants::Range::Spellcast)
        return false;

    if (RoutineState::FINISHED == skillbar->pi.Cast(player_data->energy, target_living->agent_id))
        return true;

    return false;
}

bool DbRoutine::RoutineKillSkele() const
{
    if ((!FoundSpirit(*player_data, livings_data->spirits, SOS1_AGENT_ID) ||
         !FoundSpirit(*player_data, livings_data->spirits, SOS2_AGENT_ID) ||
         !FoundSpirit(*player_data, livings_data->spirits, SOS3_AGENT_ID)) &&
        (RoutineState::FINISHED == skillbar->sos.Cast(player_data->energy)))
        return true;

    if (CastPiOnTarget())
        return true;

    return false;
}

bool DbRoutine::RoutineKillEnemiesStandard() const
{
    const auto found_honor = player_data->HasEffect(GW::Constants::SkillID::Ebon_Battle_Standard_of_Honor);

    if (!found_honor && RoutineState::FINISHED == skillbar->honor.Cast(player_data->energy))
        return true;

    if ((!FoundSpirit(*player_data, livings_data->spirits, SOS1_AGENT_ID) ||
         !FoundSpirit(*player_data, livings_data->spirits, SOS2_AGENT_ID) ||
         !FoundSpirit(*player_data, livings_data->spirits, SOS3_AGENT_ID)) &&
        (RoutineState::FINISHED == skillbar->sos.Cast(player_data->energy)))
        return true;

    if (!FoundSpirit(*player_data, livings_data->spirits, VAMPIRISMUS_AGENT_ID) &&
        (RoutineState::FINISHED == skillbar->vamp.Cast(player_data->energy)))
        return true;

    if (CastPiOnTarget())
        return true;

    return false;
}

bool DbRoutine::RoutineValeSpirits() const
{
    const auto found_honor = player_data->HasEffect(GW::Constants::SkillID::Ebon_Battle_Standard_of_Honor);
    const auto found_eoe = player_data->HasEffect(GW::Constants::SkillID::Edge_of_Extinction);
    const auto found_winnow = player_data->HasEffect(GW::Constants::SkillID::Winnowing);

    if (!found_honor && RoutineState::FINISHED == skillbar->honor.Cast(player_data->energy))
        return true;

    if ((!FoundSpirit(*player_data, livings_data->spirits, SOS1_AGENT_ID) ||
         !FoundSpirit(*player_data, livings_data->spirits, SOS2_AGENT_ID) ||
         !FoundSpirit(*player_data, livings_data->spirits, SOS3_AGENT_ID)) &&
        (RoutineState::FINISHED == skillbar->sos.Cast(player_data->energy)))
        return true;

    if (!FoundSpirit(*player_data, livings_data->spirits, VAMPIRISMUS_AGENT_ID) &&
        (RoutineState::FINISHED == skillbar->vamp.Cast(player_data->energy)))
        return true;

    if (!found_eoe && player_data->energy >= 30U && RoutineState::FINISHED == skillbar->eoe.Cast(player_data->energy))
        return true;

    if (!found_winnow && RoutineState::FINISHED == skillbar->winnow.Cast(player_data->energy))
        return true;

    return false;
}

bool DbRoutine::RoutineDhuumRecharge() const
{
    static auto qz_timer = clock();

    const auto found_qz = player_data->HasEffect(GW::Constants::SkillID::Quickening_Zephyr);

    const auto qz_diff_ms = TIMER_DIFF(qz_timer);
    if (qz_diff_ms > 36'000 || !found_qz)
    {
        if (!found_qz && RoutineState::FINISHED == skillbar->sq.Cast(player_data->energy))
            return true;

        if (RoutineState::FINISHED == skillbar->qz.Cast(player_data->energy))
        {
            qz_timer = clock();
            return true;
        }
    }

    return false;
}

bool DbRoutine::RoutineDhuumDamage() const
{
    const auto found_honor = player_data->HasEffect(GW::Constants::SkillID::Ebon_Battle_Standard_of_Honor);
    const auto found_winnow = player_data->HasEffect(GW::Constants::SkillID::Winnowing);

    if (!found_honor && player_data->energy > 33U &&
        RoutineState::FINISHED == skillbar->honor.Cast(player_data->energy))
        return true;

    if (!found_winnow && RoutineState::FINISHED == skillbar->winnow.Cast(player_data->energy))
        return true;

    if ((!FoundSpirit(*player_data, livings_data->spirits, SOS1_AGENT_ID) ||
         !FoundSpirit(*player_data, livings_data->spirits, SOS2_AGENT_ID) ||
         !FoundSpirit(*player_data, livings_data->spirits, SOS3_AGENT_ID)) &&
        (RoutineState::FINISHED == skillbar->sos.Cast(player_data->energy)))
        return true;

    return false;
}

RoutineState DbRoutine::Routine()
{
    const auto is_in_dhuum_room = IsInDhuumRoom(player_data->pos);
    const auto is_in_dhuum_fight = IsInDhuumFight(player_data->pos);
    const auto dhuum_Fight_done = DhuumFightDone(num_finished_objectives);

    if (is_in_dhuum_fight)
        move_ongoing = false;

    if (!IsUw() || dhuum_Fight_done)
    {
        action_state = ActionState::INACTIVE;
        move_ongoing = false;
        return RoutineState::FINISHED;
    }

    if (!player_data->CanCast() || !livings_data)
        return RoutineState::ACTIVE;

    if ((move_ongoing && !ActionABC::HasWaitedLongEnough(500L)) ||
        (!move_ongoing && !ActionABC::HasWaitedLongEnough(250L)))
        return RoutineState::ACTIVE;

    if (IsUw() && IsOnSpawnPlateau(player_data->pos, 300.0F) && !TankIsSoloLT())
    {
        action_state = ActionState::INACTIVE;
        return RoutineState::FINISHED;
    }

    if (!TankIsSoloLT() && !is_in_dhuum_room)
        return RoutineState::FINISHED;

    if (IsAtChamberSkele(player_data->pos) || IsAtBasementSkele(player_data->pos) ||
        IsRightAtValeHouse(player_data->pos))
    {
        const auto enemies = FilterAgentsByRange(livings_data->enemies, *player_data, GW::Constants::Range::Earshot);
        if (enemies.size() == 0)
            return RoutineState::ACTIVE;

        if (!player_data->living->GetIsAttacking() && player_data->CanAttack())
            TargetAndAttackEnemyInAggro(*player_data, livings_data->enemies, GW::Constants::Range::Earshot);

        if (RoutineKillSkele())
            return RoutineState::FINISHED;
    }

    // If mindblades were not stucked by LT, or back patrol aggro
    if (IsAtFusePulls(player_data->pos) || InBackPatrolArea(player_data->pos))
    {
        const auto enemies = FilterAgentsByRange(livings_data->enemies, *player_data, GW::Constants::Range::Earshot);
        if (enemies.size() == 0)
            return RoutineState::ACTIVE;
        RoutineKillEnemiesStandard();
    }

    if (IsAtValeSpirits(player_data->pos))
    {
        SwapToMeleeSet();

        const auto enemies = FilterAgentsByRange(livings_data->enemies, *player_data, 1700.0F);
        if (enemies.size() == 0)
            return RoutineState::ACTIVE;

        if (!player_data->living->GetIsAttacking() && player_data->CanAttack())
            TargetAndAttackEnemyInAggro(*player_data, livings_data->enemies, 1700.0F);

        if (RoutineValeSpirits())
            return RoutineState::FINISHED;
    }
    else
    {
        SwapToRangeSet();
    }

    if (!is_in_dhuum_room)
        return RoutineState::FINISHED;

    if (is_in_dhuum_fight && RoutineDhuumRecharge())
        return RoutineState::FINISHED;

    const auto dhuum_agent = GetDhuumAgent();
    if (!is_in_dhuum_fight || !dhuum_agent)
        return RoutineState::FINISHED;

    const auto world_context = GW::GetWorldContext();
    const auto item_context = GW::GetItemContext();
    if (world_context && item_context)
    {
        static auto last_time_cookie = clock();
        if (TIMER_DIFF(last_time_cookie) > 500 && world_context->morale < 90 &&
            UseInventoryItem(COOKIE_ID, 1, item_context->bags_array.size()))
        {
            last_time_cookie = clock();
            return RoutineState::FINISHED;
        }
    }

    const auto dhuum_dist = GW::GetDistance(player_data->pos, dhuum_agent->pos);
    if (dhuum_dist > GW::Constants::Range::Spellcast)
        return RoutineState::FINISHED;

    auto dhuum_hp = float{0.0F};
    auto dhuum_max_hp = uint32_t{0U};
    GetDhuumAgentData(dhuum_agent, dhuum_hp, dhuum_max_hp);
    if (dhuum_hp < 0.20F)
        return RoutineState::FINISHED;

    if (dhuum_agent && DhuumIsCastingJudgement(dhuum_agent) &&
        (RoutineState::FINISHED == skillbar->pi.Cast(player_data->energy, dhuum_agent->agent_id)))
        return RoutineState::FINISHED;

    if (RoutineDhuumDamage())
        return RoutineState::FINISHED;

    if (!player_data->living->GetIsAttacking() && player_data->CanAttack())
        TargetAndAttackEnemyInAggro(*player_data, livings_data->enemies, GW::Constants::Range::Earshot);

    return RoutineState::FINISHED;
}

bool DbRoutine::PauseRoutine() noexcept
{
    if (player_data->living->GetIsMoving())
        return true;

    if (player_data->target)
    {
        if (TargetIsReaper(*player_data) && (GW::GetDistance(player_data->pos, player_data->target->pos) < 300.0F))
            return true;
    }

    return false;
}

void DbRoutine::Update()
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

    static auto not_moving_timer = clock();
    if (player_data->living->GetIsMoving())
        not_moving_timer = clock();

    if (!TankIsSoloLT() && TIMER_DIFF(not_moving_timer) < 1000 && action_state == ActionState::ACTIVE)
        action_state = ActionState::ON_HOLD;

    if (action_state == ActionState::ACTIVE)
        (void)Routine();
}

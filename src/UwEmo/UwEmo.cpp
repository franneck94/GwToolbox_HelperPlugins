#include <cstdint>
#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/Context/ItemContext.h>
#include <GWCA/Context/WorldContext.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/CtoSMgr.h>
#include <GWCA/Managers/EffectMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Packets/Opcodes.h>

#include "ActionsBase.h"
#include "ActionsUw.h"
#include "DataPlayer.h"
#include "DataSkillbar.h"
#include "Helper.h"
#include "HelperAgents.h"
#include "HelperUw.h"
#include "HelperUwPos.h"
#include "Logger.h"
#include "Utils.h"
#include "UtilsGui.h"
#include "UtilsMath.h"

#include "UwEmo.h"

namespace
{
static ActionState *emo_casting_action_state = nullptr;
static auto move_ongoing = false;

constexpr auto DHUUM_JUDGEMENT_SKILL_ID = uint32_t{3085U};
constexpr auto CANTHA_IDS = std::array<uint32_t, 4>{GW::Constants::ModelID::SummoningStone::ImperialBarrage,
                                                    GW::Constants::ModelID::SummoningStone::ImperialCripplingSlash,
                                                    GW::Constants::ModelID::SummoningStone::ImperialQuiveringBlade,
                                                    GW::Constants::ModelID::SummoningStone::ImperialTripleChop};
constexpr auto ESCORT_IDS = std::array<uint32_t, 6>{GW::Constants::ModelID::UW::Escort1,
                                                    GW::Constants::ModelID::UW::Escort2,
                                                    GW::Constants::ModelID::UW::Escort3,
                                                    GW::Constants::ModelID::UW::Escort4,
                                                    GW::Constants::ModelID::UW::Escort5,
                                                    GW::Constants::ModelID::UW::Escort6};
constexpr auto CANTHA_STONE_ID = uint32_t{30210};
constexpr auto COOKIE_ID = uint32_t{28433};
constexpr auto SEVEN_MINS_IN_MS = 7LL * 60LL * 1000LL;
constexpr auto EIGHT_MINS_IN_MS = 8LL * 60LL * 1000LL;

const static auto reaper_moves =
    std::map<std::string, uint32_t>{{"Lab", 32}, {"Pits", 45}, {"Planes", 48}, {"Wastes", 50}};

const static auto full_team_moves = std::array<uint32_t, 9>{31, 32, 33, 34, 52, 53, 54, 55};
}; // namespace

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static UwEmo instance;
    return &instance;
}

void UwEmo::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);

    WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"UwEmo");

    emo_routine.Initialize();
    skillbar.Initialize();
    uw_metadata.Initialize();
}

void UwEmo::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

void UwEmo::LoadSettings(const wchar_t *folder)
{
    ToolboxUIPlugin::LoadSettings(folder);
    ini.LoadFile(GetSettingFile(folder).c_str());
    show_debug_map = ini.GetBoolValue(Name(), VAR_NAME(show_debug_map), show_debug_map);
    bag_idx = ini.GetLongValue(Name(), VAR_NAME(bag_idx), bag_idx);
    slot_idx = ini.GetLongValue(Name(), VAR_NAME(slot_idx), slot_idx);
}

void UwEmo::SaveSettings(const wchar_t *folder)
{
    ToolboxUIPlugin::SaveSettings(folder);
    ini.SetBoolValue(Name(), VAR_NAME(show_debug_map), show_debug_map);
    ini.SetLongValue(Name(), VAR_NAME(bag_idx), bag_idx);
    ini.SetLongValue(Name(), VAR_NAME(slot_idx), slot_idx);
    PLUGIN_ASSERT(ini.SaveFile(GetSettingFile(folder).c_str()) == SI_OK);
}

void UwEmo::DrawSettings()
{
    ToolboxUIPlugin::DrawSettings();

    static auto _bag_idx = static_cast<int>(bag_idx);
    static auto _slot_idx = static_cast<int>(slot_idx);

    const auto width = ImGui::GetWindowWidth();
    ImGui::Text("Low HP Armor Slots:");

    ImGui::Text("Bag Idx (starts at 1):");
    ImGui::SameLine(width * 0.5F);
    ImGui::PushItemWidth(width * 0.5F);
    ImGui::InputInt("###inputBagIdx", &_bag_idx, 1, 1);
    ImGui::PopItemWidth();
    bag_idx = _bag_idx;

    ImGui::Text("First Armor Piece Idx (starts at 1):");
    ImGui::SameLine(width * 0.5F);
    ImGui::PushItemWidth(width * 0.5F);
    ImGui::InputInt("###inputStartSlot", &_slot_idx, 1, 1);
    ImGui::PopItemWidth();
    slot_idx = _slot_idx;

    ImGui::Text("Show Debug Map:");
    ImGui::SameLine(width * 0.5F);
    ImGui::PushItemWidth(width * 0.5F);
    ImGui::Checkbox("debugMapActive", &show_debug_map);
    ImGui::PopItemWidth();
}

UwEmo::UwEmo() : skillbar({}), uw_metadata({}), emo_routine(&player_data, &skillbar, &bag_idx, &slot_idx, &livings_data)
{
    if (skillbar.ValidateData())
        skillbar.Load();
};

void UwEmo::Draw(IDirect3DDevice9 *)
{
    if (!player_data.ValidateData(UwHelperActivationConditions, true) || !IsEmo(player_data))
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(115.0F, 180.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(),
                     can_close && show_closebutton ? GetVisiblePtr() : nullptr,
                     GetWinFlags(ImGuiWindowFlags_NoResize)))
    {
        emo_routine.Draw();

        if (IsUw() || IsUwEntryOutpost())
        {
            DrawMovingButtons(moves, move_ongoing, move_idx);
        }
    }
    ImGui::End();

#ifdef _DEBUG
    // if (IsUw() && show_debug_map && livings_data)
    //     DrawMap(player_data.pos, livings_data.enemies, moves[move_idx]->pos, "EmoMap");
#endif
}

void UwEmo::UpdateUw()
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

    const auto is_hm_trigger_take = moves[move_idx]->name == "Talk Lab Reaper";
    const auto is_hm_trigger_move =
        (moves[move_idx]->name == "Go Vale Door" || moves[move_idx]->name == "Go Vale Bridge" ||
         moves[move_idx]->name == "Go Vale Entry" || moves[move_idx]->name == "Go Vale House" ||
         moves[move_idx]->name == "Go Vale Center" || moves[move_idx]->name == "Go To Wastes 1" ||
         moves[move_idx]->name == "Go Wastes 2" || moves[move_idx]->name == "Go To Wastes 5" ||
         moves[move_idx]->name == "Go Wastes 1" || moves[move_idx]->name == "Go To Dhuum 1" ||
         moves[move_idx]->name == "Go To Dhuum 2" || moves[move_idx]->name == "Keeper 3" ||
         moves[move_idx]->name == "Keeper 4/5" || moves[move_idx]->name == "Go Lab 1" ||
         moves[move_idx]->name == "Go To Dhuum 6" || moves[move_idx]->name == "Go Spirits 2" ||
         moves[move_idx]->name == "Go Pits 1" || moves[move_idx]->name == "Go Pits Start" ||
         moves[move_idx]->name == "Go Planes Start");
    const auto is_moving = player_data.living->GetIsMoving();

    Move_PositionABC::LtMoveTrigger(uw_metadata.lt_is_ready,
                                    move_ongoing,
                                    is_hm_trigger_take,
                                    is_hm_trigger_move,
                                    is_moving,
                                    moves[move_idx]);
}

void UwEmo::UpdateUwEntry()
{
    static auto triggered_tank_bonds_at_start = false;

    if (uw_metadata.load_cb_triggered)
    {
        move_idx = 0;
        uw_metadata.num_finished_objectives = 0U;
        move_ongoing = false;
        emo_routine.used_canthas = false;
    }

    if (uw_metadata.load_cb_triggered && !TankIsSoloLT())
    {
        uw_metadata.load_cb_triggered = false;
        emo_routine.action_state = ActionState::ACTIVE;
    }

    if (uw_metadata.load_cb_triggered)
    {
        uw_metadata.load_cb_triggered = false;
        triggered_tank_bonds_at_start = true;
        emo_routine.action_state = ActionState::ACTIVE;
        return;
    }

    if (triggered_tank_bonds_at_start && LtIsBonded())
    {
        moves[0]->Execute();
        triggered_tank_bonds_at_start = false;
        move_ongoing = true;
        return;
    }
}

void UwEmo::Update(float)
{
    if (!player_data.ValidateData(UwHelperActivationConditions, true))
    {
        move_idx = 0;
        move_ongoing = false;
        return;
    }

    player_data.Update();
    emo_routine.livings_data = &livings_data;
    emo_routine.num_finished_objectives = uw_metadata.num_finished_objectives;

    if (!IsEmo(player_data))
    {
        return;
    }

    if (IsUw() && first_frame)
    {
        UpdateUwInfo(reaper_moves, player_data, moves, move_idx, true, move_ongoing);
        first_frame = false;
    }

    if (!skillbar.ValidateData())
    {
        return;
    }

    skillbar.Update();

    emo_casting_action_state = &emo_routine.action_state;

    if (IsUw())
    {
        UpdateUwInfo(reaper_moves, player_data, moves, move_idx, false, move_ongoing);
        UpdateUw();
    }

    emo_routine.Update();
}

EmoRoutine::EmoRoutine(DataPlayer *p,
                       EmoSkillbarData *s,
                       uint32_t *_bag_idx,
                       uint32_t *_slot_idx,
                       const AgentLivingData *a)
    : EmoActionABC(p, "EmoRoutine", s), bag_idx(_bag_idx), slot_idx(_slot_idx), livings_data(a)
{
}

void EmoRoutine::Initialize()
{
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::AgentAdd>(
        &Summon_AgentAdd_Entry,
        [&](GW::HookStatus *, GW::Packet::StoC::AgentAdd *pak) -> void {
            if (pak->type != 1)
                return;

            const auto player_number = (pak->agent_type ^ 0x20000000);

            if (!IsUw() || player_number != GW::Constants::ModelID::SummoningStone::JadeiteSiegeTurtle)
                return;

            found_turtle = true;
            turtle_id = pak->agent_id;
        });
}

bool EmoRoutine::RoutineWhenInRangeBondLT() const
{
    if (!lt_agent)
        return false;

    if (player_data->energy < 40U)
        return false;

    const auto dist = GW::GetDistance(player_data->pos, lt_agent->pos);
    if (dist > GW::Constants::Range::Spellcast)
        return false;

    const auto lt_living = lt_agent->GetAsAgentLiving();
    if (!lt_living || lt_living->GetIsDead() || lt_living->hp == 0.00F)
        return false;

    if (lt_living->GetIsMoving() || player_data->living->GetIsMoving())
        return false;

    if (CastBondIfNotAvailable(skillbar->balth, lt_agent->agent_id, player_data))
        return true;

    if (CastBondIfNotAvailable(skillbar->prot, lt_agent->agent_id, player_data))
        return true;

    if (CastBondIfNotAvailable(skillbar->life, lt_agent->agent_id, player_data))
        return true;

    return false;
}

bool EmoRoutine::RoutineSelfBonds() const
{
    const auto found_ether = player_data->HasEffect(GW::Constants::SkillID::Ether_Renewal);
    const auto found_burning = player_data->HasEffect(GW::Constants::SkillID::Burning_Speed);
    const auto found_sb = player_data->HasEffect(GW::Constants::SkillID::Spirit_Bond);

    const auto need_sb_right_now = DoNeedEnchNow(player_data, GW::Constants::SkillID::Spirit_Bond, 2.0F);
    const auto need_ether_right_now = DoNeedEnchNow(player_data, GW::Constants::SkillID::Ether_Renewal, 3.0F);

    const auto is_full_energy = player_data->energy_perc == 1.0F;
    if (!is_full_energy && (!found_ether || need_ether_right_now) && player_data->CastEffect(skillbar->ether))
        return true;

    if (player_data->energy > 30U)
    {
        if (CastBondIfNotAvailable(skillbar->balth, player_data->id, player_data))
            return true;

        if (CastBondIfNotAvailable(skillbar->prot, player_data->id, player_data))
            return true;
    }

    if (!found_ether)
        return false;

    const auto need_pump = player_data->energy_perc < 0.85F || player_data->hp_perc < 0.80F;
    if ((need_pump || !found_sb || need_sb_right_now) && player_data->CastEffect(skillbar->sb))
        return true;

    if ((need_pump || !found_burning) && player_data->CastEffect(skillbar->burning))
        return true;

    return false;
}

bool EmoRoutine::RoutineEscortSpirits() const
{
    if (!livings_data || livings_data->npcs.size() == 0)
        return false;

    auto spirits_livings = std::vector<GW::AgentLiving *>{};
    FilterByIdsAndDistances(player_data->pos, livings_data->npcs, spirits_livings, ESCORT_IDS);

    if (spirits_livings.size() == 0)
        return false;

    for (const auto spirit : spirits_livings)
    {
        if (!spirit)
            continue;

        if (spirit->hp > 0.90F)
            continue;

        const auto dist = GW::GetDistance(player_data->pos, spirit->pos);
        const auto is_far_away = dist > 2000.0F;

        if (spirit->hp < 0.60F || is_far_away)
            return (RoutineState::FINISHED == skillbar->fuse.Cast(player_data->energy, spirit->agent_id));

        return (RoutineState::FINISHED == skillbar->sb.Cast(player_data->energy, spirit->agent_id));
    }

    return false;
}

bool EmoRoutine::RoutineCanthaGuards() const
{
    static auto last_gdw_idx = 0;

    if (!player_data->CanCast())
        return false;

    if (!livings_data)
        return true;

    const auto filtered_enemies = FilterAgentsByRange(livings_data->enemies, *player_data, 2500.0F);

    auto filtered_canthas = std::vector<GW::AgentLiving *>{};
    FilterByIdsAndDistances(player_data->pos,
                            livings_data->npcs,
                            filtered_canthas,
                            CANTHA_IDS,
                            GW::Constants::Range::Spellcast);

    if (filtered_canthas.size() == 0)
        return false;

    if (filtered_enemies.size() > 0)
    {
        for (const auto cantha : filtered_canthas)
        {
            if (!cantha || cantha->GetIsDead() || cantha->hp == 0.00F)
                continue;

            if (cantha->hp < 0.50F && player_data->hp_perc > 0.50F)
                return (RoutineState::FINISHED == skillbar->fuse.Cast(player_data->energy, cantha->agent_id));
            else if (cantha->hp < 0.80F)
                return (RoutineState::FINISHED == skillbar->sb.Cast(player_data->energy, cantha->agent_id));

            if (!cantha->GetIsWeaponSpelled())
                return (RoutineState::FINISHED == skillbar->gdw.Cast(player_data->energy, cantha->agent_id));
        }
    }

    return false;
}

bool EmoRoutine::RoutineDbAtSpirits() const
{
    if (!db_agent || !db_agent->agent_id)
        return false;

    const auto dist = GW::GetDistance(db_agent->pos, player_data->pos);
    if (dist < GW::Constants::Range::Spellcast)
        return (RoutineState::FINISHED == skillbar->gdw.Cast(player_data->energy, db_agent->agent_id));

    return false;
}

bool EmoRoutine::RoutineLtAtFusePulls() const
{
    static auto last_time_sb_ms = clock();

    if (!lt_agent || !player_data->target || player_data->target->agent_id != lt_agent->agent_id)
        return false;

    const auto target_living = player_data->target->GetAsAgentLiving();
    if (!target_living || target_living->GetIsMoving() || player_data->living->GetIsMoving() ||
        target_living->GetIsDead() || target_living->hp == 0.00F ||
        target_living->primary != static_cast<uint8_t>(GW::Constants::Profession::Mesmer))
        return false;

    const auto dist = GW::GetDistance(player_data->pos, player_data->target->pos);
    const auto min_range_fuse = 1220.0F;
    if (dist < min_range_fuse || dist > GW::Constants::Range::Spellcast)
        return false;

    const auto mindblades = FilterById(livings_data->enemies, GW::Constants::ModelID::UW::MindbladeSpectre);
    if (!mindblades.size())
        return false;

    const auto most_distant_mindblade_id = GetMostDistantEnemy(target_living->pos, mindblades);
    const auto most_distant_mindblade = GW::Agents::GetAgentByID(most_distant_mindblade_id);
    const auto still_need_fuse_pull =
        (most_distant_mindblade && GW::GetDistance(target_living->pos, most_distant_mindblade->pos) > 500.0F);

    if ((still_need_fuse_pull || target_living->hp < 0.90F) && player_data->hp_perc > 0.50F &&
        (RoutineState::FINISHED == skillbar->fuse.Cast(player_data->energy, target_living->agent_id)))
        return true;

    const auto sb_recast_threshold_ms = 6'000L;
    if (TIMER_DIFF(last_time_sb_ms) > sb_recast_threshold_ms &&
        RoutineState::FINISHED == skillbar->sb.Cast(player_data->energy, target_living->agent_id))
    {
        last_time_sb_ms = clock();
        return true;
    }

    return false;
}

bool EmoRoutine::RoutineDbAtDhuum() const
{
    if (!db_agent)
        return false;

    const auto dist = GW::GetDistance(player_data->pos, db_agent->pos);
    if (dist > GW::Constants::Range::Spellcast)
        return false;

    const auto living = db_agent->GetAsAgentLiving();
    if (!living || living->GetIsDead() || living->hp == 0.00F)
        return false;

    if (CastBondIfNotAvailable(skillbar->balth, db_agent->agent_id, player_data))
        return true;

    if (CastBondIfNotAvailable(skillbar->prot, db_agent->agent_id, player_data))
        return true;

    return false;
}

bool EmoRoutine::RoutineTurtle() const
{
    if (!found_turtle || !turtle_id)
        return false;

    const auto turtle_agent = GW::Agents::GetAgentByID(turtle_id);
    if (!turtle_agent)
        return false;

    const auto turtle_living = turtle_agent->GetAsAgentLiving();
    if (!turtle_living || turtle_living->GetIsDead() || turtle_living->hp == 0.00F)
        return false;

    const auto dist = GW::GetDistance(player_data->pos, turtle_agent->pos);
    if (dist > GW::Constants::Range::Spellcast)
        return false;

    if (CastBondIfNotAvailable(skillbar->prot, turtle_agent->agent_id, player_data))
        return true;

    if (CastBondIfNotAvailable(skillbar->life, turtle_agent->agent_id, player_data))
        return true;

    if (player_data->hp_perc < 0.50F || turtle_living->hp > 0.975F)
        return false;

    if (turtle_living->hp < 0.975F)
        return (RoutineState::FINISHED == skillbar->sb.Cast(player_data->energy, turtle_agent->agent_id));

    return (turtle_living->hp < 0.90F &&
            RoutineState::FINISHED == skillbar->fuse.Cast(player_data->energy, turtle_agent->agent_id));
}

bool EmoRoutine::RoutineWisdom() const
{
    return (RoutineState::FINISHED == skillbar->wisdom.Cast(player_data->energy));
}

bool EmoRoutine::RoutineGDW() const
{
    static auto last_idx = uint32_t{0};

    if (last_idx >= GW::PartyMgr::GetPartySize())
        last_idx = 0;
    const auto id = party_members[last_idx].id;
    last_idx++;

    if (!player_data->CanCast() || !id || !party_data_valid || id == player_data->id)
        return false;

    const auto agent = GW::Agents::GetAgentByID(id);
    if (!agent)
        return false;

    const auto living = agent->GetAsAgentLiving();
    if (!living || living->GetIsMoving() || living->GetIsDead() || living->hp == 0.00F)
        return false;

    const auto dist = GW::GetDistance(GW::GamePos{-16410.75F, 17294.47F, 0}, agent->pos);
    if (dist > GW::Constants::Range::Area)
        return false;

    if (living->GetIsWeaponSpelled())
        return false;

    return (RoutineState::FINISHED == skillbar->gdw.Cast(player_data->energy, living->agent_id));
}

bool EmoRoutine::RoutineTurtleGDW() const
{
    static auto last_idx = uint32_t{0};

    if (!player_data->CanCast())
        return false;

    const auto agent = GW::Agents::GetAgentByID(turtle_id);
    if (!agent)
        return false;

    const auto living = agent->GetAsAgentLiving();
    if (!living || living->GetIsMoving() || living->GetIsDead() || living->hp == 0.00F)
        return false;

    const auto dist = GW::GetDistance(GW::GamePos{-16105.50F, 17284.84F, 0}, agent->pos);
    if (dist > GW::Constants::Range::Spellcast)
        return false;

    if (living->GetIsWeaponSpelled())
        return false;

    return (RoutineState::FINISHED == skillbar->gdw.Cast(player_data->energy, living->agent_id));
}

bool EmoRoutine::RoutineDbBeforeDhuum() const
{
    static auto last_time_sb_ms = clock();

    if (!db_agent)
        return false;

    const auto living = db_agent->GetAsAgentLiving();
    if (!living)
        return false;

    const auto dist = GW::GetDistance(player_data->pos, db_agent->pos);
    if (GW::PartyMgr::GetPartySize() <= 6 && dist > 2100.0F)
        return false;
    else if (GW::PartyMgr::GetPartySize() > 6 && dist > GW::Constants::Range::Spellcast)
        return false;

    if (GW::PartyMgr::GetPartySize() > 6 && CastBondIfNotAvailable(skillbar->balth, living->agent_id, player_data))
        return true;

    if (living->hp > 0.75F || living->GetIsDead() || living->hp == 0.00F)
        return false;

    if (CastBondIfNotAvailable(skillbar->prot, living->agent_id, player_data))
        return true;

    if (living->hp < 0.50F && player_data->hp_perc > 0.50F)
        return (RoutineState::FINISHED == skillbar->fuse.Cast(player_data->energy, living->agent_id));

    const auto sb_recast_threshold_ms = 6'000L;
    if (TIMER_DIFF(last_time_sb_ms) > sb_recast_threshold_ms && living->hp < 0.50F &&
        RoutineState::FINISHED == skillbar->sb.Cast(player_data->energy, living->agent_id))
    {
        last_time_sb_ms = clock();
        return true;
    }

    return false;
}

bool EmoRoutine::RoutineKeepPlayerAlive() const
{
    if (player_data->living->GetIsMoving())
        return false;

    if (player_data->energy < 50U)
        return false;

    for (const auto &[id, _] : party_members)
    {
        if (!id || id == player_data->id)
            continue;

        const auto agent = GW::Agents::GetAgentByID(id);
        if (!agent)
            continue;

        const auto living = agent->GetAsAgentLiving();
        if (!living || living->GetIsDead() || living->hp == 0.00F)
            continue;

        const auto dist = GW::GetDistance(player_data->pos, agent->pos);
        if (dist > 600.0F)
            continue;

        if (living->hp > 0.50F)
            continue;

        if (player_data->hp_perc > 0.75F)
            return (RoutineState::FINISHED == skillbar->fuse.Cast(player_data->energy, living->agent_id));

        if (living->primary != static_cast<uint8_t>(GW::Constants::Profession::Ranger) && living->hp < 0.30F)
            if (CastBondIfNotAvailable(skillbar->prot, living->agent_id, player_data))
                return true;
    }

    return false;
}

bool EmoRoutine::DropBondsLT() const
{
    if (!lt_agent || !lt_agent->agent_id)
        return false;

    auto buffs = GW::Effects::GetPlayerBuffs();
    if (!buffs || !buffs->valid() || buffs->size() == 0)
        return false;

    const auto lt_living = lt_agent->GetAsAgentLiving();
    if (!lt_living)
        return false;

    if (DropBondsOnLiving(lt_living))
        return true;

    return false;
}

bool EmoRoutine::DropAllBonds() const
{
    auto dropped_smth = false;

    auto buffs = GW::Effects::GetPlayerBuffs();
    if (!buffs || !buffs->valid() || buffs->size() == 0)
        return false;

    for (const auto &buff : *buffs)
    {
        const auto skill = static_cast<GW::Constants::SkillID>(buff.skill_id);
        const auto is_prot_bond = skill == GW::Constants::SkillID::Protective_Bond;
        const auto is_life_bond = skill == GW::Constants::SkillID::Life_Bond;
        const auto is_balth_bond = skill == GW::Constants::SkillID::Balthazars_Spirit;
        const auto is_any_bond = is_prot_bond || is_life_bond || is_balth_bond;

        if (is_any_bond)
        {
            GW::Effects::DropBuff(buff.buff_id);
            dropped_smth = true;
        }
    }

    return dropped_smth;
}

RoutineState EmoRoutine::Routine()
{
    static auto last_warning_ms = clock();

    const auto is_in_dhuum_room = IsInDhuumRoom(player_data->pos);
    const auto is_in_dhuum_fight = IsInDhuumFight(player_data->pos);
    const auto dhuum_fight_done = DhuumFightDone(num_finished_objectives);

    if (is_in_dhuum_fight)
        move_ongoing = false;

    if (IsUw() && dhuum_fight_done && player_data->hp_perc > 0.99F)
    {
        (void)DropAllBonds();
        action_state = ActionState::INACTIVE;
        move_ongoing = false;
        return RoutineState::FINISHED;
    }

    if (player_data->energy < 20U && TIMER_DIFF(last_warning_ms) > 2000)
    {
        Log::Warning("Low Energy!, Num bonds: %u", player_data->GetNumberOfPartyBonds());
        last_warning_ms = clock();

        if (player_data->energy < 5U)
            (void)DropAllBonds();
    }

    if (!player_data->CanCast())
        return RoutineState::ACTIVE;

    if ((move_ongoing && !ActionABC::HasWaitedLongEnough(500L)) ||
        (!move_ongoing && !ActionABC::HasWaitedLongEnough(250L)))
        return RoutineState::ACTIVE;

    if (IsUw() && IsOnSpawnPlateau(player_data->pos, 300.0F) && BondLtAtStartRoutine())
        return RoutineState::ACTIVE;

    if (IsUw() && IsOnSpawnPlateau(player_data->pos, 300.0F) && !TankIsSoloLT())
    {
        action_state = ActionState::INACTIVE;
        return RoutineState::FINISHED;
    }

    if (RoutineSelfBonds())
        return RoutineState::FINISHED;

    if (!IsUw())
        return RoutineState::FINISHED;

    if (!is_in_dhuum_room && RoutineWhenInRangeBondLT())
        return RoutineState::FINISHED;

    if (RoutineKeepPlayerAlive())
        return RoutineState::FINISHED;

    if (!is_in_dhuum_room && RoutineDbBeforeDhuum())
        return RoutineState::FINISHED;

    if (!TankIsSoloLT() && !is_in_dhuum_room)
        return RoutineState::FINISHED;

    if ((IsInBasement(player_data->pos) || IsInVale(player_data->pos)) && RoutineEscortSpirits())
        return RoutineState::FINISHED;

    if (IsAtFusePulls(player_data->pos) && RoutineLtAtFusePulls())
        return RoutineState::FINISHED;

    // Make sure to only pop canthas if there is enough time until dhuum fight
    const auto stone_should_be_used =
        !used_canthas && ((GW::PartyMgr::GetPartySize() <= 4) ||
                          (GW::PartyMgr::GetPartySize() == 5 && GW::Map::GetInstanceTime() < EIGHT_MINS_IN_MS));

    const auto item_context = GW::GetItemContext();
    const auto world_context = GW::GetWorldContext();

    if (item_context && IsAtValeSpirits(player_data->pos) && stone_should_be_used &&
        UseInventoryItem(CANTHA_STONE_ID, 1, item_context->bags_array.size()))
    {
        used_canthas = true;
        return RoutineState::FINISHED;
    }

    if (IsInVale(player_data->pos) && RoutineCanthaGuards())
        return RoutineState::FINISHED;

    if (IsInVale(player_data->pos) && RoutineDbAtSpirits())
        return RoutineState::FINISHED;

    if (IsGoingToDhuum(player_data->pos) && DropBondsLT())
        return RoutineState::FINISHED;

    if (!is_in_dhuum_room)
        return RoutineState::FINISHED;

    if (RoutineTurtle())
        return RoutineState::FINISHED;

    if (RoutineDbAtDhuum())
        return RoutineState::FINISHED;

    if (!is_in_dhuum_fight)
        return RoutineState::FINISHED;

    if (world_context && item_context)
    {
        static auto last_time_cookie = clock();
        if (TIMER_DIFF(last_time_cookie) > 500 && world_context->morale <= 90 &&
            UseInventoryItem(COOKIE_ID, 1, item_context->bags_array.size()))
        {
            last_time_cookie = clock();
            return RoutineState::FINISHED;
        }
    }

    if (RoutineWisdom())
        return RoutineState::FINISHED;

    if (RoutineKeepPlayerAlive())
        return RoutineState::FINISHED;

    if (!skillbar->pi.SkillFound() && !skillbar->gdw.SkillFound())
        return RoutineState::FINISHED;

    if (!is_in_dhuum_room)
        return RoutineState::FINISHED;

    auto dhuum_hp = float{0.0F};
    auto dhuum_max_hp = uint32_t{0U};
    const auto dhuum_agent = GetDhuumAgent();
    GetDhuumAgentData(dhuum_agent, dhuum_hp, dhuum_max_hp);
    if (dhuum_hp < 0.25F)
        return RoutineState::FINISHED;

    if (dhuum_agent && DhuumIsCastingJudgement(dhuum_agent) &&
        (RoutineState::FINISHED == skillbar->pi.Cast(player_data->energy, dhuum_agent->agent_id)))
        return RoutineState::FINISHED;

    if (RoutineGDW())
        return RoutineState::FINISHED;

    if (RoutineTurtleGDW())
        return RoutineState::FINISHED;

    return RoutineState::FINISHED;
}

bool EmoRoutine::PauseRoutine() noexcept
{
    if (player_data->living->GetIsMoving())
        return true;

    if (player_data->target && TargetIsReaper(*player_data))
    {
        const auto dist_reaper = GW::GetDistance(player_data->pos, player_data->target->pos);
        if (dist_reaper < GW::Constants::Range::Nearby && player_data->energy > 30U)
            return true;
    }

    return false;
}

void EmoRoutine::Update()
{
    static auto paused = false;

    if (GW::PartyMgr::GetIsPartyDefeated())
    {
        action_state = ActionState::INACTIVE;
    }

    if (IsUw())
    {
        const auto [lt_id, has_tank] = GetTankId();
        if (lt_id && has_tank)
        {
            lt_agent = GW::Agents::GetAgentByID(lt_id);
        }

        const auto db_id = GetDhuumBitchId();
        if (db_id)
        {
            db_agent = GW::Agents::GetAgentByID(db_id);
        }
    }

    party_members.clear();
    party_data_valid = GetPartyMembers(party_members);

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

    if (action_state == ActionState::INACTIVE && GW::PartyMgr::GetPartySize() <= 6 && player_data->living &&
        !player_data->living->GetIsMoving() &&
        ((player_data->energy_perc < 0.10F || player_data->hp_perc < 0.10F) && livings_data->enemies.size() > 0U))
        action_state = ActionState::ACTIVE;

    static auto not_moving_timer = clock();
    if (player_data->living->GetIsMoving())
        not_moving_timer = clock();

    if (!TankIsSoloLT() && TIMER_DIFF(not_moving_timer) < 1000 && action_state == ActionState::ACTIVE)
        action_state = ActionState::ON_HOLD;

    if (action_state == ActionState::ACTIVE)
        (void)Routine();
}

bool EmoRoutine::BondLtAtStartRoutine() const
{
    auto [tank_id, has_tank] = GetTankId();
    if (!tank_id || !has_tank)
    {
        return false;
    }

    const auto tank = GW::Agents::GetAgentByID(tank_id);
    if (!tank)
    {
        return false;
    }

    const auto is_alive_ally = IsAliveAlly(tank);
    if (!is_alive_ally)
    {
        return false;
    }

    if (CastBondIfNotAvailable(skillbar->balth, tank_id, player_data))
    {
        return true;
    }

    if (CastBondIfNotAvailable(skillbar->prot, tank_id, player_data))
    {
        return true;
    }

    if (CastBondIfNotAvailable(skillbar->life, tank_id, player_data))
    {
        return true;
    }

    return false;
}

#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <tuple>
#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Context/GameContext.h>
#include <GWCA/Context/WorldContext.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Hero.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Utilities/Hooker.h>
#include <GWCA/Utilities/Scanner.h>

#include "ActionTypes.h"
#include "ActionsBase.h"
#include "DataPlayer.h"
#include "Defines.h"
#include "Helper.h"
#include "HelperAgents.h"
#include "HelperHero.h"
#include "HelperMaps.h"
#include "HelperSkill.h"
#include "HeroWindow.h"
#include "Logger.h"
#include "Utils.h"
#include "UtilsGui.h"
#include "UtilsMath.h"

#include <imgui.h>

namespace
{
constexpr auto IM_COLOR_RED = ImVec4(1.0F, 0.1F, 0.1F, 1.0F);
constexpr auto IM_COLOR_GREEN = ImVec4(0.1F, 0.9F, 0.1F, 1.0F);
constexpr auto IM_COLOR_BLUE = ImVec4(0.1F, 0.1F, 1.0F, 1.0F);

void PingLogic(const uint32_t agent_id)
{
    auto *instance = static_cast<HeroWindow *>(ToolboxPluginInstance());
    if (!instance)
        return;

    const auto *ping_agent = GW::Agents::GetAgentByID(agent_id);
    if (!ping_agent)
        return;

    const auto ping_distance = GW::GetDistance(ping_agent->pos, instance->player_data.pos);
    const auto ping_close = ping_distance < GW::Constants::Range::Spellcast;
    const auto ping_far = ping_distance > GW::Constants::Range::Spellcast;
    if (ping_close)
    {
        instance->StopFollowing();
        instance->ping_target_id = 0U;
        instance->move_time_ms = 0U;
    }
    else if (ping_far)
    {
        instance->StartFollowing();
        instance->ping_target_id = ping_agent->agent_id;
        instance->move_time_ms = 0U;
    }
}

void OnSkillOnEnemy(const uint32_t value_id, const uint32_t caster_id)
{
    auto *instance = static_cast<HeroWindow *>(ToolboxPluginInstance());
    if (!instance)
        return;

    const auto player_id = GW::Agents::GetPlayerId();
    if (caster_id != player_id)
        return;

    switch (value_id)
    {
    case 45:
    {
        break;
    }
    default:
    {
        return;
    }
    }

    const auto target_agent = GetTargetAsLiving();
    if (!target_agent)
        return;

    if (!target_agent || target_agent->allegiance != GW::Constants::Allegiance::Enemy)
        return;

    const auto dist = GW::GetDistance(target_agent->pos, instance->player_data.pos);
    if (dist < GW::Constants::Range::Spellcast)
        return;

    PingLogic(target_agent->agent_id);
}

void OnEnemyInteract(GW::HookStatus *, GW::UI::UIMessage, void *wparam, void *)
{
    const auto packet = static_cast<GW::UI::UIPacket::kInteractAgent *>(wparam);
    if (!packet || !packet->call_target)
        return;

    PingLogic(packet->agent_id);
}

void OnMapLoad(GW::HookStatus *, const GW::Packet::StoC::MapLoaded *)
{
    auto *instance = static_cast<HeroWindow *>(ToolboxPluginInstance());

    instance->ResetData();
}
} // namespace

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static HeroWindow instance;
    return &instance;
}

void HeroWindow::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);
    if (!GW::Initialize())
    {
        GW::Terminate();
        return;
    }

    // GW::UI::RegisterUIMessageCallback(&AgentCalled_Entry, GW::UI::UIMessage::kSendInteractEnemy, OnEnemyInteract);

    // GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericValue>(
    //     &GenericValueTarget_Entry,
    //     [this](const GW::HookStatus *, const GW::Packet::StoC::GenericValue *packet) -> void {
    //         const uint32_t value_id = packet->value_id;
    //         const uint32_t caster_id = packet->agent_id;
    //         OnSkillOnEnemy(value_id, caster_id);
    //     });

    // GW::StoC::RegisterPacketCallback<GW::Packet::StoC::MapLoaded>(&MapLoaded_Entry, OnMapLoad);

    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"HeroWindow");
}

void HeroWindow::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

bool HeroWindow::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void HeroWindow::Terminate()
{
    ToolboxPlugin::Terminate();
    GW::StoC::RemoveCallbacks(&MapLoaded_Entry);
    GW::StoC::RemoveCallbacks(&GenericValueTarget_Entry);
    GW::UI::RemoveUIMessageCallback(&AgentCalled_Entry);
}

void HeroWindow::LoadSettings(const wchar_t *folder)
{
    ToolboxUIPlugin::LoadSettings(folder);
    show_debug_map = ini.GetBoolValue(Name(), VAR_NAME(show_debug_map), show_debug_map);
}

void HeroWindow::SaveSettings(const wchar_t *folder)
{
    ToolboxUIPlugin::SaveSettings(folder);
    ini.SetBoolValue(Name(), VAR_NAME(show_debug_map), show_debug_map);
    PLUGIN_ASSERT(ini.SaveFile(GetSettingFile(folder).c_str()) == SI_OK);
}

void HeroWindow::Draw(IDirect3DDevice9 *)
{
    //     if (!player_data.ValidateData(HelperActivationConditions, true) || (*GetVisiblePtr()) == false)
    //         return;

    //     const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    //     if (!party_info || party_info->heroes.size() == 0)
    //         return;

    //     ImGui::SetNextWindowSize(ImVec2(240.0F, 45.0F), ImGuiCond_FirstUseEver);
    //     if (ImGui::Begin(Name(), show_closebutton ? GetVisiblePtr() : nullptr, GetWinFlags(ImGuiWindowFlags_NoResize)))
    //     {
    //         const auto width = ImGui::GetWindowWidth();
    //         const auto im_button_size = ImVec2{width / 3.0F - 10.0F, 50.0F};

    //         toggled_follow = false;

    //         HeroBehaviour_DrawAndLogic(im_button_size);
    //         HeroFollow_DrawAndLogic(im_button_size);
    //         HeroSpike_DrawAndLogic(im_button_size);
    //     }
    //     ImGui::End();

    // #ifdef _DEBUG
    //     if (show_debug_map)
    //     {
    //         // const auto enemies_in_aggro_of_player = AgentLivingData::AgentsInRange(player_data.pos,
    //         //                                                                        GW::Constants::Allegiance::Enemy,
    //         //                                                                        GW::Constants::Range::Compass);

    //         // const auto enemy_center_pos = AgentLivingData::ComputeCenterOfMass(enemies_in_aggro_of_player);
    //         // const auto player_pos = player_data.pos;
    //         // const auto [center_player_m, center_player_b] = ComputeLine(enemy_center_pos, player_pos);
    //         // const auto [dividing_m, dividing_b] =
    //         //     ComputePerpendicularLineAtPos(center_player_m, center_player_b, player_pos);
    //         // const auto a = ComputePositionOnLine(player_pos, center_player_m, center_player_b, 500.0F);

    //         // DrawFlaggingFeature(player_data.pos, enemies_in_aggro_of_player, "Flagging");
    //     }
    // #endif
}

void HeroWindow::Update(float)
{
    if (!player_data.ValidateData(HelperActivationConditions, true))
    {
        ResetData();
        return;
    }

    player_data.Update();
    hero_data.Update();
    // UpdateInternalData();

    // HeroSmarterFollow_Main();
    // HeroSmarterFlagging_Main();
    HeroSmarterSkills_Main();
}

/* MAIN CYCLE FUNCTIONS */


void HeroWindow::UpdateInternalData()
{
    if (player_data.pos == follow_pos && following_active)
    {
        ms_with_no_pos_change = TIMER_DIFF(time_at_last_pos_change);
    }
    else
    {
        ms_with_no_pos_change = 0U;
        time_at_last_pos_change = TIMER_INIT();
    }
    follow_pos = player_data.pos;

    const auto target = GW::Agents::GetTarget();
    if (target && target->agent_id)
        target_agent_id = target->agent_id;
    else
        target_agent_id = 0U;
}

void HeroWindow::ResetData()
{
    target_agent_id = 0U;
    ping_target_id = 0U;
    follow_pos = GW::GamePos{};
    following_active = false;
    hero_data.hero_vec.clear();
}

void HeroWindow::HeroSmarterFollow_Main()
{
    HeroFollow_StartWhileRunning();
    HeroFollow_StopConditions();
    HeroFollow_StuckCheck();
    HeroFollow_Flagging();
}

void HeroWindow::HeroSmarterFlagging_Main()
{
}

bool HeroWindow::HeroSmarterSkills_Main()
{
    if (!IsMapReady() || !IsExplorable() || following_active || (hero_data.hero_vec.size() == 0))
        return false;

    if (UseBipOnPlayer())
        return true;
    if (UseSplinterOnPlayer())
        return true;
    // if (UseShelterInFight())
    //     return true;
    // if (UseUnionInFight())
    //     return true;
    // if (UseSosInFight())
    //     return true;
    // if (UseVigSpiritOnPlayer())
    //     return true;
    // if (RemoveImportantConditions())
    //     return true;
    // if (ShatterImportantHexes())
    //     return true;
    // if (UseHonorOnPlayer())
    //     return true;
    // if (RuptEnemies())
    //     return true;

    return false;
}

/* INTERNAL FUNCTIONS FOLLOW */

void HeroWindow::FollowPlayer()
{
    if (!IsMapReady() || !IsExplorable() || !player_data.living || (follow_pos.x == 0.0F && follow_pos.y == 0.0F) ||
        !GW::PartyMgr::GetIsPartyLoaded() || GW::PartyMgr::GetIsPartyDefeated())
        return;

    GW::PartyMgr::FlagAll(follow_pos);
}

void HeroWindow::StartFollowing()
{
    if (!following_active)
    {
        Log::Info("Heroes will follow the player!");
        current_hero_behaviour_before_follow = current_hero_behaviour;
    }
    following_active = true;
    current_hero_behaviour = GW::HeroBehavior::AvoidCombat;
}

void HeroWindow::StopFollowing()
{
    if (following_active)
        Log::Info("Heroes stopped following the player!");
    following_active = false;
    GW::PartyMgr::UnflagAll();
    if (current_hero_behaviour != current_hero_behaviour_before_follow)
    {
        current_hero_behaviour = current_hero_behaviour_before_follow;
        SetHerosBehaviour(player_data.living->login_number, current_hero_behaviour);
    }
}

void HeroWindow::HeroFollow_DrawAndLogic(const ImVec2 &im_button_size)
{
    auto added_color_follow = false;

    if (following_active)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COLOR_GREEN);
        added_color_follow = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("Follow###followPlayer", im_button_size))
    {
        if (IsExplorable())
        {
            following_active = !following_active;
            toggled_follow = true;

            if (following_active)
                StartFollowing();
            else
                StopFollowing();
        }
    }

    if (added_color_follow)
        ImGui::PopStyleColor();
}

void HeroWindow::HeroFollow_StopConditions()
{
    if (!IsExplorable() || !following_active)
    {
        ms_with_no_pos_change = 0U;
        time_at_last_pos_change = TIMER_INIT();
        return;
    }

    if (ms_with_no_pos_change >= 10'000)
    {
        StopFollowing();
        ms_with_no_pos_change = 0U;
        time_at_last_pos_change = TIMER_INIT();
    }

    if (player_data.IsAttacking() && following_active)
    {
        StopFollowing();
        ms_with_no_pos_change = 0U;
        time_at_last_pos_change = TIMER_INIT();
    }

    if (following_active && ping_target_id)
    {
        const auto *target_agent = GW::Agents::GetAgentByID(ping_target_id);
        if (target_agent)
        {
            const auto dist = GW::GetDistance(target_agent->pos, player_data.pos);
            if (dist < GW::Constants::Range::Spellcast)
            {
                StopFollowing();
                ping_target_id = 0;
            }
        }
    }
    else
    {
        ping_target_id = 0;
    }
}

void HeroWindow::HeroFollow_Flagging()
{
    static auto last_follow_trigger_ms = clock();
    static auto gen = std::mt19937{};
    static auto time_dist = std::uniform_int_distribution<long>(-10, 10);

    if (IsExplorable() && following_active && TIMER_DIFF(last_follow_trigger_ms) > 800 + time_dist(gen))
    {
        FollowPlayer();
        last_follow_trigger_ms = clock();

        UseFallback();
    }
    else if (IsMapReady() && IsExplorable() && toggled_follow)
    {
        GW::PartyMgr::UnflagAll();
    }
}

void HeroWindow::HeroFollow_StartWhileRunning()
{
    if (!IsExplorable())
        return;

    if (!player_data.IsMoving())
        move_time_ms = clock();

    const auto start_follow_bc_moving = TIMER_DIFF(move_time_ms) > 5'000;

    const auto target_agent = GetTargetAsLiving();
    if (!target_agent)
    {
        if (start_follow_bc_moving)
            StartFollowing();
        return;
    }

    if (target_agent->allegiance == GW::Constants::Allegiance::Enemy)
    {
        const auto dist = GW::GetDistance(target_agent->pos, player_data.pos);
        if (dist < GW::Constants::Range::Spellcast)
        {
            StopFollowing();
            return;
        }
    }

    if (start_follow_bc_moving)
        StartFollowing();
}

void HeroWindow::HeroFollow_StuckCheck()
{
    constexpr static auto max_num_heros = 7U;
    static auto same_position_counters = std::array<uint32_t, max_num_heros>{};
    static auto last_positions = std::array<GW::GamePos, max_num_heros>{};

    auto hero_idx = 0U;
    for (const auto &hero : hero_data.hero_vec)
    {
        if (!hero.hero_living || !hero.hero_living->agent_id || !player_data.living ||
            !player_data.living->GetIsMoving())
        {
            same_position_counters.at(hero_idx) = 0U;
            ++hero_idx;
            continue;
        }

        const auto *hero_agent = GW::Agents::GetAgentByID(hero.hero_living->agent_id);
        if (!hero_agent || GW::GetDistance(player_data.pos, hero_agent->pos) > (GW::Constants::Range::Compass - 500.0F))
        {
            same_position_counters.at(hero_idx) = 0U;
            ++hero_idx;
            continue;
        }

        if (hero_agent->pos == last_positions.at(hero_idx))
            ++same_position_counters.at(hero_idx);
        else
            same_position_counters.at(hero_idx) = 0U;

        if (same_position_counters.at(hero_idx) >= 1'000U)
        {
            Log::Info("Hero at position %d might be stuck!", hero_idx + 1U);
            same_position_counters.at(hero_idx) = 0U;
        }

        last_positions.at(hero_idx) = hero_agent->pos;

        ++hero_idx;
    }
}

void HeroWindow::HeroFollow_AttackTarget()
{
    const auto target = GW::Agents::GetTarget();
    if (!target)
        return;

    const auto target_distance = GW::GetDistance(player_data.pos, target->pos);

    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (!party_info || !party_info->heroes.valid())
        return;

    for (const auto &hero : party_info->heroes)
    {
        const auto *hero_living = GW::Agents::GetAgentByID(hero.agent_id);
        if (!hero_living)
            continue;

        if (GW::GetDistance(player_data.pos, hero_living->pos) > 800.0F || target_distance > 800.0F)
        {
            UseFallback();
            break;
        }
    }
}

/* INTERNAL FUNCTIONS BEHAVIOUR */

void HeroWindow::HeroBehaviour_DrawAndLogic(const ImVec2 &im_button_size)
{
    switch (current_hero_behaviour)
    {
    case GW::HeroBehavior::Guard:
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COLOR_BLUE);
        break;
    }
    case GW::HeroBehavior::AvoidCombat:
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COLOR_GREEN);
        break;
    }
    case GW::HeroBehavior::Fight:
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COLOR_RED);
        break;
    }
    }

    if (ImGui::Button("Behaviour###toggleState", im_button_size))
        ToggleHeroBehaviour();

    ImGui::PopStyleColor();
}

void HeroWindow::ToggleHeroBehaviour()
{
    if (!player_data.living || !IsMapReady() || following_active)
        return;

    Log::Info("Toggle hero hehaviour!");

    if (current_hero_behaviour == GW::HeroBehavior::Guard)
        current_hero_behaviour = GW::HeroBehavior::AvoidCombat;
    else if (current_hero_behaviour == GW::HeroBehavior::Fight)
        current_hero_behaviour = GW::HeroBehavior::Guard;
    else if (current_hero_behaviour == GW::HeroBehavior::AvoidCombat)
        current_hero_behaviour = GW::HeroBehavior::Fight;
    else
        return;

    SetHerosBehaviour(player_data.living->login_number, current_hero_behaviour);
}

/* INTERNAL FUNCTIONS ATTACK */


void HeroWindow::HeroSpike_DrawAndLogic(const ImVec2 &im_button_size)
{
    ImGui::SameLine();

    if (ImGui::Button("Attack###attackTarget", im_button_size))
    {
        StopFollowing();

        if (!IsExplorable())
            return;

        HeroFollow_AttackTarget();

        AttackTarget();
    }
}

bool HeroWindow::MesmerSpikeTarget(const Hero &hero) const
{
    constexpr static auto skill_id = GW::Constants::SkillID::Energy_Surge;
    constexpr static auto skill_class = GW::Constants::Profession::Mesmer;

    if (!hero.hero_living)
        return false;

    const auto target = GW::Agents::GetTarget();
    if (!target)
        return false;

    const auto *target_living = target->GetAsAgentLiving();
    if (!target_living)
        return false;

    if (target_living->allegiance != GW::Constants::Allegiance::Enemy)
        return false;

    auto hero_conditions = [](const DataPlayer &, const Hero &) { return true; };

    if (hero.hero_living->primary == static_cast<uint8_t>(skill_class))
        return HeroCastSkillIfAvailable(hero, player_data, skill_id, hero_conditions, TargetLogic::PLAYER_TARGET);

    return false;
}

void HeroWindow::AttackTarget()
{
    const auto target = GW::Agents::GetTarget();
    if (!target)
        return;

    const auto *target_living = target->GetAsAgentLiving();
    if (!target_living || target_living->allegiance != GW::Constants::Allegiance::Enemy)
        return;

    if (!HeroSkill_StartConditions(GW::Constants::SkillID::No_Skill))
        return;

    Log::Info("Mesmer Heroes will attack the players target!");

    for (const auto &hero : hero_data.hero_vec)
    {
        MesmerSpikeTarget(hero);
    }
}

/* INTERNAL SMART SKILLS */

bool HeroWindow::ShatterImportantHexes()
{
    constexpr static auto to_remove_hexes_melee = std::array{
        // Mesmer
        GW::Constants::SkillID::Ineptitude,
        GW::Constants::SkillID::Empathy,
        GW::Constants::SkillID::Crippling_Anguish,
        GW::Constants::SkillID::Clumsiness,
        GW::Constants::SkillID::Faintheartedness,
        // Necro
        GW::Constants::SkillID::Spiteful_Spirit,
        // Ele
        GW::Constants::SkillID::Blurred_Vision,
    };
    constexpr static auto to_remove_hexes_caster = std::array{
        // Mesmer
        GW::Constants::SkillID::Panic,
        GW::Constants::SkillID::Backfire,
        GW::Constants::SkillID::Mistrust,
        GW::Constants::SkillID::Power_Leech,
        // Necro
        GW::Constants::SkillID::Spiteful_Spirit,
        GW::Constants::SkillID::Soul_Leech,
    };
    constexpr static auto to_remove_hexes_all = std::array{
        // Mesmer
        GW::Constants::SkillID::Diversion,
        GW::Constants::SkillID::Visions_of_Regret,
        // Ele
        GW::Constants::SkillID::Deep_Freeze,
        GW::Constants::SkillID::Mind_Freeze,
    };
    constexpr static auto to_remove_hexes_paragon = std::array{
        // Necro
        GW::Constants::SkillID::Vocal_Minority,
    };

    const static auto skill_class_pairs = std::vector<std::tuple<GW::Constants::SkillID, GW::Constants::Profession>>{
        {GW::Constants::SkillID::Shatter_Hex, GW::Constants::Profession::Mesmer},
        {GW::Constants::SkillID::Remove_Hex, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Smite_Hex, GW::Constants::Profession::Monk},
    };
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data) {
        if (!player_data.living || !player_data.living->GetIsHexed())
            return false;

        const auto *const effects = GW::Effects::GetPlayerEffects();
        if (!effects || !effects->valid())
            return false;

        auto found_hex = [](const auto &hex_array, const GW::Effect &curr_hex) {
            return std::find(hex_array.begin(), hex_array.end(), curr_hex.skill_id) != hex_array.end();
        };

        for (const auto &effect : *effects)
        {
            if (effect.agent_id != 0)
                continue;

            if (DataPlayer::HoldsMeleeWeapon() && found_hex(to_remove_hexes_melee, effect))
                return true;

            if (!DataPlayer::HoldsMeleeWeapon() && found_hex(to_remove_hexes_caster, effect))
                return true;

            if (found_hex(to_remove_hexes_all, effect))
                return true;

            if (player_data.primary == GW::Constants::Profession::Paragon && found_hex(to_remove_hexes_paragon, effect))
                return true;
        }

        return false;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!player_data.living || !hero.hero_living)
            return false;

        const auto distance = GW::GetDistance(player_data.living->pos, hero.hero_living->pos);
        return distance <= GW::Constants::Range::Spellcast;
    };

    for (const auto &[skill_id, skill_class] : skill_class_pairs)
    {
        if (SmartUseSkill(skill_id,
                          skill_class,
                          "Remove Hex",
                          player_data,
                          hero_data,
                          player_conditions,
                          hero_conditions,
                          wait_ms,
                          target_logic,
                          false))
            return true;
    }

    return false;
}

bool HeroWindow::RemoveImportantConditions()
{
    constexpr static auto to_remove_conditions_melee = std::array{
        GW::Constants::SkillID::Blind,
        GW::Constants::SkillID::Weakness,
    };
    constexpr static auto to_remove_conditions_caster = std::array{
        GW::Constants::SkillID::Dazed,
    };
    constexpr static auto to_remove_conditions_all = std::array{
        GW::Constants::SkillID::Crippled,
    };

    const static auto skill_class_pairs = std::vector<std::tuple<GW::Constants::SkillID, GW::Constants::Profession>>{
        {GW::Constants::SkillID::Mend_Body_and_Soul, GW::Constants::Profession::Ritualist},
        {GW::Constants::SkillID::Dismiss_Condition, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Mend_Condition, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Smite_Condition, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Purge_Conditions, GW::Constants::Profession::Monk},
    };
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data) {
        if (!player_data.living || !player_data.living->GetIsConditioned())
            return false;

        const auto *const effects = GW::Effects::GetPlayerEffects();
        if (!effects || !effects->valid())
            return false;

        auto found_cond = [](const auto &cond_array, const GW::Effect &curr_cond) {
            return std::find(cond_array.begin(), cond_array.end(), curr_cond.skill_id) != cond_array.end();
        };

        for (const auto &effect : *effects)
        {
            if (effect.agent_id != 0)
                continue;

            if (DataPlayer::HoldsMeleeWeapon())
            {
                if (found_cond(to_remove_conditions_melee, effect))
                    return true;
            }
            else
            {
                if (found_cond(to_remove_conditions_caster, effect))
                    return true;
            }

            if (found_cond(to_remove_conditions_all, effect))
                return true;
        }

        return false;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!player_data.living || !hero.hero_living)
            return false;

        const auto distance = GW::GetDistance(player_data.living->pos, hero.hero_living->pos);
        return distance <= GW::Constants::Range::Spellcast;
    };

    for (const auto &[skill_id, skill_class] : skill_class_pairs)
    {
        if (SmartUseSkill(skill_id,
                          skill_class,
                          "Remove Cond",
                          player_data,
                          hero_data,
                          player_conditions,
                          hero_conditions,
                          wait_ms,
                          target_logic,
                          false))
            return true;
    }

    return false;
}

bool HeroWindow::RuptEnemies()
{
    const static auto skill_class_pairs = std::vector<std::tuple<GW::Constants::SkillID, GW::Constants::Profession>>{
        {GW::Constants::SkillID::Cry_of_Frustration, GW::Constants::Profession::Mesmer},
        {GW::Constants::SkillID::Power_Drain, GW::Constants::Profession::Mesmer},
    };
    const static auto skills_to_rupt = std::array{
        // Mesmer
        GW::Constants::SkillID::Panic,
        GW::Constants::SkillID::Energy_Surge,
        // Necro
        GW::Constants::SkillID::Chilblains,
        // Ele
        GW::Constants::SkillID::Meteor,
        GW::Constants::SkillID::Meteor_Shower,
        GW::Constants::SkillID::Searing_Flames,
        // All
        GW::Constants::SkillID::Resurrection_Signet,

    };
    constexpr static auto wait_ms = 200UL;
    constexpr static auto target_logic = TargetLogic::SEARCH_TARGET;
    static auto last_time_target_changed = clock();

    const auto target = GW::Agents::GetTarget();
    auto player_target = target ? target->agent_id : 0;
    auto change_target_to_id = 0U;

    auto player_conditions = [&change_target_to_id](const DataPlayer &player_data) {
        static auto _last_time_target_changed = clock();

        const auto agents_ptr = GW::Agents::GetAgentArray();
        if (!agents_ptr)
            return false;
        auto &agents = *agents_ptr;

        for (const auto *enemy : agents)
        {
            if (!enemy)
                continue;

            const auto dist_to_enemy = GW::GetDistance(player_data.pos, enemy->pos);
            if (dist_to_enemy > GW::Constants::Range::Spellcast + 200.0F)
                continue;

            if (!enemy)
                continue;

            const auto enemy_living = enemy->GetAsAgentLiving();
            if (!enemy_living || enemy_living->allegiance != GW::Constants::Allegiance::Enemy)
                continue;

            const auto skill_id = static_cast<GW::Constants::SkillID>(enemy_living->skill);
            if (skill_id == GW::Constants::SkillID::No_Skill)
                continue;

            if (std::find(skills_to_rupt.begin(), skills_to_rupt.end(), skill_id) != skills_to_rupt.end())
            {
                const auto new_target_id = enemy->agent_id;
                const auto target = GW::Agents::GetTarget();
                if (target && target->agent_id != new_target_id && TIMER_DIFF(_last_time_target_changed) > 10)
                {
                    change_target_to_id = new_target_id;
                    _last_time_target_changed = clock();
                }
                return true;
            }
        }

        return false;
    };

    const auto hero_conditions = [&change_target_to_id](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living || !player_data.living)
            return false;

        const auto dist_to_enemy = GW::GetDistance(player_data.living->pos, hero.hero_living->pos);
        if (dist_to_enemy > GW::Constants::Range::Spellcast + 200.0F)
            return false;

        if (change_target_to_id)
        {
            GW::GameThread::Enqueue([&, change_target_to_id] { GW::Agents::ChangeTarget(change_target_to_id); });
            change_target_to_id = 0;
        }

        return true;
    };

    for (const auto &[skill_id, skill_class] : skill_class_pairs)
    {
        if (SmartUseSkill(skill_id,
                          skill_class,
                          "Rupted Skill",
                          player_data,
                          hero_data,
                          player_conditions,
                          hero_conditions,
                          wait_ms,
                          target_logic))
        {
            if (TIMER_DIFF(last_time_target_changed) > 10)
            {
                GW::GameThread::Enqueue([&, player_target] { GW::Agents::ChangeTarget(player_target); });
                last_time_target_changed = clock();
            }

            return true;
        }
    }

    return false;
}

bool HeroWindow::UseSplinterOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Splinter_Weapon;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data) {
        const auto num_enemies_at_player = AgentLivingData::NumAgentsInRange(player_data.pos,
                                                                             GW::Constants::Allegiance::Enemy,
                                                                             GW::Constants::Range::Nearby);

        const auto player_is_melee_attacking = DataPlayer::HoldsMeleeWeapon();
        const auto player_is_melee_class = DataPlayer::IsMeleeClass;

        return num_enemies_at_player >= 2 && player_is_melee_attacking && player_is_melee_class;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast && hero.hero_living->energy > 0.25F;
    };

    return SmartUseSkill(skill_id,
                         skill_class,
                         "Splinter",
                         player_data,
                         hero_data,
                         player_conditions,
                         hero_conditions,
                         wait_ms,
                         target_logic,
                         false);
}

bool HeroWindow::UseVigSpiritOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Vigorous_Spirit;
    constexpr static auto skill_class = GW::Constants::Profession::Monk;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &) {
        const auto player_is_melee_attacking = DataPlayer::HoldsMeleeWeapon();
        const auto player_is_melee_class = DataPlayer::IsMeleeClass;

        return player_is_melee_attacking && player_is_melee_class;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast && hero.hero_living->energy > 0.25F;
    };

    return SmartUseSkill(skill_id,
                         skill_class,
                         "Splinter",
                         player_data,
                         hero_data,
                         player_conditions,
                         hero_conditions,
                         wait_ms,
                         target_logic,
                         false);
}

bool HeroWindow::UseHonorOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Strength_of_Honor;
    constexpr static auto skill_class = GW::Constants::Profession::Monk;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &) {
        const auto player_is_melee_attacking = DataPlayer::HoldsMeleeWeapon();
        const auto player_is_melee_class = DataPlayer::IsMeleeClass;

        return player_is_melee_attacking && player_is_melee_class;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast && hero.hero_living->energy > 0.25F;
    };

    return SmartUseSkill(skill_id,
                         skill_class,
                         "Honor",
                         player_data,
                         hero_data,
                         player_conditions,
                         hero_conditions,
                         wait_ms,
                         target_logic,
                         false);
}

bool HeroWindow::UseShelterInFight()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Shelter;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data) {
        const auto num_enemies_in_aggro_of_player =
            AgentLivingData::NumAgentsInRange(player_data.pos,
                                              GW::Constants::Allegiance::Enemy,
                                              GW::Constants::Range::Spellcast + 200.0F);

        const auto player_started_fight = num_enemies_in_aggro_of_player >= 3 && player_data.IsFighting();
        const auto has_skill_already = DataPlayer::PlayerHasEffect(GW::Constants::SkillID::Shelter);

        return !has_skill_already && player_started_fight;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spirit - 100.0F;
    };

    return SmartUseSkill(skill_id,
                         skill_class,
                         "Shelter",
                         player_data,
                         hero_data,
                         player_conditions,
                         hero_conditions,
                         wait_ms,
                         target_logic,
                         true);
}

bool HeroWindow::UseUnionInFight()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Union;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data) {
        const auto num_enemies_in_aggro_of_player =
            AgentLivingData::NumAgentsInRange(player_data.pos,
                                              GW::Constants::Allegiance::Enemy,
                                              GW::Constants::Range::Spellcast + 200.0F);

        const auto player_started_fight = num_enemies_in_aggro_of_player >= 3 && player_data.IsFighting();
        const auto has_skill_already = DataPlayer::PlayerHasEffect(GW::Constants::SkillID::Union);

        return !has_skill_already && player_started_fight;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spirit - 100.0F;
    };

    return SmartUseSkill(skill_id,
                         skill_class,
                         "Union",
                         player_data,
                         hero_data,
                         player_conditions,
                         hero_conditions,
                         wait_ms,
                         target_logic,
                         true);
}

bool HeroWindow::UseSosInFight()
{
    constexpr static auto SOS1_AGENT_ID = uint32_t{4229};
    constexpr static auto SOS2_AGENT_ID = uint32_t{4230};
    constexpr static auto SOS3_AGENT_ID = uint32_t{4231};

    constexpr static auto skill_id = GW::Constants::SkillID::Signet_of_Spirits;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data) {
        const auto agents_ptr = GW::Agents::GetAgentArray();
        if (!agents_ptr)
            return false;
        auto &agents = *agents_ptr;

        const auto num_enemies_in_aggro_of_player = AgentLivingData::NumAgentsInRange(player_data.pos,
                                                                                      GW::Constants::Allegiance::Enemy,
                                                                                      GW::Constants::Range::Spellcast);

        const auto player_started_fight = num_enemies_in_aggro_of_player >= 3 && player_data.IsFighting();

        if (!player_started_fight)
            return false;

        auto spirits_in_range = std::vector<GW::AgentLiving *>{};
        for (auto *enemy : agents)
        {
            if (!enemy)
                continue;

            const auto dist_to_enemy = GW::GetDistance(player_data.pos, enemy->pos);
            if (dist_to_enemy > GW::Constants::Range::Spellcast + 200.0F)
                continue;

            if (!enemy)
                continue;
            const auto enemy_living = enemy->GetAsAgentLiving();
            if (!enemy_living || enemy_living->allegiance != GW::Constants::Allegiance::Spirit_Pet)
                continue;

            spirits_in_range.push_back(enemy_living);
        }

        const auto sos_spirits_in_range = FoundSpirit(player_data, spirits_in_range, SOS1_AGENT_ID) &&
                                          FoundSpirit(player_data, spirits_in_range, SOS2_AGENT_ID) &&
                                          FoundSpirit(player_data, spirits_in_range, SOS3_AGENT_ID);

        return player_started_fight && !sos_spirits_in_range;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast;
    };

    return SmartUseSkill(skill_id,
                         skill_class,
                         "SoS",
                         player_data,
                         hero_data,
                         player_conditions,
                         hero_conditions,
                         wait_ms,
                         target_logic,
                         true);
}

bool HeroWindow::UseFallback()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Fall_Back;
    constexpr static auto skill_class = GW::Constants::Profession::Paragon;
    constexpr static auto wait_ms = 250UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &) { return true; };

    auto hero_conditions = [](const DataPlayer &, const Hero &) {
        return !DataPlayer::PlayerOrHeroHasEffect(GW::Constants::SkillID::Fall_Back);
    };

    return SmartUseSkill(skill_id,
                         skill_class,
                         "FallBack",
                         player_data,
                         hero_data,
                         player_conditions,
                         hero_conditions,
                         wait_ms,
                         target_logic,
                         true);
}

bool HeroWindow::UseBipOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Blood_is_Power;
    constexpr static auto skill_class = GW::Constants::Profession::Necromancer;
    constexpr static auto wait_ms = 600UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;
    const static auto energy_class_map = std::map<GW::Constants::Profession, std::pair<uint32_t, float>>{
        {GW::Constants::Profession::Warrior, {25U, 0.70F}},
        {GW::Constants::Profession::Ranger, {25U, 0.60F}},
        {GW::Constants::Profession::Monk, {30U, 0.50F}},
        {GW::Constants::Profession::Necromancer, {30U, 0.50F}},
        {GW::Constants::Profession::Mesmer, {30U, 0.50F}},
        {GW::Constants::Profession::Elementalist, {40U, 0.40F}},
        {GW::Constants::Profession::Assassin, {25U, 0.60F}},
        {GW::Constants::Profession::Ritualist, {30U, 0.50F}},
        {GW::Constants::Profession::Paragon, {25U, 0.60F}},
        {GW::Constants::Profession::Dervish, {25U, 0.50F}},
    };

    auto player_conditions = [](const DataPlayer &player_data) {
        if (!player_data.living)
            return false;

        if (player_data.living->energy_regen > 0.03F) // Dont have bip yet
            return false;

        const auto [enrgy_treshold_abs, enrgy_treshold_perc] = energy_class_map.at(player_data.primary);
        const auto [energy, _, energy_perc] = GetEnergyData();
        if (energy_perc > enrgy_treshold_perc && energy > enrgy_treshold_abs)
            return false;

        return true;
    };

    auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);
        const auto is_close_enough = dist < GW::Constants::Range::Spellcast + 300.0F;
        const auto hero_has_enough_hp = hero.hero_living->hp > 0.50F;

        return is_close_enough && hero_has_enough_hp;
    };

    return SmartUseSkill(skill_id,
                         skill_class,
                         "BiP",
                         player_data,
                         hero_data,
                         player_conditions,
                         hero_conditions,
                         wait_ms,
                         target_logic,
                         false);
}

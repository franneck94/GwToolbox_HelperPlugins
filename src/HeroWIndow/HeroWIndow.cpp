#include <array>
#include <cmath>
#include <cstdint>
#include <random>

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
#include "Defines.h"
#include "Helper.h"
#include "HelperAgents.h"
#include "HelperHero.h"
#include "HelperMaps.h"
#include "HelperPlayer.h"
#include "HelperSkill.h"
#include "HeroSmartSkills.h"
#include "HeroWindow.h"
#include "Logger.h"
#include "Utils.h"
#include "UtilsGui.h"
#include "UtilsMath.h"

#include <imgui.h>

namespace
{
#define GAME_MSG_SKILL_ON_ENEMY 45

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

    const auto player_pos = GetPlayerPos();
    const auto ping_distance = GW::GetDistance(ping_agent->pos, player_pos);
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

    if (value_id != GAME_MSG_SKILL_ON_ENEMY)
        return;

    const auto target_agent = GetTargetAsLiving();
    if (!target_agent)
        return;

    if (!target_agent || target_agent->allegiance != GW::Constants::Allegiance::Enemy)
        return;

    const auto player_pos = GetPlayerPos();
    const auto target_dist = GW::GetDistance(target_agent->pos, player_pos);
    if (target_dist < GW::Constants::Range::Spellcast)
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

    GW::UI::RegisterUIMessageCallback(&AgentCalled_Entry, GW::UI::UIMessage::kSendInteractEnemy, OnEnemyInteract);

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericValue>(
        &GenericValueTarget_Entry,
        [this](const GW::HookStatus *, const GW::Packet::StoC::GenericValue *packet) -> void {
            const uint32_t value_id = packet->value_id;
            const uint32_t caster_id = packet->agent_id;
            OnSkillOnEnemy(value_id, caster_id);
        });

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::MapLoaded>(&MapLoaded_Entry, OnMapLoad);

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
    if (!ValidateData(HelperActivationConditions, true) || (*GetVisiblePtr()) == false)
        return;

    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (!party_info || party_info->heroes.size() == 0)
        return;

    ImGui::SetNextWindowSize(ImVec2(240.0F, 45.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(), show_closebutton ? GetVisiblePtr() : nullptr, GetWinFlags(ImGuiWindowFlags_NoResize)))
    {
        const auto width = ImGui::GetWindowWidth();
        const auto im_button_size = ImVec2{width / 3.0F - 10.0F, 50.0F};

        toggled_follow = false;

        HeroBehaviour_DrawAndLogic(im_button_size);
        HeroFollow_DrawAndLogic(im_button_size);
        HeroSpike_DrawAndLogic(im_button_size);
    }
    ImGui::End();

    // #ifdef _DEBUG
    //     if (show_debug_map)
    //     {
    //         // const auto enemies_in_aggro_of_player = AgentLivingData::AgentsInRange(player_pos,
    //         //                                                                        GW::Constants::Allegiance::Enemy,
    //         //                                                                        GW::Constants::Range::Compass);

    //         // const auto enemy_center_pos = AgentLivingData::ComputeCenterOfMass(enemies_in_aggro_of_player);
    //         // const auto player_pos = player_pos;
    //         // const auto [center_player_m, center_player_b] = ComputeLine(enemy_center_pos, player_pos);
    //         // const auto [dividing_m, dividing_b] =
    //         //     ComputePerpendicularLineAtPos(center_player_m, center_player_b, player_pos);
    //         // const auto a = ComputePositionOnLine(player_pos, center_player_m, center_player_b, 500.0F);

    //         // DrawFlaggingFeature(player_pos, enemies_in_aggro_of_player, "Flagging");
    //     }
    // #endif
}

void HeroWindow::Update(float)
{
    if (!ValidateData(HelperActivationConditions, true))
    {
        ResetData();
        return;
    }

    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (!party_info || party_info->heroes.size() == 0)
        return;

    UpdateInternalData();

    HeroSmarterFollow_Main();
    HeroSmarterFlagging_Main();
}

/* MAIN CYCLE FUNCTIONS */


void HeroWindow::UpdateInternalData()
{
    const auto player_pos = GetPlayerPos();
    if (player_pos == follow_pos && following_active)
    {
        ms_with_no_pos_change = TIMER_DIFF(time_at_last_pos_change);
    }
    else
    {
        ms_with_no_pos_change = 0U;
        time_at_last_pos_change = TIMER_INIT();
    }
    follow_pos = player_pos;

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

/* INTERNAL FUNCTIONS FOLLOW */

void HeroWindow::FollowPlayer()
{
    if (!IsMapReady() || !IsExplorable() || (follow_pos.x == 0.0F && follow_pos.y == 0.0F) ||
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

        Helper::Hero::SetHerosBehaviour(current_hero_behaviour);
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

    if (ImGui::Button("Follow", im_button_size))
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

    const auto player_pos = GetPlayerPos();
    const auto num_enemies_at_player = AgentLivingData::NumAgentsInRange(player_pos,
                                                                         GW::Constants::Allegiance::Enemy,
                                                                         GW::Constants::Range::Spellcast);
    const auto no_pos_change_limit = num_enemies_at_player > 0 ? 5'000L : 2'000L;

    if (ms_with_no_pos_change >= no_pos_change_limit)
    {
        StopFollowing();
        ms_with_no_pos_change = 0U;
        time_at_last_pos_change = TIMER_INIT();
    }

    if (IsAttacking() && following_active)
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
            const auto target_dist = GW::GetDistance(target_agent->pos, player_pos);
            const auto is_melee_player = IsMeleeClass() && HoldsMeleeWeapon();
            const auto stop_distance =
                is_melee_player ? GW::Constants::Range::Spellcast - 500.0F : GW::Constants::Range::Spellcast;

            if (target_dist < stop_distance)
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

        HeroSmartSkills::UseFallback();
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

    if (!IsMoving())
        move_time_ms = clock();

    const auto player_pos = GetPlayerPos();
    const auto num_enemies_at_player = AgentLivingData::NumAgentsInRange(player_pos,
                                                                         GW::Constants::Allegiance::Enemy,
                                                                         GW::Constants::Range::Spellcast);
    const auto min_moving_time = num_enemies_at_player > 0 ? 5'000L : 2'000L;

    auto start_follow_bc_moving = TIMER_DIFF(move_time_ms) > min_moving_time;
    const auto target_agent = GetTargetAsLiving();
    if (!target_agent)
    {
        if (start_follow_bc_moving)
            StartFollowing();
        return;
    }

    if (target_agent->allegiance == GW::Constants::Allegiance::Enemy)
    {
        const auto target_dist = GW::GetDistance(target_agent->pos, player_pos);
        if (target_dist < GW::Constants::Range::Spellcast)
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

    const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
    if (!me_living)
        return;

    const auto hero_living_vec = Helper::Hero::GetPlayersHerosAsLivings();
    auto hero_idx = 0U;
    for (const auto &hero_living : hero_living_vec)
    {
        if (!hero_living || !hero_living->agent_id || !me_living->GetIsMoving())
        {
            same_position_counters.at(hero_idx) = 0U;
            ++hero_idx;
            continue;
        }

        const auto *hero_agent = GW::Agents::GetAgentByID(hero_living->agent_id);
        const auto player_pos = GetPlayerPos();
        if (!hero_agent || GW::GetDistance(player_pos, hero_agent->pos) > (GW::Constants::Range::Compass - 500.0F))
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
            Log::Info("Hero %d might be stuck!", hero_idx + 1U);
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

    const auto player_pos = GetPlayerPos();
    const auto target_distance = GW::GetDistance(player_pos, target->pos);

    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (!party_info || !party_info->heroes.valid())
        return;

    for (const auto &hero : party_info->heroes)
    {
        const auto *hero_living = GW::Agents::GetAgentByID(hero.agent_id);
        if (!hero_living)
            continue;

        if (GW::GetDistance(player_pos, hero_living->pos) > 800.0F || target_distance > 800.0F)
        {
            HeroSmartSkills::UseFallback();
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
    if (!IsMapReady() || following_active)
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

    Helper::Hero::SetHerosBehaviour(current_hero_behaviour);
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

        HeroSmartSkills::AttackTarget();
    }
}

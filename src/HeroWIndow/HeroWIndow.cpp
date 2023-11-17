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
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Managers/UIMgr.h>

#include "ActionTypes.h"
#include "ActionsBase.h"
#include "DataPlayer.h"
#include "Helper.h"
#include "HelperAgents.h"
#include "HelperMaps.h"
#include "HelperPackets.h"
#include "Logger.h"
#include "Utils.h"

#include <imgui.h>

#include "HeroWindow.h"

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static HeroWindow instance;
    return &instance;
}

void HeroWindow::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::MapLoaded>(
        &MapLoaded_Entry,
        [this](GW::HookStatus *, const GW::Packet::StoC::MapLoaded *) -> void { ResetData(); });
}

void HeroWindow::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

static bool HasWaitedLongEnough(const long timer_threshold_ms)
{
    static auto timer_last_cast_ms = clock();

    const auto last_cast_diff_ms = TIMER_DIFF(timer_last_cast_ms);
    if (last_cast_diff_ms < timer_threshold_ms)
        return false;

    timer_last_cast_ms = clock();
    return true;
}

static void HeroUseSkill(const uint32_t hero_agent_id,
                         const uint32_t target_agent_id,
                         const GW::Constants::SkillID skill_id)
{
    struct HeroUseSkill_s
    {
        const uint32_t header = GAME_CMSG_HERO_USE_SKILL;
        uint32_t hero_agent_id;
        uint32_t skill_id;
        uint32_t call_target;
        uint32_t target_id;
    };
    static HeroUseSkill_s pak;
    pak.hero_agent_id = hero_agent_id;
    pak.skill_id = (uint32_t)(skill_id);
    pak.call_target = 0U;
    pak.target_id = target_agent_id;
    GW::CtoS::SendPacket(&pak);
}

static bool HeroCastSkillIfAvailable(const GW::HeroPartyMember &hero,
                                     const GW::AgentLiving *hero_living,
                                     const uint32_t target_agent_id,
                                     const GW::Constants::SkillID skill_id)
{
    if (!hero_living || skill_id == GW::Constants::SkillID::No_Skill)
        return false;

    const auto skillbar_array = GW::SkillbarMgr::GetSkillbarArray();
    if (!skillbar_array)
        return false;

    for (const auto &skillbar : *skillbar_array)
    {
        if (skillbar.agent_id != hero_living->agent_id)
            continue;

        for (const auto &skill : skillbar.skills)
        {
            const auto has_skill_in_skillbar = skill.skill_id == skill_id;
            if (!has_skill_in_skillbar)
                continue;

            const auto skill_data = GW::SkillbarMgr::GetSkillConstantData(skill_id);
            if (!skill_data)
                continue;
            const auto hero_energy = (hero_living->energy * hero_living->max_energy);

            if (has_skill_in_skillbar && skill.GetRecharge() == 0 && hero_energy >= skill_data->GetEnergyCost())
            {
                HeroUseSkill(hero.agent_id, target_agent_id, skill.skill_id);
                return true;
            }
        }
    }

    return false;
}

uint32_t HeroWindow::GetNumPlayerHeroes()
{
    if (!party_heros)
        return 0U;

    auto num = uint32_t{0};
    for (const auto &hero : *party_heros)
    {
        if (hero.owner_player_id == player_data.living->login_number)
            ++num;
    }
    return num;
}

void HeroWindow::ToggleHeroBehaviour()
{
    if (!IsMapReady() || !party_heros)
        return;

    Log::Info("Toggle hero hehaviour!");

    if (current_hero_behaviour == HeroBehaviour::GUARD)
        current_hero_behaviour = HeroBehaviour::AVOID_COMBAT;
    else if (current_hero_behaviour == HeroBehaviour::ATTACK)
        current_hero_behaviour = HeroBehaviour::GUARD;
    else if (current_hero_behaviour == HeroBehaviour::AVOID_COMBAT)
        current_hero_behaviour = HeroBehaviour::ATTACK;

    for (const auto &hero : *party_heros)
    {
        if (hero.owner_player_id == player_data.living->login_number)
            GW::CtoS::SendPacket(0xC,
                                 GAME_CMSG_HERO_BEHAVIOR,
                                 hero.agent_id,
                                 static_cast<uint8_t>(current_hero_behaviour));
    }
}

void HeroWindow::FollowPlayer()
{
    if (!IsMapReady() || !IsExplorable() || (follow_pos.x == 0.0F && follow_pos.y == 0.0F) ||
        !GW::PartyMgr::GetIsPartyLoaded() || GW::PartyMgr::GetIsPartyDefeated())
        return;

    GW::PartyMgr::FlagAll(follow_pos);
}

void HeroWindow::UseBipOnPlayer()
{
    constexpr static auto wait_time_ms = 1000;

    if (!IsMapReady() || !IsExplorable() || !party_heros)
        return;

    if (!HasWaitedLongEnough(wait_time_ms))
        return;

    if (player_data.energy_perc > 0.30F)
        return;

    const auto effects = GetEffects(player_data.id);
    if (!effects)
        return;

    for (const auto effect : *effects)
    {
        if (effect.skill_id == GW::Constants::SkillID::Blood_is_Power && effect.agent_id == player_data.id)
        {
            return;
        }
    }

    if (player_data.living->energy_regen > 0.03F)
        return;

    for (const auto &hero : *party_heros)
    {
        if (hero.owner_player_id == player_data.living->login_number)
        {
            const auto hero_agent = GW::Agents::GetAgentByID(hero.agent_id);
            if (!hero_agent)
                continue;
            const auto hero_living = hero_agent->GetAsAgentLiving();
            if (!hero_living)
                continue;

            const auto dist = GW::GetDistance(hero_living->pos, player_data.pos);

            if (hero_living->primary == static_cast<uint8_t>(GW::Constants::Profession::Necromancer) &&
                dist < GW::Constants::Range::Spellcast && hero_living->hp > 0.80F)
            {
                if (HeroCastSkillIfAvailable(hero, hero_living, player_data.id, GW::Constants::SkillID::Blood_is_Power))
                {
                    Log::Info("Casted BIP.");
                    return;
                }
            }
        }
    }
}

bool HeroWindow::MesmerSpikeTarget(const GW::HeroPartyMember &hero)
{
    if (!IsMapReady() || !IsExplorable())
        return false;

    const auto hero_agent = GW::Agents::GetAgentByID(hero.agent_id);
    if (!hero_agent)
        return false;
    const auto hero_living = hero_agent->GetAsAgentLiving();
    if (!hero_living)
        return false;

    if (hero_living->primary == static_cast<uint8_t>(GW::Constants::Profession::Mesmer))
    {
        if (HeroCastSkillIfAvailable(hero, hero_living, target_agent_id, GW::Constants::SkillID::Energy_Surge))
            return true;
    }

    return false;
}

void HeroWindow::UseFallback()
{
    constexpr static auto wait_time_ms = 200;

    if (!IsMapReady() || !IsExplorable() || !party_heros)
        return;

    if (!HasWaitedLongEnough(wait_time_ms))
        return;

    const auto effects = GetEffects(player_data.id);
    if (!effects)
        return;

    for (const auto effect : *effects)
    {
        if (effect.skill_id == GW::Constants::SkillID::Fall_Back)
            return;
    }

    for (const auto &hero : *party_heros)
    {
        if (hero.owner_player_id == player_data.living->login_number)
        {
            const auto hero_agent = GW::Agents::GetAgentByID(hero.agent_id);
            if (!hero_agent)
                continue;
            const auto hero_living = hero_agent->GetAsAgentLiving();
            if (!hero_living)
                continue;

            if (hero_living->secondary == static_cast<uint8_t>(GW::Constants::Profession::Paragon))
            {
                if (HeroCastSkillIfAvailable(hero, hero_living, target_agent_id, GW::Constants::SkillID::Fall_Back))
                {
                    Log::Info("Used Fall Back.");
                    return;
                }
            }
        }
    }
}

void HeroWindow::AttackTarget()
{
    if (!IsMapReady() || !IsExplorable() || !target_agent_id || !party_heros)
        return;

    const auto target_agent = GW::Agents::GetAgentByID(target_agent_id);
    if (!target_agent)
        return;
    const auto target_living = target_agent->GetAsAgentLiving();
    if (!target_living)
        return;

    if (target_living->allegiance != GW::Constants::Allegiance::Enemy)
        return;

    Log::Info("Heroes will attack the players target!");

    for (const auto &hero : *party_heros)
    {
        if (hero.owner_player_id == player_data.living->login_number)
        {
            GW::CtoS::SendPacket(0xC, GAME_CMSG_HERO_LOCK_TARGET, hero.agent_id, target_agent_id);

            (void)MesmerSpikeTarget(hero);
        }
    }
}

void HeroWindow::ResetData()
{
    party_heros = nullptr;
    target_agent_id = 0U;
    follow_pos = GW::GamePos{};
    following_active = false;
}

void HeroWindow::Draw(IDirect3DDevice9 *)
{
    static auto last_follow_trigger_ms = clock();
    static auto gen = std::mt19937{};
    static auto time_dist = std::uniform_int_distribution<long>(-10, 10);

    if (!player_data.ValidateData(HelperActivationConditions, true))
        return;

    if (!party_heros || party_heros->size() == 0 || GetNumPlayerHeroes() == 0)
        return;

    if ((*GetVisiblePtr()) == false)
        return;

    ImGui::SetNextWindowSize(ImVec2(240.0F, 45.0F), ImGuiCond_Always);
    if (ImGui::Begin(Name(),
                     can_close && show_closebutton ? GetVisiblePtr() : nullptr,
                     GetWinFlags(ImGuiWindowFlags_NoResize)))
    {
        const auto width = ImGui::GetWindowWidth();
        auto added_color_follow = false;
        auto toggled_follow = false;

        switch (current_hero_behaviour)
        {
        case HeroBehaviour::GUARD:
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1F, 0.1F, 1.0F, 1.0F));
            break;
        }
        case HeroBehaviour::AVOID_COMBAT:
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1F, 0.9F, 0.1F, 1.0F));
            break;
        }
        case HeroBehaviour::ATTACK:
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0F, 0.1F, 0.1F, 1.0F));
            break;
        }
        }
        if (ImGui::Button("Behaviour###toggleState", ImVec2{width / 3.0F - 10.0F, 50.0F}))
        {
            ToggleHeroBehaviour();
        }
        ImGui::PopStyleColor();
        if (following_active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1F, 0.9F, 0.1F, 1.0F));
            added_color_follow = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Follow###followPlayer", ImVec2{width / 3.0F - 10.0F, 50.0F}))
        {
            following_active = !following_active;
            toggled_follow = true;

            Log::Info("Heroes will follow the player!");
        }
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
        else if (IsExplorable() && !following_active)
        {
            UseBipOnPlayer();
        }
        if (added_color_follow)
            ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button("Attack###attackTarget", ImVec2{width / 3.0F - 10.0F, 50.0F}))
        {
            if (IsExplorable())
                AttackTarget();
        }
    }
    ImGui::End();
}

void HeroWindow::Update(float)
{
    if (!player_data.ValidateData(HelperActivationConditions, true))
    {
        ResetData();
        return;
    }
    player_data.Update();

    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (party_info)
    {
        party_heros = &party_info->heroes;
    }
    else
    {
        ResetData();
        return;
    }

    follow_pos = player_data.pos;
    if (player_data.target)
        target_agent_id = player_data.target->agent_id;
    else
        target_agent_id = 0U;
}

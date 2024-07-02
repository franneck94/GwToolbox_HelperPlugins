#include <array>
#include <cmath>
#include <cstdint>
#include <random>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Utilities/Hooker.h>

#include "DataLivings.h"
#include "Helper.h"
#include "HelperHero.h"
#include "HelperMaps.h"
#include "SmartST.h"
#include "Utils.h"

#include <imgui.h>

namespace
{
bool UseShelterInFight()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Shelter;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = Helper::Hero::TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto player_pos = GetPlayerPos();
        const auto num_enemies_in_aggro_of_player =
            AgentLivingData::NumAgentsInRange(player_pos,
                                              GW::Constants::Allegiance::Enemy,
                                              GW::Constants::Range::Spellcast + 200.0F);

        const auto player_started_fight = num_enemies_in_aggro_of_player >= 3 && IsFighting();
        const auto has_skill_already = PlayerHasEffect(GW::Constants::SkillID::Shelter);

        return !has_skill_already && player_started_fight;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist = GW::GetDistance(hero_living->pos, player_pos);

        return dist < GW::Constants::Range::Spirit - 100.0F;
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "Shelter",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           true);
}

bool UseUnionInFight()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Union;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = Helper::Hero::TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto player_pos = GetPlayerPos();
        const auto num_enemies_in_aggro_of_player =
            AgentLivingData::NumAgentsInRange(player_pos,
                                              GW::Constants::Allegiance::Enemy,
                                              GW::Constants::Range::Spellcast + 200.0F);

        const auto player_started_fight = num_enemies_in_aggro_of_player >= 3 && IsFighting();
        const auto has_skill_already = PlayerHasEffect(GW::Constants::SkillID::Union);

        return !has_skill_already && player_started_fight;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist = GW::GetDistance(hero_living->pos, player_pos);

        return dist < GW::Constants::Range::Spirit - 100.0F;
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "Union",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           true);
}
} // namespace

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static SmartST instance;
    return &instance;
}

void SmartST::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

bool SmartST::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void SmartST::Terminate()
{
    ToolboxPlugin::Terminate();
}

void SmartST::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);

    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"Smart ST");
}

void SmartST::Update(float)
{
    if (!ValidateData(HelperActivationConditions, true))
        return;

    HeroSmarterSkills_Main();
}

bool SmartST::HeroSmarterSkills_Main()
{
    if (!IsMapReady() || !IsExplorable())
        return false;

    UseShelterInFight();
    UseUnionInFight();

    return false;
}
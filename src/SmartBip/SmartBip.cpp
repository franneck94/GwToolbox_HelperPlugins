#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <random>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Hero.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Utilities/Hooker.h>

#include "Helper.h"
#include "HelperAgents.h"
#include "HelperHero.h"
#include "HelperMaps.h"
#include "SmartBip.h"
#include "Utils.h"

#include <imgui.h>

namespace
{
bool UseBipOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Blood_is_Power;
    constexpr static auto skill_class = GW::Constants::Profession::Necromancer;
    constexpr static auto wait_ms = 1'000UL;
    constexpr static auto target_logic = Helper::Hero::TargetLogic::NO_TARGET;
    constexpr static auto ignore_effect_agent_id = true;
    constexpr static auto check_for_effect = true;
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

    auto player_conditions = []() {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (me_living->energy_regen > 0.03F)
            return false;

        const auto primary = GetPrimaryClass();
        const auto [enrgy_treshold_abs, enrgy_treshold_perc] = energy_class_map.at(primary);
        const auto [energy, _, energy_perc] = GetEnergyData();
        if (energy_perc > enrgy_treshold_perc && energy > enrgy_treshold_abs)
            return false;

        return true;
    };

    auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist = GW::GetDistance(hero_living->pos, player_pos);
        const auto is_close_enough = dist < GW::Constants::Range::Spellcast + 300.0F;
        const auto hero_has_enough_hp = hero_living->hp > 0.50F;

        return is_close_enough && hero_has_enough_hp;
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "BiP",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           ignore_effect_agent_id,
                                           check_for_effect);
}
} // namespace

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static SmartBip instance;
    return &instance;
}

void SmartBip::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

bool SmartBip::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void SmartBip::Terminate()
{
    ToolboxPlugin::Terminate();
}

void SmartBip::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);

    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"SmartBip");
}

void SmartBip::Update(float)
{
    if (!ValidateData(HelperActivationConditions, true))
        return;

    HeroSmarterSkills_Main();
}

bool SmartBip::HeroSmarterSkills_Main()
{
    if (!IsMapReady() || !IsExplorable())
        return false;

    if (UseBipOnPlayer())
        return true;

    return false;
}

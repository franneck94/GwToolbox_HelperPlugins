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
#include "SmartMeleeBuffs.h"
#include "Utils.h"

#include <imgui.h>

namespace
{
bool UseSplinterOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Splinter_Weapon;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = Helper::Hero::TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto player_pos = GetPlayerPos();
        const auto num_enemies_at_player = AgentLivingData::NumAgentsInRange(player_pos,
                                                                             GW::Constants::Allegiance::Enemy,
                                                                             GW::Constants::Range::Nearby);

        const auto player_is_melee_attacking = HoldsMeleeWeapon();
        const auto player_is_melee_class = IsMeleeClass;

        return num_enemies_at_player >= 2 && player_is_melee_attacking && player_is_melee_class;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist = GW::GetDistance(hero_living->pos, player_pos);

        return dist < GW::Constants::Range::Spellcast && hero_living->energy > 0.25F;
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "Splinter",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           false);
}

bool UseVigSpiritOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Vigorous_Spirit;
    constexpr static auto skill_class = GW::Constants::Profession::Monk;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = Helper::Hero::TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto player_pos = GetPlayerPos();
        const auto num_enemies_at_player = AgentLivingData::NumAgentsInRange(player_pos,
                                                                             GW::Constants::Allegiance::Enemy,
                                                                             GW::Constants::Range::Nearby);

        const auto player_is_melee_attacking = HoldsMeleeWeapon();
        const auto player_is_melee_class = IsMeleeClass;

        return num_enemies_at_player > 1 && player_is_melee_attacking && player_is_melee_class;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist = GW::GetDistance(hero_living->pos, player_pos);

        return dist < GW::Constants::Range::Spellcast && hero_living->energy > 0.25F;
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "Vig Spirit",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           false);
}
} // namespace

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static SmartMeleeBuffs instance;
    return &instance;
}

void SmartMeleeBuffs::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

bool SmartMeleeBuffs::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void SmartMeleeBuffs::Terminate()
{
    ToolboxPlugin::Terminate();
}

void SmartMeleeBuffs::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);

    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"SmartMeleeBuffs");
}

void SmartMeleeBuffs::Update(float)
{
    if (!ValidateData(HelperActivationConditions, true))
        return;

    HeroSmarterSkills_Main();
}

bool SmartMeleeBuffs::HeroSmarterSkills_Main()
{
    if (!IsMapReady() || !IsExplorable())
        return false;

    // UseSplinterOnPlayer();
    UseVigSpiritOnPlayer();

    return false;
}

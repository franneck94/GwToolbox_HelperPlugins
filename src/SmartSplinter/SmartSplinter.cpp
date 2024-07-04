#include <array>
#include <cmath>
#include <cstdint>
#include <random>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Utilities/Hooker.h>

#include "DataLivings.h"
#include "Helper.h"
#include "HelperHero.h"
#include "HelperMaps.h"
#include "SmartSplinter.h"
#include "Utils.h"

#include <imgui.h>

namespace
{
bool UseSplinterOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Splinter_Weapon;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 1000UL;
    constexpr static auto target_logic = Helper::Hero::TargetLogic::NO_TARGET;
    constexpr static auto ignore_effect_agent_id = true;
    constexpr static auto check_for_effect = true;

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
                                           ignore_effect_agent_id,
                                           check_for_effect);
}
} // namespace

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static SmartSplinter instance;
    return &instance;
}

void SmartSplinter::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

bool SmartSplinter::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void SmartSplinter::Terminate()
{
    ToolboxPlugin::Terminate();
}

void SmartSplinter::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);

    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"SmartSplinter");
}

void SmartSplinter::Update(float)
{
    if (!ValidateData(HelperActivationConditions, true))
        return;

    const auto *party_info = GW::PartyMgr::GetPartyInfo();
    if (!party_info || party_info->heroes.size() == 0)
        return;

    HeroSmarterSkills_Main();
}

bool SmartSplinter::HeroSmarterSkills_Main()
{
    if (!IsMapReady() || !IsExplorable())
        return false;

    UseSplinterOnPlayer();

    return false;
}

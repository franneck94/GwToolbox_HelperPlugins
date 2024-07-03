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
#include "HelperAgents.h"
#include "HelperHero.h"
#include "HelperMaps.h"
#include "SmartSoS.h"
#include "Utils.h"

#include <imgui.h>

namespace
{
bool UseSosInFight()
{
    constexpr static auto SOS1_AGENT_ID = uint32_t{4229};
    constexpr static auto SOS2_AGENT_ID = uint32_t{4230};
    constexpr static auto SOS3_AGENT_ID = uint32_t{4231};

    constexpr static auto skill_id = GW::Constants::SkillID::Signet_of_Spirits;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = Helper::Hero::TargetLogic::NO_TARGET;
    constexpr static auto ignore_effect_agent_id = false;
    constexpr static auto check_for_effect = false;

    auto player_conditions = []() {
        const auto agents_ptr = GW::Agents::GetAgentArray();
        if (!agents_ptr)
            return false;
        auto &agents = *agents_ptr;

        const auto player_pos = GetPlayerPos();
        const auto num_enemies_in_aggro_of_player = AgentLivingData::NumAgentsInRange(player_pos,
                                                                                      GW::Constants::Allegiance::Enemy,
                                                                                      GW::Constants::Range::Spellcast);

        auto spirits_in_range = std::vector<GW::AgentLiving *>{};
        for (auto *enemy : agents)
        {
            if (!enemy)
                continue;

            const auto dist_to_enemy = GW::GetDistance(player_pos, enemy->pos);
            if (dist_to_enemy > GW::Constants::Range::Spellcast + 200.0F)
                continue;

            if (!enemy)
                continue;
            const auto enemy_living = enemy->GetAsAgentLiving();
            if (!enemy_living || enemy_living->allegiance != GW::Constants::Allegiance::Spirit_Pet)
                continue;

            spirits_in_range.push_back(enemy_living);
        }

        const auto num_sos_spirits_in_range = static_cast<uint32_t>(FoundSpirit(spirits_in_range, SOS1_AGENT_ID)) +
                                              static_cast<uint32_t>(FoundSpirit(spirits_in_range, SOS2_AGENT_ID)) +
                                              static_cast<uint32_t>(FoundSpirit(spirits_in_range, SOS3_AGENT_ID));

        if (num_enemies_in_aggro_of_player > 6U)
            return (IsFighting() || CanFight()) && num_sos_spirits_in_range < 3U;
        else if (num_enemies_in_aggro_of_player > 4U)
            return (IsFighting() || CanFight()) && num_sos_spirits_in_range < 2U;
        else if (num_enemies_in_aggro_of_player > 2U)
            return (IsFighting() || CanFight()) && num_sos_spirits_in_range < 1U;

        return false;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist = GW::GetDistance(hero_living->pos, player_pos);

        return dist < GW::Constants::Range::Spellcast;
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "SoS",
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
    static SmartSoS instance;
    return &instance;
}

void SmartSoS::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

bool SmartSoS::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void SmartSoS::Terminate()
{
    ToolboxPlugin::Terminate();
}

void SmartSoS::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);

    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"Smart SoS");
}

void SmartSoS::Update(float)
{
    if (!ValidateData(HelperActivationConditions, true))
        return;

    const auto *party_info = GW::PartyMgr::GetPartyInfo();
    if (!party_info || party_info->heroes.size() == 0)
        return;

    HeroSmarterSkills_Main();
}

bool SmartSoS::HeroSmarterSkills_Main()
{
    if (!IsMapReady() || !IsExplorable())
        return false;

    UseSosInFight();

    return false;
}

#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <tuple>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameEntities/Agent.h>

#include "HelperPlayer.h"

namespace Helper
{
namespace Hero
{
enum class TargetLogic
{
    NO_TARGET,
    PLAYER_TARGET,
    SEARCH_TARGET,
};

void SetHerosBehaviour(const GW::HeroBehavior hero_behaviour);

bool HeroUseSkill_Main(const GW::Constants::SkillID skill_id,
                       const GW::Constants::Profession skill_class,
                       const std::string_view skill_name,
                       std::function<bool()> player_conditions,
                       std::function<bool(const GW::AgentLiving *hero_living)> hero_conditions,
                       const long wait_ms,
                       const TargetLogic target_logic = TargetLogic::NO_TARGET,
                       const bool ignore_effect_agent_id = false,
                       const bool check_for_effect = false);

bool PlayerHasHerosInParty();

uint32_t NumPlayersHerosInParty();

std::vector<GW::AgentLiving *> GetPlayersHerosAsLivings();

std::map<GW::Constants::Profession, std::vector<uint32_t>> GetPlayersHerosClassMaps(
    const std::vector<GW::AgentLiving *> &players_heros);

std::vector<uint32_t> GetPlayersHeroIdxsWithClass(
    const std::map<GW::Constants::Profession, std::vector<uint32_t>> &players_hero_class_idx_map,
    const GW::Constants::Profession skill_class);

void FlagHeros(const GW::GamePos &follow_pos);

void UnflagHeros();
} // namespace Hero
} // namespace Helper

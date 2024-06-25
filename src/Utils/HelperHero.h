#pragma once

#include <cstdint>
#include <functional>
#include <tuple>

#include <GWCA/Constants/Constants.h>

#include "DataHero.h"
#include "DataPlayer.h"

enum class TargetLogic
{
    NO_TARGET,
    PLAYER_TARGET,
    SEARCH_TARGET,
};

bool HeroUseSkill(const uint32_t target_agent_id, const uint32_t skill_idx, const uint32_t hero_idx_zero_based);

bool HeroCastSkillIfAvailable(const Hero &hero,
                              const DataPlayer &player_data,
                              const GW::Constants::SkillID skill_id,
                              std::function<bool(const DataPlayer &, const Hero &)> cb_fn,
                              const TargetLogic target_logic,
                              const uint32_t target_id = 0);

std::tuple<uint32_t, bool> SkillIdxOfHero(const Hero &hero, const GW::Constants::SkillID skill_id);

void SetHerosBehaviour(const uint32_t player_login_number, const GW::HeroBehavior hero_behaviour);

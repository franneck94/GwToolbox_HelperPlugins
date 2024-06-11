#pragma once

#include <cstdint>
#include <functional>
#include <tuple>

#include <GWCA/Constants/Constants.h>

#include "DataHero.h"
#include "DataPlayer.h"

bool HeroUseSkill(const uint32_t target_agent_id, const uint32_t skill_idx, const uint32_t hero_idx_zero_based);

bool HeroCastSkillIfAvailable(const Hero &hero_data,
                              const DataPlayer &player_data,
                              const GW::Constants::SkillID skill_id,
                              std::function<bool(const DataPlayer &, const Hero &)> cb_fn,
                              const bool use_player_target);

std::tuple<uint32_t, bool> SkillIdxOfHero(const Hero &hero, const GW::Constants::SkillID skill_id);

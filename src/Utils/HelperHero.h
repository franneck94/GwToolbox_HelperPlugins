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

                              const GW::Constants::SkillID skill_id,
                              std::function<bool(const Hero &)> cb_fn,
                              const TargetLogic target_logic,
                              const uint32_t target_id = 0);

std::tuple<uint32_t, bool> SkillIdxOfHero(const Hero &hero, const GW::Constants::SkillID skill_id);

void SetHerosBehaviour(const uint32_t player_login_number, const GW::HeroBehavior hero_behaviour);


bool HeroSkill_StartConditions(const GW::Constants::SkillID skill_id,
                               const long wait_ms = 0UL,
                               const bool ignore_effect_agent_id = true);

bool SmartUseSkill(const GW::Constants::SkillID skill_id,
                   const GW::Constants::Profession skill_class,
                   const std::string_view skill_name,

                   const HeroData &hero_data,
                   std::function<bool()> player_conditions,
                   std::function<bool(const Hero &)> hero_conditions,
                   const long wait_ms,
                   const TargetLogic target_logic = TargetLogic::NO_TARGET,
                   const uint32_t current_target_id = 0U,
                   const bool ignore_effect_agent_id = false);

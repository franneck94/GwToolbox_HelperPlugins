#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <tuple>
#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Context/GameContext.h>
#include <GWCA/Context/WorldContext.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Hero.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/PartyMgr.h>

#include "HelperAgents.h"
#include "HelperHero.h"
#include "HelperMaps.h"
#include "HelperSkill.h"
#include "Utils.h"

namespace HeroSmartSkills
{
void AttackTarget()
{
    const static auto skill_class_pairs = std::vector<std::tuple<GW::Constants::SkillID, GW::Constants::Profession>>{
        {GW::Constants::SkillID::Energy_Surge, GW::Constants::Profession::Mesmer},
    };
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = Helper::Hero::TargetLogic::PLAYER_TARGET;

    auto player_conditions = []() {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        const auto target = GW::Agents::GetTarget();
        if (!target)
            return false;

        const auto *target_living = target->GetAsAgentLiving();
        if (!target_living || target_living->allegiance != GW::Constants::Allegiance::Enemy)
            return false;

        return true;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (!me_living || !hero_living)
            return false;

        return true;
    };

    for (const auto &[skill_id, skill_class] : skill_class_pairs)
    {
        Helper::Hero::HeroUseSkill_Main(skill_id,
                                        skill_class,
                                        "Mesmer Spike",
                                        player_conditions,
                                        hero_conditions,
                                        wait_ms,
                                        target_logic,
                                        false);
    }
}

bool UseFallback()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Fall_Back;
    constexpr static auto skill_class = GW::Constants::Profession::Paragon;
    constexpr static auto wait_ms = 250UL;
    constexpr static auto target_logic = Helper::Hero::TargetLogic::NO_TARGET;

    auto player_conditions = []() { return true; };

    auto hero_conditions = [](const GW::AgentLiving *) {
        return !PlayerOrHeroHasEffect(GW::Constants::SkillID::Fall_Back);
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "FallBack",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           true);
}

} // namespace HeroSmartSkills

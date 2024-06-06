#include <cstdint>
#include <functional>

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
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Utilities/Scanner.h>

#include "DataHero.h"
#include "DataPlayer.h"
#include "HelperHero.h"

void HeroUseSkill(const uint32_t target_agent_id, const uint32_t skill_idx, const uint32_t hero_idx_zero_based)
{
    auto hero_action = GW::UI::ControlAction_Hero1Skill1;
    if (hero_idx_zero_based == 0)
        hero_action = GW::UI::ControlAction_Hero1Skill1;
    else if (hero_idx_zero_based == 1)
        hero_action = GW::UI::ControlAction_Hero2Skill1;
    else if (hero_idx_zero_based == 2)
        hero_action = GW::UI::ControlAction_Hero3Skill1;
    else if (hero_idx_zero_based == 3)
        hero_action = GW::UI::ControlAction_Hero4Skill1;
    else if (hero_idx_zero_based == 4)
        hero_action = GW::UI::ControlAction_Hero5Skill1;
    else if (hero_idx_zero_based == 5)
        hero_action = GW::UI::ControlAction_Hero6Skill1;
    else if (hero_idx_zero_based == 6)
        hero_action = GW::UI::ControlAction_Hero7Skill1;
    else
        return;

    const auto curr_target_id = GW::Agents::GetTargetId();

    GW::GameThread::Enqueue([=] {
        if (target_agent_id && target_agent_id != GW::Agents::GetTargetId())
            GW::Agents::ChangeTarget(target_agent_id);
        GW::UI::Keypress((GW::UI::ControlAction)(static_cast<uint32_t>(hero_action) + skill_idx));
        if (curr_target_id && target_agent_id != curr_target_id)
            GW::Agents::ChangeTarget(curr_target_id);
    });
}

bool HeroCastSkillIfAvailable(const Hero &hero,
                              const DataPlayer &player_data,
                              const GW::Constants::SkillID skill_id,
                              std::function<bool(const DataPlayer &, const Hero &)> cb_fn,
                              const bool use_player_target)
{
    if (!hero.hero_living || !hero.hero_living->agent_id)
        return false;

    if (!cb_fn(player_data, hero))
        return false;

    auto skill_idx = 0U;
    for (const auto &skill : hero.skills)
    {
        const auto has_skill_in_skillbar = skill.skill_id == skill_id;
        if (!has_skill_in_skillbar)
        {
            ++skill_idx;
            continue;
        }

        const auto *skill_data = GW::SkillbarMgr::GetSkillConstantData(skill_id);
        if (!skill_data)
        {
            ++skill_idx;
            continue;
        }

        const auto hero_energy = (static_cast<uint32_t>(hero.hero_living->energy) * hero.hero_living->max_energy);

        if (has_skill_in_skillbar && skill.GetRecharge() == 0 && hero_energy >= skill_data->GetEnergyCost())
        {
            HeroUseSkill(use_player_target ? player_data.target->agent_id : player_data.id,
                         skill_idx,
                         hero.hero_idx_zero_based);
            return true;
        }

        ++skill_idx;
    }

    return false;
}

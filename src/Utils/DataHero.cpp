#include <array>
#include <cstdint>
#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Managers/PlayerMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>

#include "DataHero.h"

bool HeroData::Update(const GW::Array<GW::HeroPartyMember> &heroes_array)
{
    hero_vec.clear();
    hero_class_idx_map.clear();

    auto hero_idx_zero_based = 0U;
    auto skills = std::array<GW::SkillbarSkill, 8U>{};

    for (const auto &hero : heroes_array)
    {
        if (!hero.agent_id)
        {
            ++hero_idx_zero_based;
            continue;
        }

        auto *hero_agent = GW::Agents::GetAgentByID(hero.agent_id);
        if (!hero_agent)
        {
            ++hero_idx_zero_based;
            continue;
        }
        auto *hero_living = hero_agent->GetAsAgentLiving();
        if (!hero_living)
        {
            ++hero_idx_zero_based;
            continue;
        }

        const auto primary = static_cast<GW::Constants::Profession>(hero_living->primary);
        const auto secondary = static_cast<GW::Constants::Profession>(hero_living->secondary);

        hero_class_idx_map[primary].push_back(hero_idx_zero_based);
        hero_class_idx_map[secondary].push_back(hero_idx_zero_based);

        const auto *skillbar_array = GW::SkillbarMgr::GetSkillbarArray();
        if (!skillbar_array)
        {
            ++hero_idx_zero_based;
            continue;
        }

        for (const auto &skillbar : *skillbar_array)
        {
            if (skillbar.agent_id == hero_living->agent_id)
            {
                for (uint32_t i = 0; i < 8; i++)
                {
                    skills[i] = skillbar.skills[i];
                }

                break;
            }
        }

        auto hero_data = Hero{.hero_living = hero_living, .hero_idx_zero_based = hero_idx_zero_based, .skills = skills};
        hero_vec.push_back(hero_data);

        ++hero_idx_zero_based;
    }

    return true;
}

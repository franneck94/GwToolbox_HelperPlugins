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

HeroData::HeroData()
{
    hero_class_idx_map[GW::Constants::Profession::Warrior] = {};
    hero_class_idx_map[GW::Constants::Profession::Ranger] = {};
    hero_class_idx_map[GW::Constants::Profession::Monk] = {};
    hero_class_idx_map[GW::Constants::Profession::Necromancer] = {};
    hero_class_idx_map[GW::Constants::Profession::Mesmer] = {};
    hero_class_idx_map[GW::Constants::Profession::Elementalist] = {};
    hero_class_idx_map[GW::Constants::Profession::Assassin] = {};
    hero_class_idx_map[GW::Constants::Profession::Ritualist] = {};
    hero_class_idx_map[GW::Constants::Profession::Paragon] = {};
    hero_class_idx_map[GW::Constants::Profession::Dervish] = {};
}

bool HeroData::Update()
{

    hero_vec.clear();
    hero_class_idx_map.clear();

    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (!party_info)
        return false;

    auto hero_idx_zero_based = 0U;
    for (const auto &hero : party_info->heroes)
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

        auto hero_data = Hero{.hero_living = hero_living, .hero_idx_zero_based = hero_idx_zero_based};
        hero_vec.push_back(hero_data);

        ++hero_idx_zero_based;
    }

    return true;
}

#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Managers/PartyMgr.h>

struct Hero
{
public:
    const GW::AgentLiving *hero_living;
    const uint32_t hero_idx_zero_based;
};

class HeroData
{
public:
    HeroData();

    std::vector<Hero> hero_vec;
    std::map<GW::Constants::Profession, std::vector<uint32_t>> hero_class_idx_map;

    bool Update();
};

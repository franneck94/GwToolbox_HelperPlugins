#pragma once

#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>

#include "Logger.h"

struct AgentLivingData
{
    AgentLivingData() : allies({}), neutrals({}), enemies({}), spirits({}), minions({}), npcs({}) {};
    ~AgentLivingData() {};

    void Update();

    static std::vector<const GW::AgentLiving *> UpdateType(const GW::Constants::Allegiance type);

    size_t NumEnemiesInRange(const GW::GamePos &player_pos, const float range) const;

    static size_t NumAgentsInRange(const GW::AgentArray &agents,
                                   const GW::GamePos &player_pos,
                                   const GW::Constants::Allegiance allegiance,
                                   const float range);

    std::vector<const GW::AgentLiving *> allies;
    std::vector<const GW::AgentLiving *> neutrals;
    std::vector<const GW::AgentLiving *> enemies;
    std::vector<const GW::AgentLiving *> spirits;
    std::vector<const GW::AgentLiving *> minions;
    std::vector<const GW::AgentLiving *> npcs;
};

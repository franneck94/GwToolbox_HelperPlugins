#pragma once

#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>

#include "Logger.h"

struct AgentLivingData
{
    AgentLivingData() : allies({}), neutrals({}), enemies({}), spirits({}), minions({}), npcs({}){};
    ~AgentLivingData()
    {
        Log::Info("Destroyed Living Data");
    };

    void Update();

    static void UpdateType(std::vector<const GW::AgentLiving *> &filtered_agents, const GW::Constants::Allegiance type);

    size_t NumEnemiesInRange(const GW::GamePos &player_pos, const float range) const;

    std::vector<const GW::AgentLiving *> allies;
    std::vector<const GW::AgentLiving *> neutrals;
    std::vector<const GW::AgentLiving *> enemies;
    std::vector<const GW::AgentLiving *> spirits;
    std::vector<const GW::AgentLiving *> minions;
    std::vector<const GW::AgentLiving *> npcs;
};

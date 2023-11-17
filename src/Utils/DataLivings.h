#pragma once

#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameEntities/Agent.h>

struct AgentLivingData
{
    AgentLivingData() : allies({}), neutrals({}), enemies({}), spirits({}), minions({}), npcs({}){};
    ~AgentLivingData(){};

    void Update();
    void UpdateType(std::vector<GW::AgentLiving *> &filtered_agents, const GW::Constants::Allegiance type);

    std::vector<GW::AgentLiving *> allies;
    std::vector<GW::AgentLiving *> neutrals;
    std::vector<GW::AgentLiving *> enemies;
    std::vector<GW::AgentLiving *> spirits;
    std::vector<GW::AgentLiving *> minions;
    std::vector<GW::AgentLiving *> npcs;
};

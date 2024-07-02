#pragma once

#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>

#include "Logger.h"

class AgentLivingData
{
public:
    AgentLivingData() : allies({}), neutrals({}), enemies({}), spirits({}), minions({}), npcs({}) {};
    ~AgentLivingData() {};

    void Update();

    static std::vector<GW::AgentLiving *> UpdateType(const GW::Constants::Allegiance type);

    size_t NumEnemiesInRange(const GW::GamePos &player_pos, const float range) const;

    static size_t NumAgentsInRange(const GW::GamePos &player_pos,
                                   const GW::Constants::Allegiance allegiance,
                                   const float range);

    static std::vector<GW::AgentLiving *> AgentsInRange(const GW::GamePos &player_pos,
                                                        const GW::Constants::Allegiance allegiance,
                                                        const float range);

    static GW::GamePos ComputeCenterOfMass(const std::vector<GW::AgentLiving *> &agents);

public:
    std::vector<GW::AgentLiving *> allies;
    std::vector<GW::AgentLiving *> neutrals;
    std::vector<GW::AgentLiving *> enemies;
    std::vector<GW::AgentLiving *> spirits;
    std::vector<GW::AgentLiving *> minions;
    std::vector<GW::AgentLiving *> npcs;
};

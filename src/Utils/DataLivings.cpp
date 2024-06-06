#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/MapMgr.h>

#include "DataLivings.h"

void AgentLivingData::Update()
{
    UpdateType(allies, GW::Constants::Allegiance::Ally_NonAttackable);
    UpdateType(enemies, GW::Constants::Allegiance::Enemy);
    UpdateType(minions, GW::Constants::Allegiance::Minion);
    UpdateType(neutrals, GW::Constants::Allegiance::Neutral);
    UpdateType(npcs, GW::Constants::Allegiance::Npc_Minipet);
    UpdateType(spirits, GW::Constants::Allegiance::Spirit_Pet);
}

void AgentLivingData::UpdateType(std::vector<GW::AgentLiving *> &filtered_agents, const GW::Constants::Allegiance type)
{
    filtered_agents.clear();

    const auto agents = GW::Agents::GetAgentArray();
    if (!agents || !agents->valid())
        return;

    for (const auto agent : *agents)
    {
        if (!agent)
            continue;

        const auto living = agent->GetAsAgentLiving();
        if (!living)
            continue;

        if (living->allegiance != type)
            continue;

        if (type != GW::Constants::Allegiance::Ally_NonAttackable && living->GetIsDead())
            continue;

        filtered_agents.push_back(living);
    }
}

size_t AgentLivingData::NumEnemiesInRange(const GW::GamePos &player_pos, const float range) const
{
    return std::count_if(enemies.begin(), enemies.end(), [=](const GW::AgentLiving *enemy_living) {
        if (!enemy_living)
            return false;

        const auto dist = GW::GetDistance(enemy_living->pos, player_pos);

        return dist < range;
    });
}

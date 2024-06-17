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
    allies = UpdateType(GW::Constants::Allegiance::Ally_NonAttackable);
    enemies = UpdateType(GW::Constants::Allegiance::Enemy);
    minions = UpdateType(GW::Constants::Allegiance::Minion);
    neutrals = UpdateType(GW::Constants::Allegiance::Neutral);
    npcs = UpdateType(GW::Constants::Allegiance::Npc_Minipet);
    spirits = UpdateType(GW::Constants::Allegiance::Spirit_Pet);
}

std::vector<const GW::AgentLiving *> AgentLivingData::UpdateType(const GW::Constants::Allegiance type)
{
    auto filtered_agents = std::vector<const GW::AgentLiving *>{};

    const auto agents_ptr = GW::Agents::GetAgentArray();
    if (!agents_ptr || !agents_ptr->valid())
        return filtered_agents;

    auto &agents = *agents_ptr;

    for (const auto agent : agents)
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

    return filtered_agents;
}

size_t AgentLivingData::NumEnemiesInRange(const GW::GamePos &player_pos, const float range) const
{
    return std::count_if(enemies.begin(), enemies.end(), [=](const auto *enemy_living) {
        if (!enemy_living)
            return false;

        const auto dist = GW::GetDistance(enemy_living->pos, player_pos);

        return dist < range;
    });
}

size_t AgentLivingData::NumAgentsInRange(const GW::AgentArray &agents,
                                         const GW::GamePos &player_pos,
                                         const GW::Constants::Allegiance allegiance,
                                         const float range)
{
    return std::count_if(agents.begin(), agents.end(), [=](const auto *enemy_living) {
        if (!enemy_living)
            return false;

        const auto dist = GW::GetDistance(enemy_living->pos, player_pos);

        return dist < range;
    });
}

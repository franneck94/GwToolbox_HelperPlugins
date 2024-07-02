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

std::vector<GW::AgentLiving *> AgentLivingData::UpdateType(const GW::Constants::Allegiance type)
{
    auto filtered_agents = std::vector<GW::AgentLiving *>{};

    const auto agents_ptr = GW::Agents::GetAgentArray();
    if (!agents_ptr || !agents_ptr->valid())
        return filtered_agents;

    auto &agents = *agents_ptr;

    for (const auto agent : agents)
    {
        const auto living = agent ? agent->GetAsAgentLiving() : nullptr;
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
    return NumAgentsInRange(player_pos, GW::Constants::Allegiance::Enemy, range);
}

size_t AgentLivingData::NumAgentsInRange(const GW::GamePos &player_pos,
                                         const GW::Constants::Allegiance allegiance,
                                         const float range)
{
    const auto agents_ptr = GW::Agents::GetAgentArray();
    if (!agents_ptr)
        return 0;
    auto &agents = *agents_ptr;

    return std::count_if(agents.begin(), agents.end(), [=](const GW::Agent *enemy) {
        if (!enemy)
            return false;

        const auto enemy_living = enemy->GetAsAgentLiving();
        if (!enemy_living)
            return false;

        if (enemy_living->allegiance != allegiance)
            return false;

        const auto dist = GW::GetDistance(enemy_living->pos, player_pos);
        return dist < range;
    });
}

std::vector<GW::AgentLiving *> AgentLivingData::AgentsInRange(const GW::GamePos &player_pos,
                                                              const GW::Constants::Allegiance allegiance,
                                                              const float range)
{
    const auto agents_ptr = GW::Agents::GetAgentArray();
    if (!agents_ptr)
        return {};
    auto &agents = *agents_ptr;

    auto agents_vec = std::vector<GW::AgentLiving *>{};
    for (auto *enemy : agents)
    {
        if (!enemy)
            continue;

        auto enemy_living = enemy->GetAsAgentLiving();
        if (!enemy_living)
            continue;

        if (enemy_living->allegiance != allegiance)
            continue;

        const auto dist = GW::GetDistance(enemy_living->pos, player_pos);
        if (dist > range)
            continue;

        agents_vec.push_back(enemy_living);
    }

    return agents_vec;
}

GW::GamePos AgentLivingData::ComputeCenterOfMass(const std::vector<GW::AgentLiving *> &agents)
{
    auto sum_x = 0.0f;
    auto sum_y = 0.0f;

    for (const auto *agent : agents)
    {
        sum_x += agent->pos.x;
        sum_y += agent->pos.y;
    }

    float center_x = sum_x / agents.size();
    float center_y = sum_y / agents.size();

    return GW::GamePos{center_x, center_y, 0U};
}

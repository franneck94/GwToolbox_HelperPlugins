#pragma once

#include <cstdint>
#include <set>
#include <tuple>
#include <vector>

#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/EffectMgr.h>
#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/PartyMgr.h>

#include "ActionsBase.h"
#include "DataLivings.h"
#include "HelperPlayer.h"
#include "DataSkill.h"

bool CanMove();

bool IsAliveAlly(const GW::Agent *target);

const GW::EffectArray *GetEffects(const uint32_t target_agent_id);

bool DetectPlayerIsDead();

std::tuple<uint32_t, uint32_t, float> GetEnergyData();

std::tuple<uint32_t, uint32_t, float> GetHpData();

bool AgentHasBuff(const GW::Constants::SkillID buff_skill_id, const uint32_t target_agent_id);

void TargetAndAttackEnemyInAggro(const std::vector<GW::AgentLiving *> &enemies,
                                 const float range = GW::Constants::Range::Spellcast);

bool CastBondIfNotAvailable(const DataSkill &skill_data, const uint32_t target_id);

bool TargetNearest(const GW::GamePos &player_pos,
                   const std::vector<GW::AgentLiving *> &livings,
                   const float max_distance = GW::Constants::SqrRange::Compass);

std::vector<GW::AgentLiving *> FilterById(const std::vector<GW::AgentLiving *> &livings, const uint32_t id);

void FilterByIdAndDistance(const GW::GamePos &player_pos,
                           const std::vector<GW::AgentLiving *> &livings,
                           std::vector<GW::AgentLiving *> &filtered_livings,
                           const uint32_t id,
                           const float max_distance = GW::Constants::Range::Compass);

template <uint32_t N>
void FilterByIdsAndDistances(const GW::GamePos &player_pos,
                             const std::vector<GW::AgentLiving *> &livings,
                             std::vector<GW::AgentLiving *> &filtered_livings,
                             const std::array<uint32_t, N> ids,
                             const float max_distance = GW::Constants::Range::Compass)
{
    for (auto living : livings)
    {
        for (const auto id : ids)
        {
            if (living->player_number == id && GW::GetDistance(player_pos, living->pos) < max_distance)
                filtered_livings.push_back(living);
        }
    }
}

void SortByDistance(std::vector<GW::AgentLiving *> &filtered_livings);

std::pair<const GW::AgentLiving *, float> GetClosestEnemy(const std::vector<GW::AgentLiving *> &enemies);

uint32_t GetClosestToPosition(const GW::GamePos &pos,
                              const std::vector<GW::AgentLiving *> &livings,
                              const uint32_t target_id);

uint32_t GetMostDistantEnemy(const GW::GamePos &pos, const std::vector<GW::AgentLiving *> &livings);

uint32_t GetClosestById(const std::vector<GW::AgentLiving *> &livings, const uint32_t id);

uint32_t GetClosestEnemyById(const std::vector<GW::AgentLiving *> &enemies, const uint32_t id);

uint32_t GetClosestAllyById(const std::vector<GW::AgentLiving *> &allies, const uint32_t id);

uint32_t GetClosestNpcbyId(const std::vector<GW::AgentLiving *> &npcs, const uint32_t id);

uint32_t TargetClosestEnemyById(const std::vector<GW::AgentLiving *> &enemies, const uint32_t id);

uint32_t TargetClosestAllyById(const std::vector<GW::AgentLiving *> &allies, const uint32_t id);

uint32_t TargetClosestNpcById(const std::vector<GW::AgentLiving *> &npcs, const uint32_t id);

std::vector<GW::AgentLiving *> FilterAgentsByRange(const std::vector<GW::AgentLiving *> &livings,

                                                   const float dist_threshold = GW::Constants::Range::Earshot);

struct PlayerMapping
{
    uint32_t id;
    uint32_t party_idx;
};

bool GetPartyMembers(std::vector<PlayerMapping> &party_members);

std::set<uint32_t> FilterAgentIDS(const std::vector<GW::AgentLiving *> &filtered_livings,
                                  const std::set<uint32_t> &filter_ids);

uint32_t GetPartyIdxByID(const uint32_t id);

bool DropEmoBondsOnLiving(const GW::AgentLiving *living);

const GW::AgentLiving *GetPlayerAsLiving();

const GW::AgentLiving *GetTargetAsLiving();

bool FoundSpirit(const std::vector<GW::AgentLiving *> &spirits,
                 const uint32_t spirit_id,
                 const float range = GW::Constants::Range::Spellcast);

bool DoNeedEnchNow(const GW::Constants::SkillID ench_id, const float time_offset_s);

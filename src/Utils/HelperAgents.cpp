#include <algorithm>
#include <cstdint>
#include <iterator>
#include <set>
#include <tuple>
#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Context/CharContext.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Managers/PlayerMgr.h>

#include "ActionsBase.h"
#include "DataPlayer.h"
#include "DataSkill.h"
#include "Helper.h"
#include "HelperMaps.h"

#include "HelperAgents.h"

bool CanMove()
{
    return !IsLoading() && !GW::Map::GetIsObserving();
}

bool IsAliveAlly(const GW::Agent *target)
{
    if (!target)
        return false;

    if (!target->GetIsLivingType())
        return false;

    const auto target_living = target->GetAsAgentLiving();
    if (!target_living)
        return false;

    if (target_living->allegiance != GW::Constants::Allegiance::Ally_NonAttackable || target_living->GetIsDead() ||
        target_living->hp == 0.00F)
        return false;

    return target_living->GetIsAlive();
}

const GW::EffectArray *GetEffects(const uint32_t agent_id)
{
    const auto agent_effects = GW::Effects::GetPartyEffectsArray();

    if (!agent_effects || !agent_effects->valid())
        return nullptr;

    for (const auto &agent_effect : *agent_effects)
    {
        if (agent_effect.agent_id == agent_id && agent_effect.effects.valid())
            return &agent_effect.effects;
    }

    return nullptr;
}

bool TargetNearest(const GW::GamePos &player_pos,
                   const std::vector<GW::AgentLiving *> &livings,
                   const float max_distance)
{
    auto distance = max_distance;
    auto closest = size_t{0};

    for (const auto living : livings)
    {
        if (!living)
            continue;

        const auto newDistance = GW::GetDistance(player_pos, living->pos);
        if (newDistance < distance)
        {
            closest = living->agent_id;
            distance = newDistance;
        }
    }

    if (closest)
    {
        GW::Agents::ChangeTarget(closest);
        return true;
    }

    return false;
}

bool DetectPlayerIsDead()
{
    const auto me = GW::Agents::GetPlayer();
    if (!me)
        return false;

    const auto living_me = me->GetAsAgentLiving();
    if (!living_me)
        return false;

    return living_me->GetIsDead();
}

std::tuple<uint32_t, uint32_t, float> GetEnergy()
{
    const auto me = GW::Agents::GetPlayer();
    if (!me)
        return std::make_tuple(0, 0, 0.0F);

    const auto living_me = me->GetAsAgentLiving();
    if (!living_me)
        return std::make_tuple(0, 0, 0.0F);

    const auto max_energy = living_me->max_energy;
    const auto energy_perc = living_me->energy;
    const auto energy = static_cast<float>(max_energy) * energy_perc;

    return std::make_tuple(static_cast<uint32_t>(energy), max_energy, energy_perc);
}

std::tuple<uint32_t, uint32_t, float> GetHp()
{
    const auto me = GW::Agents::GetPlayer();
    if (!me)
        return std::make_tuple(0, 0, 0.0F);

    const auto living_me = me->GetAsAgentLiving();
    if (!living_me)
        return std::make_tuple(0, 0, 0.0F);

    const auto max_hp = living_me->max_hp;
    const auto hp_perc = living_me->hp;
    const auto hp = static_cast<float>(max_hp) * hp_perc;

    return std::make_tuple(static_cast<uint32_t>(hp), max_hp, hp_perc);
}

bool AgentHasBuff(const GW::Constants::SkillID buff_skill_id, const uint32_t target_agent_id)
{
    const auto effects = GW::Effects::GetPartyEffectsArray();
    if (!effects || !effects->valid())
        return false;

    const auto &buffs = (*effects)[0].buffs;
    if (!buffs.valid())
        return false;

    for (size_t i = 0; i < buffs.size(); ++i)
    {
        const auto agent_id = buffs[i].target_agent_id;
        const auto skill_id = buffs[i].skill_id;

        if (agent_id == target_agent_id)
        {
            if (skill_id == buff_skill_id)
                return true;
        }
    }

    return false;
}

void TargetAndAttackEnemyInAggro(const DataPlayer &player_data,
                                 const std::vector<GW::AgentLiving *> &enemies,
                                 const float range)
{
    if (!player_data.target || !player_data.target->agent_id || !player_data.target->GetIsLivingType() ||
        player_data.target->GetAsAgentLiving()->allegiance != GW::Constants::Allegiance::Enemy)
        TargetNearest(player_data.pos, enemies, range);

    if (player_data.target && player_data.target->agent_id)
    {
        const auto dist = GW::GetDistance(player_data.pos, player_data.target->pos);
        if (dist < range)
            AttackAgent(player_data.target);
    }
}

bool CastBondIfNotAvailable(const DataSkill &skill_data, const uint32_t target_id, const DataPlayer *const player_data)
{
    const auto has_bond = AgentHasBuff(static_cast<GW::Constants::SkillID>(skill_data.id), target_id);
    const auto bond_avail = skill_data.CanBeCasted(player_data->energy);

    if (!has_bond && bond_avail)
    {
        GW::GameThread::Enqueue([&]() { GW::SkillbarMgr::UseSkill(skill_data.idx, target_id); });
        return true;
    }
    return false;
}

std::pair<GW::Agent *, float> GetClosestEnemy(const DataPlayer *player_data,
                                              const std::vector<GW::AgentLiving *> &enemies)
{
    GW::AgentLiving *closest = nullptr;
    auto closest_dist = FLT_MAX;

    for (const auto enemy : enemies)
    {
        if (!enemy)
            continue;

        if (enemy->allegiance != GW::Constants::Allegiance::Enemy)
            continue;

        const auto dist = GW::GetDistance(player_data->pos, enemy->pos);
        if (dist < closest_dist)
        {
            closest = enemy;
            closest_dist = dist;
        }
    }

    return std::make_pair(closest, closest_dist);
}

uint32_t GetClosestToPosition(const GW::GamePos &pos,
                              const std::vector<GW::AgentLiving *> &livings,
                              const uint32_t target_id)
{
    auto closest_id = uint32_t{0U};
    auto closest_dist = FLT_MAX;

    for (const auto living : livings)
    {
        if (!living || target_id == living->agent_id)
            continue;

        const auto dist = GW::GetDistance(pos, living->pos);
        if (dist < closest_dist)
        {
            closest_dist = dist;
            closest_id = living->agent_id;
        }
    }

    return closest_id;
}


uint32_t GetMostDistantEnemy(const GW::GamePos &pos, const std::vector<GW::AgentLiving *> &livings)
{
    auto distant_id = uint32_t{0U};
    auto distant_dist = 0.0F;

    for (const auto living : livings)
    {
        if (!living)
            continue;

        const auto dist = GW::GetDistance(pos, living->pos);
        if (dist > distant_dist)
        {
            distant_dist = dist;
            distant_id = living->agent_id;
        }
    }

    return distant_id;
}

uint32_t GetClosestById(const DataPlayer &player_data, const std::vector<GW::AgentLiving *> &livings, const uint32_t id)
{
    auto closest_id = uint32_t{0U};
    auto closest_dist = FLT_MAX;

    for (const auto living : livings)
    {
        if (!living || id != living->player_number)
            continue;

        const auto dist = GW::GetDistance(player_data.pos, living->pos);
        if (dist < closest_dist)
        {
            closest_dist = dist;
            closest_id = living->agent_id;
        }
    }

    return closest_id;
}

uint32_t GetClosestEnemyById(const DataPlayer &player_data,
                             const std::vector<GW::AgentLiving *> &enemies,
                             const uint32_t id)
{
    return GetClosestById(player_data, enemies, id);
}

uint32_t GetClosestAllyById(const DataPlayer &player_data,
                            const std::vector<GW::AgentLiving *> &allies,
                            const uint32_t id)
{
    return GetClosestById(player_data, allies, id);
}

uint32_t GetClosestNpcbyId(const DataPlayer &player_data, const std::vector<GW::AgentLiving *> &npcs, const uint32_t id)
{
    return GetClosestById(player_data, npcs, id);
}

uint32_t TargetClosestEnemyById(DataPlayer &player_data,
                                const std::vector<GW::AgentLiving *> &enemies,
                                const uint32_t id)
{
    const auto target_id = GetClosestEnemyById(player_data, enemies, id);
    if (!target_id)
        return 0;

    player_data.ChangeTarget(target_id);

    return target_id;
}

uint32_t TargetClosestAllyById(DataPlayer &player_data, const std::vector<GW::AgentLiving *> &allies, const uint32_t id)
{
    const auto target_id = GetClosestAllyById(player_data, allies, id);
    if (!target_id)
        return 0;

    player_data.ChangeTarget(target_id);

    return target_id;
}

uint32_t TargetClosestNpcById(DataPlayer &player_data, const std::vector<GW::AgentLiving *> &npcs, const uint32_t id)
{
    const auto target_id = GetClosestNpcbyId(player_data, npcs, id);
    if (!target_id)
        return 0;

    player_data.ChangeTarget(target_id);

    return target_id;
}

void SortByDistance(const DataPlayer &player_data, std::vector<GW::AgentLiving *> &filtered_livings)
{
    const auto player_pos = player_data.pos;

    std::sort(filtered_livings.begin(), filtered_livings.end(), [&player_pos](const auto a1, const auto a2) {
        const auto sqrd1 = GW::GetSquareDistance(player_pos, a1->pos);
        const auto sqrd2 = GW::GetSquareDistance(player_pos, a2->pos);
        return sqrd1 < sqrd2;
    });
}

std::vector<GW::AgentLiving *> FilterAgentsByRange(const std::vector<GW::AgentLiving *> &livings,
                                                   const DataPlayer &player_data,
                                                   const float dist_threshold)
{
    auto filtered_livings = std::vector<GW::AgentLiving *>{};

    for (const auto living : livings)
    {
        const auto dist = GW::GetDistance(player_data.pos, living->pos);
        if (dist <= dist_threshold)
            filtered_livings.push_back(living);
    }

    return filtered_livings;
}

uint32_t GetPartyIdxByID(const uint32_t id)
{
    auto party_members = std::vector<PlayerMapping>{};
    const auto success = GetPartyMembers(party_members);

    if (!success)
        return 0U;

    const auto it = std::find_if(party_members.begin(), party_members.end(), [&id](const PlayerMapping &member) {
        return member.id == static_cast<uint32_t>(id);
    });
    if (it == party_members.end())
        return 0U;

    const auto idx = static_cast<uint32_t>(std::distance(party_members.begin(), it));
    if (idx >= GW::PartyMgr::GetPartySize())
        return 0U;

    return idx;
}

std::vector<GW::AgentLiving *> FilterById(const std::vector<GW::AgentLiving *> &livings, const uint32_t id)
{
    auto res = std::vector<GW::AgentLiving *>{};

    for (auto living : livings)
    {
        if (living->player_number == id)
            res.push_back(living);
    }

    return res;
}

void FilterByIdAndDistance(const GW::GamePos &player_pos,
                           const std::vector<GW::AgentLiving *> &livings,
                           std::vector<GW::AgentLiving *> &filtered_livings,
                           const uint32_t id,
                           const float max_distance)
{
    for (auto living : livings)
    {
        if (living->player_number == id && GW::GetDistance(player_pos, living->pos) < max_distance)
            filtered_livings.push_back(living);
    }
}

bool GetPartyMembers(std::vector<PlayerMapping> &party_members)
{
    if (!GW::PartyMgr::GetIsPartyLoaded())
        return false;
    if (!GW::Map::GetIsMapLoaded())
        return false;

    const auto info = GW::PartyMgr::GetPartyInfo();
    if (!info)
        return false;

    const auto players = GW::Agents::GetPlayerArray();
    if (!players || !players->valid())
        return false;

    party_members.clear();

    auto idx = uint32_t{0};
    for (const auto &player_data : info->players)
    {
        const auto id = (*players)[player_data.login_number].agent_id;
        party_members.push_back({id, idx});
        ++idx;

        for (const auto &hero : info->heroes)
        {
            if (hero.owner_player_id == player_data.login_number)
            {
                party_members.push_back({hero.agent_id, idx});
                ++idx;
            }
        }
    }
    for (const auto &hench : info->henchmen)
    {
        party_members.push_back({hench.agent_id, idx});
        ++idx;
    }

    return true;
}

std::set<uint32_t> FilterAgentIDS(const std::vector<GW::AgentLiving *> &filtered_livings,
                                  const std::set<uint32_t> &filter_ids)
{
    auto set_ids = std::set<uint32_t>{};
    auto result_ids = std::set<uint32_t>{};
    for (const auto living : filtered_livings)
    {
        set_ids.insert(static_cast<uint32_t>(living->player_number));
    }
    std::set_difference(set_ids.begin(),
                        set_ids.end(),
                        filter_ids.begin(),
                        filter_ids.end(),
                        std::inserter(result_ids, result_ids.end()));

    return result_ids;
}

bool DropBondsOnLiving(const GW::AgentLiving *living)
{
    auto dropped_smth = false;

    auto buffs = GW::Effects::GetPlayerBuffs();
    if (!buffs || !buffs->valid() || buffs->size() == 0)
        return false;

    for (const auto &buff : *buffs)
    {
        const auto agent_id = buff.target_agent_id;
        const auto skill = static_cast<GW::Constants::SkillID>(buff.skill_id);
        const auto is_prot_bond = skill == GW::Constants::SkillID::Protective_Bond;
        const auto is_life_bond = skill == GW::Constants::SkillID::Life_Bond;
        const auto is_balth_bond = skill == GW::Constants::SkillID::Balthazars_Spirit;
        const auto is_any_bond = is_prot_bond || is_life_bond || is_balth_bond;

        if (is_any_bond && living->agent_id == agent_id)
            GW::Effects::DropBuff(buff.buff_id);
    }

    return dropped_smth;
}

const GW::AgentLiving *GetPlayerAsLiving()
{
    const auto me = GW::Agents::GetPlayer();
    if (!me)
        return nullptr;
    return me->GetAsAgentLiving();
}

const GW::AgentLiving *GetTargetAsLiving()
{
    const auto me = GW::Agents::GetTarget();
    if (!me)
        return nullptr;
    return me->GetAsAgentLiving();
}

GW::Player *GetPlayerByName(const wchar_t *_name)
{
    const auto players = GW::Agents::GetPlayerArray();
    if (!players || !players->valid() || !_name)
        return nullptr;

    for (auto &player : *players)
    {
        if (!player.name)
            continue;
        if (_name == player.name)
            return &player;
    }

    return nullptr;
}

std::wstring GetPlayerName(uint32_t player_number)
{
    GW::Player *player = nullptr;
    if (!player_number)
    {
        player = GW::PlayerMgr::GetPlayerByID(GW::PlayerMgr::GetPlayerNumber());
        if (!player || !player->name)
        {
            // Map not loaded; try to get from character context
            auto c = GW::CharContext::instance();
            return c ? c->player_name : L"";
        }
    }
    else
    {
        player = GW::PlayerMgr::GetPlayerByID(player_number);
    }

    return player && player->name ? player->name : L"";
}

bool FoundSpirit(const DataPlayer &player_data,
                 const std::vector<GW::AgentLiving *> &spirits,
                 const uint32_t spirit_id,
                 const float range)
{
    if (spirits.size())
    {
        for (const auto spirit : spirits)
        {
            if (spirit->player_number == spirit_id && GW::GetDistance(spirit->pos, player_data.pos) < range)
                return true;
        }
    }

    return false;
}

bool DoNeedEnchNow(const DataPlayer *player_data, const GW::Constants::SkillID ench_id, const float time_offset_s)
{
    const auto found = player_data->HasEffect(ench_id);
    const auto data = GW::SkillbarMgr::GetSkillConstantData(ench_id);

    const auto remain_duration_s = player_data->GetRemainingEffectDuration(ench_id) / 1000.0F;
    const auto trigger_time_s = data ? data->activation + data->aftercast : 1.0F;

    if (!found || remain_duration_s <= (trigger_time_s + time_offset_s))
        return true;

    return false;
}

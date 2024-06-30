#include <cstdint>
#include <functional>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Hero.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Managers/UIMgr.h>

#include "ActionsBase.h"
#include "HelperHero.h"
#include "HelperPlayer.h"
#include "HelperSkill.h"

namespace
{
std::tuple<uint32_t, bool> SkillIdxOfHero(const GW::AgentLiving *hero_living, const GW::Constants::SkillID skill_id)
{
    constexpr static auto invalid_case = std::make_tuple(static_cast<uint32_t>(-1), false);

    if (!hero_living)
        return invalid_case;

    const auto hero_energy = static_cast<uint32_t>(hero_living->energy * static_cast<float>(hero_living->max_energy));

    auto skill_idx = 0U;
    const auto hero_skills = GetAgentSkillbar(hero_living->agent_id);
    if (!hero_skills)
        return invalid_case;

    for (const auto &skill : hero_skills->skills)
    {
        const auto has_skill_in_skillbar = skill.skill_id == skill_id;
        if (!has_skill_in_skillbar)
        {
            ++skill_idx;
            continue;
        }

        const auto *skill_data = GW::SkillbarMgr::GetSkillConstantData(skill_id);
        if (!skill_data)
        {
            ++skill_idx;
            continue;
        }

        const auto can_cast_skill = skill.GetRecharge() == 0 && hero_energy >= skill_data->GetEnergyCost();
        return std::make_tuple(skill_idx, can_cast_skill);
    }

    return invalid_case;
}

bool HeroUseSkill(const uint32_t target_agent_id, const uint32_t skill_idx, const uint32_t hero_idx_zero_based)
{
    auto hero_action = GW::UI::ControlAction_Hero1Skill1;
    if (hero_idx_zero_based == 0)
        hero_action = GW::UI::ControlAction_Hero1Skill1;
    else if (hero_idx_zero_based == 1)
        hero_action = GW::UI::ControlAction_Hero2Skill1;
    else if (hero_idx_zero_based == 2)
        hero_action = GW::UI::ControlAction_Hero3Skill1;
    else if (hero_idx_zero_based == 3)
        hero_action = GW::UI::ControlAction_Hero4Skill1;
    else if (hero_idx_zero_based == 4)
        hero_action = GW::UI::ControlAction_Hero5Skill1;
    else if (hero_idx_zero_based == 5)
        hero_action = GW::UI::ControlAction_Hero6Skill1;
    else if (hero_idx_zero_based == 6)
        hero_action = GW::UI::ControlAction_Hero7Skill1;
    else
        return false;

    const auto curr_target_id = GW::Agents::GetTargetId();
    auto success = true;

    GW::GameThread::Enqueue([=, &success] {
        if (target_agent_id && target_agent_id != GW::Agents::GetTargetId())
            success &= GW::Agents::ChangeTarget(target_agent_id);
        const auto keypress_id = (GW::UI::ControlAction)(static_cast<uint32_t>(hero_action) + skill_idx);
        success &= GW::UI::Keypress(keypress_id);
        if (curr_target_id && target_agent_id != curr_target_id)
            success &= GW::Agents::ChangeTarget(curr_target_id);
    });

    return success;
}

bool HeroCastSkillIfAvailable(const GW::AgentLiving *hero_living,
                              const uint32_t hero_idx,
                              const GW::Constants::SkillID skill_id,
                              std::function<bool(const GW::AgentLiving *)> cb_fn,
                              const TargetLogic target_logic,
                              const uint32_t target_id)
{
    if (!hero_living || !hero_living->agent_id)
        return false;

    if (!cb_fn(hero_living))
        return false;

    const auto [skill_idx, can_cast_skill] = SkillIdxOfHero(hero_living, skill_id);
    const auto has_skill_in_skillbar = skill_idx != static_cast<uint32_t>(-1);
    const auto player_id = GW::Agents::GetPlayerId();

    if (has_skill_in_skillbar && can_cast_skill)
    {
        switch (target_logic)
        {
        case TargetLogic::SEARCH_TARGET:
        case TargetLogic::PLAYER_TARGET:
        {
            const auto target = GW::Agents::GetTarget();
            if (target)
                return HeroUseSkill(target->agent_id, skill_idx, hero_idx);
            else
                return HeroUseSkill(player_id, skill_idx, hero_idx);
        }
        case TargetLogic::NO_TARGET:
        default:
        {
            return HeroUseSkill(player_id, skill_idx, hero_idx);
        }
        }
    }

    return false;
}

bool HeroSkill_StartConditions(const GW::Constants::SkillID skill_id,
                               const long wait_ms,
                               const bool ignore_effect_agent_id)
{
    if (!ActionABC::HasWaitedLongEnough(wait_ms))
        return false;

    if (skill_id != GW::Constants::SkillID::No_Skill)
        return true;

    if (PlayerHasEffect(skill_id, ignore_effect_agent_id))
        return false;

    return true;
}
} // namespace

namespace Helper
{
namespace Hero
{
void SetHerosBehaviour(const uint32_t player_login_number, const GW::HeroBehavior hero_behaviour)
{
    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (!party_info || !party_info->heroes.valid())
        return;

    for (const auto &hero : party_info->heroes)
    {
        if (hero.owner_player_id == player_login_number)
            GW::PartyMgr::SetHeroBehavior(hero.agent_id, hero_behaviour);
    }
}

bool HeroUseSkill_Main(const GW::Constants::SkillID skill_id,
                       const GW::Constants::Profession skill_class,
                       const std::string_view skill_name,
                       std::function<bool()> player_conditions,
                       std::function<bool(const GW::AgentLiving *hero_living)> hero_conditions,
                       const long wait_ms,
                       const TargetLogic target_logic,
                       const uint32_t current_target_id,
                       const bool ignore_effect_agent_id)
{
    if (!HeroSkill_StartConditions(skill_id, wait_ms, ignore_effect_agent_id))
        return false;

    if (!player_conditions())
        return false;

    const auto players_heros = GetPlayersHerosAsLivings();
    const auto players_hero_class_idx_map = GetPlayersHerosClassMaps(players_heros);
    const auto hero_idxs_with_class = GetPlayersHeroIdxsWithClass(players_hero_class_idx_map, skill_class);

    for (const auto hero_idx_zero_based : hero_idxs_with_class)
    {
        const auto hero_living = players_heros.at(hero_idx_zero_based);

        if (HeroCastSkillIfAvailable(hero_living,
                                     hero_idx_zero_based,
                                     skill_id,
                                     hero_conditions,
                                     target_logic,
                                     current_target_id))
        {
#ifdef _DEBUG
            Log::Info("Casted %s.", skill_name);
#else
            (void)skill_name;
#endif
            return true;
        }
    }

    return true;
}

bool PlayerHasHerosInParty()
{
    const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
    if (!me_living)
        return false;

    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (!party_info || !party_info->heroes.valid())
        return false;

    for (const auto &hero : party_info->heroes)
    {
        if (!hero.agent_id)
            continue;

        const auto hero_agent = GW::Agents::GetAgentByID(hero.agent_id);
        if (!hero_agent)
            continue;

        const auto hero_living = hero_agent->GetAsAgentLiving();
        if (!hero_living)
            continue;

        if (hero.owner_player_id == me_living->login_number)
            return true;
    }

    return false;
}

uint32_t NumPlayersHerosInParty()
{
    const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
    if (!me_living)
        return 0U;

    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (!party_info || !party_info->heroes.valid())
        return 0U;

    auto hero_counter = 0U;
    for (const auto &hero : party_info->heroes)
    {
        if (!hero.agent_id)
            continue;

        const auto hero_agent = GW::Agents::GetAgentByID(hero.agent_id);
        if (!hero_agent)
            continue;

        const auto hero_living = hero_agent->GetAsAgentLiving();
        if (!hero_living)
            continue;

        if (hero.owner_player_id == me_living->login_number)
            ++hero_counter;
    }

    return hero_counter;
}

std::vector<GW::AgentLiving *> GetPlayersHerosAsLivings()
{
    const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
    if (!me_living)
        return {};

    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (!party_info || !party_info->heroes.valid())
        return {};


    auto heros = std::vector<GW::AgentLiving *>{};
    for (auto &hero : party_info->heroes)
    {
        if (!hero.agent_id)
            continue;

        const auto hero_agent = GW::Agents::GetAgentByID(hero.agent_id);
        if (!hero_agent)
            continue;

        auto hero_living = hero_agent->GetAsAgentLiving();
        if (!hero_living)
            continue;

        if (hero.owner_player_id == me_living->login_number)
            heros.push_back(hero_living);
    }

    return heros;
}

std::map<GW::Constants::Profession, std::vector<uint32_t>> GetPlayersHerosClassMaps(
    const std::vector<GW::AgentLiving *> &players_heros)
{
    auto hero_class_idx_map = std::map<GW::Constants::Profession, std::vector<uint32_t>>{
        {GW::Constants::Profession::Warrior, {}},
        {GW::Constants::Profession::Ranger, {}},
        {GW::Constants::Profession::Monk, {}},
        {GW::Constants::Profession::Necromancer, {}},
        {GW::Constants::Profession::Mesmer, {}},
        {GW::Constants::Profession::Elementalist, {}},
        {GW::Constants::Profession::Assassin, {}},
        {GW::Constants::Profession::Ritualist, {}},
        {GW::Constants::Profession::Paragon, {}},
        {GW::Constants::Profession::Dervish, {}},
    };

    auto hero_idx_zero_based = 0U;
    for (const auto hero_living : players_heros)
    {
        if (!hero_living)
        {
            ++hero_idx_zero_based;
            continue;
        }

        const auto primary = static_cast<GW::Constants::Profession>(hero_living->primary);
        const auto secondary = static_cast<GW::Constants::Profession>(hero_living->secondary);

        hero_class_idx_map[primary].push_back(hero_idx_zero_based);
        hero_class_idx_map[secondary].push_back(hero_idx_zero_based);

        ++hero_idx_zero_based;
    }

    return hero_class_idx_map;
}

std::vector<uint32_t> GetPlayersHeroIdxsWithClass(
    const std::map<GW::Constants::Profession, std::vector<uint32_t>> &players_hero_class_idx_map,
    const GW::Constants::Profession skill_class)
{
    if (players_hero_class_idx_map.find(skill_class) == players_hero_class_idx_map.end())
        return {};

    auto hero_idxs_zero_based = players_hero_class_idx_map.at(skill_class);
    return hero_idxs_zero_based;
}
} // namespace Hero
} // namespace Helper

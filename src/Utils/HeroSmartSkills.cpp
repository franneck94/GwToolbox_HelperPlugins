#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <tuple>
#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Context/GameContext.h>
#include <GWCA/Context/WorldContext.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Hero.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/PartyMgr.h>

#include "HelperAgents.h"
#include "HelperHero.h"
#include "HelperMaps.h"
#include "HelperSkill.h"
#include "Utils.h"

namespace HeroSmartSkills
{
void AttackTarget()
{
    const static auto skill_class_pairs = std::vector<std::tuple<GW::Constants::SkillID, GW::Constants::Profession>>{
        {GW::Constants::SkillID::Energy_Surge, GW::Constants::Profession::Mesmer},
    };
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::PLAYER_TARGET;

    auto player_conditions = []() {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        const auto target = GW::Agents::GetTarget();
        if (!target)
            return false;

        const auto *target_living = target->GetAsAgentLiving();
        if (!target_living || target_living->allegiance != GW::Constants::Allegiance::Enemy)
            return false;

        return true;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (!me_living || !hero_living)
            return false;

        return true;
    };

    for (const auto &[skill_id, skill_class] : skill_class_pairs)
    {
        Helper::Hero::HeroUseSkill_Main(skill_id,
                                        skill_class,
                                        "Mesmer Spike",
                                        player_conditions,
                                        hero_conditions,
                                        wait_ms,
                                        target_logic,
                                        false);
    }
}

bool ShatterImportantHexes()
{
    constexpr static auto to_remove_hexes_melee = std::array{
        // Mesmer
        GW::Constants::SkillID::Ineptitude,
        GW::Constants::SkillID::Empathy,
        GW::Constants::SkillID::Crippling_Anguish,
        GW::Constants::SkillID::Clumsiness,
        GW::Constants::SkillID::Faintheartedness,
        // Necro
        GW::Constants::SkillID::Spiteful_Spirit,
        // Ele
        GW::Constants::SkillID::Blurred_Vision,
    };
    constexpr static auto to_remove_hexes_caster = std::array{
        // Mesmer
        GW::Constants::SkillID::Panic,
        GW::Constants::SkillID::Backfire,
        GW::Constants::SkillID::Mistrust,
        GW::Constants::SkillID::Power_Leech,
        // Necro
        GW::Constants::SkillID::Spiteful_Spirit,
        GW::Constants::SkillID::Soul_Leech,
    };
    constexpr static auto to_remove_hexes_all = std::array{
        // Mesmer
        GW::Constants::SkillID::Diversion,
        GW::Constants::SkillID::Visions_of_Regret,
        // Ele
        GW::Constants::SkillID::Deep_Freeze,
        GW::Constants::SkillID::Mind_Freeze,
    };
    constexpr static auto to_remove_hexes_paragon = std::array{
        // Necro
        GW::Constants::SkillID::Vocal_Minority,
    };

    const static auto skill_class_pairs = std::vector<std::tuple<GW::Constants::SkillID, GW::Constants::Profession>>{
        {GW::Constants::SkillID::Shatter_Hex, GW::Constants::Profession::Mesmer},
        {GW::Constants::SkillID::Remove_Hex, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Smite_Hex, GW::Constants::Profession::Monk},
    };
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (!me_living->GetIsHexed())
            return false;

        const auto *const effects = GW::Effects::GetPlayerEffects();
        if (!effects || !effects->valid())
            return false;

        auto found_hex = [](const auto &hex_array, const GW::Effect &curr_hex) {
            return std::find(hex_array.begin(), hex_array.end(), curr_hex.skill_id) != hex_array.end();
        };

        for (const auto &effect : *effects)
        {
            if (effect.agent_id != 0)
                continue;

            if (HoldsMeleeWeapon() && found_hex(to_remove_hexes_melee, effect))
                return true;

            if (!HoldsMeleeWeapon() && found_hex(to_remove_hexes_caster, effect))
                return true;

            if (found_hex(to_remove_hexes_all, effect))
                return true;

            const auto primary = GetPrimaryClass();
            if (primary == GW::Constants::Profession::Paragon && found_hex(to_remove_hexes_paragon, effect))
                return true;
        }

        return false;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (!me_living || !hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto distance = GW::GetDistance(player_pos, hero_living->pos);
        return distance <= GW::Constants::Range::Spellcast;
    };

    auto casted_skill = false;
    for (const auto &[skill_id, skill_class] : skill_class_pairs)
    {
        if (Helper::Hero::HeroUseSkill_Main(skill_id,
                                            skill_class,
                                            "Remove Hex",
                                            player_conditions,
                                            hero_conditions,
                                            wait_ms,
                                            target_logic,
                                            false))
            casted_skill = true;
    }

    return casted_skill;
}

bool RemoveImportantConditions()
{
    constexpr static auto to_remove_conditions_melee = std::array{
        GW::Constants::SkillID::Blind,
        GW::Constants::SkillID::Weakness,
    };
    constexpr static auto to_remove_conditions_caster = std::array{
        GW::Constants::SkillID::Dazed,
    };
    constexpr static auto to_remove_conditions_all = std::array{
        GW::Constants::SkillID::Crippled,
    };

    const static auto skill_class_pairs = std::vector<std::tuple<GW::Constants::SkillID, GW::Constants::Profession>>{
        {GW::Constants::SkillID::Mend_Body_and_Soul, GW::Constants::Profession::Ritualist},
        {GW::Constants::SkillID::Dismiss_Condition, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Mend_Condition, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Smite_Condition, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Purge_Conditions, GW::Constants::Profession::Monk},
    };
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (!me_living->GetIsConditioned())
            return false;

        const auto *const effects = GW::Effects::GetPlayerEffects();
        if (!effects || !effects->valid())
            return false;

        auto found_cond = [](const auto &cond_array, const GW::Effect &curr_cond) {
            return std::find(cond_array.begin(), cond_array.end(), curr_cond.skill_id) != cond_array.end();
        };

        for (const auto &effect : *effects)
        {
            if (effect.agent_id != 0)
                continue;

            if (HoldsMeleeWeapon())
            {
                if (found_cond(to_remove_conditions_melee, effect))
                    return true;
            }
            else
            {
                if (found_cond(to_remove_conditions_caster, effect))
                    return true;
            }

            if (found_cond(to_remove_conditions_all, effect))
                return true;
        }

        return false;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto distance = GW::GetDistance(player_pos, hero_living->pos);
        return distance <= GW::Constants::Range::Spellcast;
    };

    for (const auto &[skill_id, skill_class] : skill_class_pairs)
    {
        if (Helper::Hero::HeroUseSkill_Main(skill_id,
                                            skill_class,
                                            "Remove Cond",
                                            player_conditions,
                                            hero_conditions,
                                            wait_ms,
                                            target_logic,
                                            false))
            return true;
    }

    return false;
}

bool RuptEnemies()
{
    const static auto skill_class_pairs = std::vector<std::tuple<GW::Constants::SkillID, GW::Constants::Profession>>{
        {GW::Constants::SkillID::Cry_of_Frustration, GW::Constants::Profession::Mesmer},
        {GW::Constants::SkillID::Power_Drain, GW::Constants::Profession::Mesmer},
    };
    const static auto skills_to_rupt = std::array{
        // Mesmer
        GW::Constants::SkillID::Panic,
        GW::Constants::SkillID::Energy_Surge,
        // Necro
        GW::Constants::SkillID::Chilblains,
        // Ele
        GW::Constants::SkillID::Meteor,
        GW::Constants::SkillID::Meteor_Shower,
        GW::Constants::SkillID::Searing_Flames,
        // All
        GW::Constants::SkillID::Resurrection_Signet,

    };
    constexpr static auto wait_ms = 200UL;
    constexpr static auto target_logic = TargetLogic::SEARCH_TARGET;
    static auto last_time_target_changed = clock();

    const auto target = GW::Agents::GetTarget();
    auto player_target = target ? target->agent_id : 0;
    auto change_target_to_id = 0U;

    auto player_conditions = [&change_target_to_id]() {
        static auto _last_time_target_changed = clock();

        const auto agents_ptr = GW::Agents::GetAgentArray();
        if (!agents_ptr)
            return false;
        auto &agents = *agents_ptr;

        for (const auto *enemy : agents)
        {
            if (!enemy)
                continue;

            const auto player_pos = GetPlayerPos();
            const auto dist_to_enemy = GW::GetDistance(player_pos, enemy->pos);
            if (dist_to_enemy > GW::Constants::Range::Spellcast + 200.0F)
                continue;

            if (!enemy)
                continue;

            const auto enemy_living = enemy->GetAsAgentLiving();
            if (!enemy_living || enemy_living->allegiance != GW::Constants::Allegiance::Enemy)
                continue;

            const auto skill_id = static_cast<GW::Constants::SkillID>(enemy_living->skill);
            if (skill_id == GW::Constants::SkillID::No_Skill)
                continue;

            if (std::find(skills_to_rupt.begin(), skills_to_rupt.end(), skill_id) != skills_to_rupt.end())
            {
                const auto new_target_id = enemy->agent_id;
                const auto target = GW::Agents::GetTarget();
                if (target && target->agent_id != new_target_id && TIMER_DIFF(_last_time_target_changed) > 10)
                {
                    change_target_to_id = new_target_id;
                    _last_time_target_changed = clock();
                }
                return true;
            }
        }

        return false;
    };

    const auto hero_conditions = [&change_target_to_id](const GW::AgentLiving *hero_living) {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist_to_enemy = GW::GetDistance(player_pos, hero_living->pos);
        if (dist_to_enemy > GW::Constants::Range::Spellcast + 200.0F)
            return false;

        if (change_target_to_id)
        {
            GW::GameThread::Enqueue([&, change_target_to_id] { GW::Agents::ChangeTarget(change_target_to_id); });
            change_target_to_id = 0;
        }

        return true;
    };

    for (const auto &[skill_id, skill_class] : skill_class_pairs)
    {
        if (Helper::Hero::HeroUseSkill_Main(skill_id,
                                            skill_class,
                                            "Rupted Skill",
                                            player_conditions,
                                            hero_conditions,
                                            wait_ms,
                                            target_logic))
        {
            if (TIMER_DIFF(last_time_target_changed) > 10)
            {
                GW::GameThread::Enqueue([&, player_target] { GW::Agents::ChangeTarget(player_target); });
                last_time_target_changed = clock();
            }

            return true;
        }
    }

    return false;
}

bool UseSplinterOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Splinter_Weapon;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto player_pos = GetPlayerPos();
        const auto num_enemies_at_player = AgentLivingData::NumAgentsInRange(player_pos,
                                                                             GW::Constants::Allegiance::Enemy,
                                                                             GW::Constants::Range::Nearby);

        const auto player_is_melee_attacking = HoldsMeleeWeapon();
        const auto player_is_melee_class = IsMeleeClass;

        return num_enemies_at_player >= 2 && player_is_melee_attacking && player_is_melee_class;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist = GW::GetDistance(hero_living->pos, player_pos);

        return dist < GW::Constants::Range::Spellcast && hero_living->energy > 0.25F;
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "Splinter",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           false);
}

bool UseVigSpiritOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Vigorous_Spirit;
    constexpr static auto skill_class = GW::Constants::Profession::Monk;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto player_is_melee_attacking = HoldsMeleeWeapon();
        const auto player_is_melee_class = IsMeleeClass;

        return player_is_melee_attacking && player_is_melee_class;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist = GW::GetDistance(hero_living->pos, player_pos);

        return dist < GW::Constants::Range::Spellcast && hero_living->energy > 0.25F;
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "Splinter",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           false);
}

bool UseHonorOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Strength_of_Honor;
    constexpr static auto skill_class = GW::Constants::Profession::Monk;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto player_is_melee_attacking = HoldsMeleeWeapon();
        const auto player_is_melee_class = IsMeleeClass;

        return player_is_melee_attacking && player_is_melee_class;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist = GW::GetDistance(hero_living->pos, player_pos);

        return dist < GW::Constants::Range::Spellcast && hero_living->energy > 0.25F;
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "Honor",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           false);
}

bool UseShelterInFight()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Shelter;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto player_pos = GetPlayerPos();
        const auto num_enemies_in_aggro_of_player =
            AgentLivingData::NumAgentsInRange(player_pos,
                                              GW::Constants::Allegiance::Enemy,
                                              GW::Constants::Range::Spellcast + 200.0F);

        const auto player_started_fight = num_enemies_in_aggro_of_player >= 3 && IsFighting();
        const auto has_skill_already = PlayerHasEffect(GW::Constants::SkillID::Shelter);

        return !has_skill_already && player_started_fight;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist = GW::GetDistance(hero_living->pos, player_pos);

        return dist < GW::Constants::Range::Spirit - 100.0F;
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "Shelter",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           true);
}

bool UseUnionInFight()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Union;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto player_pos = GetPlayerPos();
        const auto num_enemies_in_aggro_of_player =
            AgentLivingData::NumAgentsInRange(player_pos,
                                              GW::Constants::Allegiance::Enemy,
                                              GW::Constants::Range::Spellcast + 200.0F);

        const auto player_started_fight = num_enemies_in_aggro_of_player >= 3 && IsFighting();
        const auto has_skill_already = PlayerHasEffect(GW::Constants::SkillID::Union);

        return !has_skill_already && player_started_fight;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist = GW::GetDistance(hero_living->pos, player_pos);

        return dist < GW::Constants::Range::Spirit - 100.0F;
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "Union",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           true);
}

bool UseSosInFight()
{
    constexpr static auto SOS1_AGENT_ID = uint32_t{4229};
    constexpr static auto SOS2_AGENT_ID = uint32_t{4230};
    constexpr static auto SOS3_AGENT_ID = uint32_t{4231};

    constexpr static auto skill_id = GW::Constants::SkillID::Signet_of_Spirits;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto agents_ptr = GW::Agents::GetAgentArray();
        if (!agents_ptr)
            return false;
        auto &agents = *agents_ptr;

        const auto player_pos = GetPlayerPos();
        const auto num_enemies_in_aggro_of_player = AgentLivingData::NumAgentsInRange(player_pos,
                                                                                      GW::Constants::Allegiance::Enemy,
                                                                                      GW::Constants::Range::Spellcast);

        const auto player_started_fight = num_enemies_in_aggro_of_player >= 3 && IsFighting();

        if (!player_started_fight)
            return false;

        auto spirits_in_range = std::vector<GW::AgentLiving *>{};
        for (auto *enemy : agents)
        {
            if (!enemy)
                continue;

            const auto dist_to_enemy = GW::GetDistance(player_pos, enemy->pos);
            if (dist_to_enemy > GW::Constants::Range::Spellcast + 200.0F)
                continue;

            if (!enemy)
                continue;
            const auto enemy_living = enemy->GetAsAgentLiving();
            if (!enemy_living || enemy_living->allegiance != GW::Constants::Allegiance::Spirit_Pet)
                continue;

            spirits_in_range.push_back(enemy_living);
        }

        const auto sos_spirits_in_range = FoundSpirit(spirits_in_range, SOS1_AGENT_ID) &&
                                          FoundSpirit(spirits_in_range, SOS2_AGENT_ID) &&
                                          FoundSpirit(spirits_in_range, SOS3_AGENT_ID);

        return player_started_fight && !sos_spirits_in_range;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist = GW::GetDistance(hero_living->pos, player_pos);

        return dist < GW::Constants::Range::Spellcast;
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "SoS",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           true);
}

bool UseFallback()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Fall_Back;
    constexpr static auto skill_class = GW::Constants::Profession::Paragon;
    constexpr static auto wait_ms = 250UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = []() { return true; };

    auto hero_conditions = [](const GW::AgentLiving *) {
        return !PlayerOrHeroHasEffect(GW::Constants::SkillID::Fall_Back);
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "FallBack",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           true);
}

bool UseBipOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Blood_is_Power;
    constexpr static auto skill_class = GW::Constants::Profession::Necromancer;
    constexpr static auto wait_ms = 600UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;
    const static auto energy_class_map = std::map<GW::Constants::Profession, std::pair<uint32_t, float>>{
        {GW::Constants::Profession::Warrior, {25U, 0.70F}},
        {GW::Constants::Profession::Ranger, {25U, 0.60F}},
        {GW::Constants::Profession::Monk, {30U, 0.50F}},
        {GW::Constants::Profession::Necromancer, {30U, 0.50F}},
        {GW::Constants::Profession::Mesmer, {30U, 0.50F}},
        {GW::Constants::Profession::Elementalist, {40U, 0.40F}},
        {GW::Constants::Profession::Assassin, {25U, 0.60F}},
        {GW::Constants::Profession::Ritualist, {30U, 0.50F}},
        {GW::Constants::Profession::Paragon, {25U, 0.60F}},
        {GW::Constants::Profession::Dervish, {25U, 0.50F}},
    };

    auto player_conditions = []() {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (me_living->energy_regen > 0.03F) // Dont have bip yet
            return false;

        const auto primary = GetPrimaryClass();
        const auto [enrgy_treshold_abs, enrgy_treshold_perc] = energy_class_map.at(primary);
        const auto [energy, _, energy_perc] = GetEnergyData();
        if (energy_perc > enrgy_treshold_perc && energy > enrgy_treshold_abs)
            return false;

        return true;
    };

    auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist = GW::GetDistance(hero_living->pos, player_pos);
        const auto is_close_enough = dist < GW::Constants::Range::Spellcast + 300.0F;
        const auto hero_has_enough_hp = hero_living->hp > 0.50F;

        return is_close_enough && hero_has_enough_hp;
    };

    return Helper::Hero::HeroUseSkill_Main(skill_id,
                                           skill_class,
                                           "BiP",
                                           player_conditions,
                                           hero_conditions,
                                           wait_ms,
                                           target_logic,
                                           false);
}
} // namespace HeroSmartSkills

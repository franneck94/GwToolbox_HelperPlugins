#pragma once

#include <cstdint>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Packets/StoC.h>

#include "DataSkillbar.h"
#include "Logger.h"

class DataPlayer
{
public:
    DataPlayer(const uint32_t agent_id = UINT32_MAX) noexcept : id(agent_id), pos(GW::GamePos{0.0F, 0.0F, 0}) {};
    ~DataPlayer() {};

    bool ValidateData(std::function<bool(bool)> cb_fn, const bool need_party_loaded) const;
    void Update();

    void ChangeTarget(const uint32_t target_id);
    bool CastEffectIfNotAvailable(const DataSkill &skill_data) const;
    bool CastEffect(const DataSkill &skill_data) const;

    static bool HoldsMeleeWeapon();
    static bool HoldsCasterWeapon();
    static bool IsMeleeClass();
    static bool IsCasterClass();

    static bool CanCast();
    static bool CanAttack();
    static bool IsCasting();
    static bool IsAttacking();
    static bool IsFighting();
    static bool IsMoving();

    static bool HasBuff(const GW::Constants::SkillID buff_skill_id);
    static bool HasEffect(const GW::Constants::SkillID effect_skill_id);
    static float GetRemainingEffectDuration(const GW::Constants::SkillID effect_skill_id);
    static uint32_t GetNumberOfPartyBonds();
    static bool PlayerOrHeroHasEffect(const GW::Constants::SkillID effect_id);
    static bool PlayerHasEffect(const GW::Constants::SkillID effect_id, const bool ignore_id = false);


public:
    uint32_t id = UINT32_MAX;
    const GW::AgentLiving *living = nullptr;

    GW::GamePos pos;

    uint32_t energy = 0U;
    uint32_t max_energy = 0U;
    float energy_perc = 0.0F;

    uint32_t hp = 0U;
    uint32_t max_hp = 0U;
    float hp_perc = 0.0F;
    GW::Constants::Profession primary = GW::Constants::Profession::None;
    GW::Constants::Profession secondary = GW::Constants::Profession::None;
};

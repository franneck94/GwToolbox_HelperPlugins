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
    DataPlayer() noexcept {};
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

    static uint32_t GetMaxEnergy();
    static uint32_t GetEnergy();
    static float GetEnergyPerc();

    static uint32_t GetMaxHp();
    static uint32_t GetHp();
    static float GetHpPerc();

public:
    const GW::AgentLiving *living = nullptr;

    GW::GamePos pos;

    GW::Constants::Profession primary = GW::Constants::Profession::None;
    GW::Constants::Profession secondary = GW::Constants::Profession::None;
};

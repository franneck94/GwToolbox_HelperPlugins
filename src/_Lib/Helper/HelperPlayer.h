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

bool ValidateData(std::function<bool(bool)> cb_fn, const bool need_party_loaded);

void ChangeTarget(const uint32_t target_id);
bool CastEffectIfNotAvailable(const DataSkill &skill_data);
bool CastEffect(const DataSkill &skill_data);

bool HoldsMeleeWeapon();
bool HoldsCasterWeapon();
bool IsMeleeClass();
bool IsCasterClass();

bool CanCast();
bool CanAttack();
bool IsCasting();
bool IsAttacking();
bool IsFighting();
bool IsMoving();
bool CanFight();

bool HasBuff(const GW::Constants::SkillID buff_skill_id);
bool HasEffect(const GW::Constants::SkillID effect_skill_id);
float GetRemainingEffectDuration(const GW::Constants::SkillID effect_skill_id);
uint32_t GetNumberOfPartyBonds();
bool PlayerOrHeroHasEffect(const GW::Constants::SkillID effect_id);
bool PlayerHasEffect(const GW::Constants::SkillID effect_id, const bool ignore_effect_agent_id = false);

uint32_t GetMaxEnergy();
uint32_t GetEnergy();
float GetEnergyPerc();

uint32_t GetMaxHp();
uint32_t GetHp();
float GetHpPerc();

GW::Constants::Profession GetPrimaryClass();
GW::Constants::Profession GetSecondaryClass();

GW::GamePos GetPlayerPos();

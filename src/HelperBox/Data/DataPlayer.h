#pragma once

#include <cstdint>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Packets/StoC.h>

#include <DataSkillbar.h>

class DataPlayer
{
public:
    DataPlayer(const uint32_t agent_id = UINT32_MAX) noexcept : id(agent_id), pos(GW::GamePos{0.0F, 0.0F, 0}){};

    bool ValidateData(std::function<bool(bool)> cb_fn, const bool need_party_loaded) const;
    void Update();

    bool CanCast() const;
    bool CanAttack() const;

    bool HasBuff(const GW::Constants::SkillID buff_skill_id) const;
    bool HasEffect(const GW::Constants::SkillID effect_skill_id) const;
    float GetRemainingEffectDuration(const GW::Constants::SkillID effect_skill_id) const;
    uint32_t GetNumberOfPartyBonds() const;

    bool CastEffectIfNotAvailable(const DataSkill &skill_data);
    bool CastEffect(const DataSkill &skill_data);

    void ChangeTarget(const uint32_t target_id);

    bool SkillStoppedCallback(const GW::Packet::StoC::GenericValue *const packet) const noexcept;


public:
    uint32_t id = UINT32_MAX;

    GW::Agent *me = nullptr;
    GW::AgentLiving *living = nullptr;
    GW::Agent *target = nullptr;
    GW::GamePos pos;

    uint32_t energy = 0U;
    uint32_t max_energy = 0U;
    float energy_perc = 0.0F;

    uint32_t hp = 0U;
    uint32_t max_hp = 0U;
    float hp_perc = 0.0F;

    GW::Constants::Profession primary = GW::Constants::Profession::None;
    GW::Constants::Profession secondary = GW::Constants::Profession::None;

    bool dead = false;
};

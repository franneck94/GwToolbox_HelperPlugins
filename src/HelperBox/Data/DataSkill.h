#pragma once

#include <cstdint>

#include <GWCA/Constants/Skills.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Managers/SkillbarMgr.h>

#include <ActionTypes.h>

struct DataSkill
{
public:
    const GW::Constants::SkillID id;
    uint32_t idx;
    uint8_t energy_cost;
    uint32_t recharge;

    DataSkill(const GW::Constants::SkillID id_, const uint32_t idx_);

    void Update(const GW::SkillbarSkill *const skillbar_skills);

    bool CanBeCasted(const uint32_t current_energy) const noexcept;

    RoutineState Cast(const uint32_t current_energy, const uint32_t target_id = 0) const;

    bool SkillFound() const noexcept;
};

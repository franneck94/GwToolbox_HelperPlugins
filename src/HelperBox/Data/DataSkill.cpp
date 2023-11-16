#include <cstdint>

#include <GWCA/Constants/Skills.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>

#include "DataSkill.h"

DataSkill::DataSkill(const GW::Constants::SkillID id_, const uint32_t idx_)
    : id(id_), idx(idx_), energy_cost(0U), recharge(UINT32_MAX)
{
    const auto *const skill_data = GW::SkillbarMgr::GetSkillConstantData((uint32_t)id);
    if (!skill_data)
        energy_cost = 0U;
    else
        energy_cost = skill_data->GetEnergyCost();
}

void DataSkill::Update(const GW::SkillbarSkill *const skillbar_skills)
{
    if (!skillbar_skills)
        return;
    recharge = skillbar_skills[idx].GetRecharge();
}

bool DataSkill::CanBeCasted(const uint32_t current_energy) const noexcept
{
    return SkillFound() && (current_energy > energy_cost && recharge == 0);
}

RoutineState DataSkill::Cast(const uint32_t current_energy, const uint32_t target_id) const
{
    if (!CanBeCasted(current_energy))
        return RoutineState::ACTIVE;

    if (target_id != 0)
        GW::GameThread::Enqueue([&]() { GW::SkillbarMgr::UseSkill(idx, target_id); });
    else
        GW::GameThread::Enqueue([&]() { GW::SkillbarMgr::UseSkill(idx); });

    return RoutineState::FINISHED;
}

bool DataSkill::SkillFound() const noexcept
{
    return idx != static_cast<uint32_t>(-1);
}

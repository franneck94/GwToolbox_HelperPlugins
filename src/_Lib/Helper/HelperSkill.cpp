#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Managers/SkillbarMgr.h>

#include "HelperSkill.h"

bool SkillIsMeleeAttack(const GW::Constants::SkillID skill_id)
{
    const auto skill_data = GW::SkillbarMgr::GetSkillConstantData(skill_id);
    if (!skill_data)
        return false;

    if (skill_data->type == GW::Constants::SkillType::Attack)
    {
        return true;
    }

    return false;
}

const GW::Skillbar *GetAgentSkillbar(const uint32_t agent_id)
{
    const auto *skillbar_array = GW::SkillbarMgr::GetSkillbarArray();

    if (!skillbar_array)
        return nullptr;

    for (const GW::Skillbar &skillbar : *skillbar_array)
    {
        if (skillbar.agent_id == agent_id)
            return &skillbar;
    }

    return nullptr;
}

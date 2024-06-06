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

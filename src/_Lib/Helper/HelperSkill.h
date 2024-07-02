#pragma once

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/GameEntities/Skill.h>

bool SkillIsMeleeAttack(const GW::Constants::SkillID skill_id);

const GW::Skillbar *GetAgentSkillbar(const uint32_t agent_id);

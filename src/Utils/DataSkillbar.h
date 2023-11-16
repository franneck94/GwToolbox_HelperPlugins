#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include <GWCA/Constants/Skills.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Managers/CtoSMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Packets/Opcodes.h>
#include <GWCA/Packets/StoC.h>
#include <GWCA/Utilities/Hook.h>

#include <DataSkill.h>
#include <HelperCallbacks.h>

template <uint32_t N>
class SkillbarDataABC
{
public:
    SkillbarDataABC()
    {
        GW::StoC::RegisterPacketCallback<GW::Packet::StoC::MapLoaded>(
            &MapLoaded_Entry,
            [this](GW::HookStatus *status, GW::Packet::StoC::MapLoaded *packet) -> void {
                reset = ExplorableLoadCallback(status, packet);
            });
    }

    bool ValidateData()
    {
        const auto skillbar_ = GW::SkillbarMgr::GetPlayerSkillbar();
        if (!skillbar_)
            return false;

        const auto skillbar_skills_ = skillbar_->skills;
        if (!skillbar_skills_)
            return false;

        return true;
    }

    void Load()
    {
        const auto internal_skillbar = GW::SkillbarMgr::GetPlayerSkillbar();
        if (!internal_skillbar)
            return;
        const auto skillbar_skills = internal_skillbar->skills;
        LoadInternal(skillbar_skills);
    }

    void LoadInternal(const GW::SkillbarSkill *skillbar_skills)
    {
        if (!skillbar_skills)
            return;

        for (auto skill : skills)
        {
            if (!skill)
                continue;
            skill->idx = static_cast<uint32_t>(-1);

            for (uint32_t idx = 0; idx < 8U; ++idx)
            {
                if (skillbar_skills[idx].skill_id == (uint32_t)skill->id)
                {
                    skill->idx = idx;
                    break;
                }
            }
        }
    }

    void Update()
    {
        const auto internal_skillbar = GW::SkillbarMgr::GetPlayerSkillbar();
        if (!internal_skillbar)
            return;
        const auto skillbar_skills = internal_skillbar->skills;
        UpdateInternal(skillbar_skills);

        if (reset)
        {
            Load();
            reset = false;
        }
    }

    void UpdateInternal(const GW::SkillbarSkill *skillbar_skills)
    {
        if (!skillbar_skills)
            return;

        for (auto skill : skills)
        {
            if (!skill)
                continue;
            if (skill->SkillFound())
                skill->Update(skillbar_skills);
        }
    }

protected:
    bool reset = false;
    GW::HookEntry MapLoaded_Entry;
    std::array<DataSkill *, N> skills;
};

#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <random>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Hero.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Utilities/Hooker.h>

#include "Helper.h"
#include "HelperAgents.h"
#include "HelperHero.h"
#include "HelperMaps.h"
#include "SmartSkillRupts.h"
#include "Utils.h"

#include <imgui.h>

namespace
{
bool RuptEnemies()
{
    const static auto skill_class_pairs = std::vector<std::tuple<GW::Constants::SkillID, GW::Constants::Profession>>{
        {GW::Constants::SkillID::Cry_of_Frustration, GW::Constants::Profession::Mesmer},
        {GW::Constants::SkillID::Power_Drain, GW::Constants::Profession::Mesmer},
    };
    const static auto skills_to_rupt = std::array{
        // Mesmer
        GW::Constants::SkillID::Panic,
        GW::Constants::SkillID::Energy_Surge,
        // Necro
        GW::Constants::SkillID::Chilblains,
        // Ele
        GW::Constants::SkillID::Meteor,
        GW::Constants::SkillID::Meteor_Shower,
        GW::Constants::SkillID::Searing_Flames,
        // All
        GW::Constants::SkillID::Resurrection_Signet,

    };
    const static auto rupt_names_map = std::map<GW::Constants::SkillID, const char *>{
        {GW::Constants::SkillID::No_Skill, "Unk"},
        {GW::Constants::SkillID::Panic, "Panic"},
        {GW::Constants::SkillID::Energy_Surge, "Energy Surge"},
        {GW::Constants::SkillID::Chilblains, "Chilblains"},
        {GW::Constants::SkillID::Meteor, "Meteor"},
        {GW::Constants::SkillID::Meteor_Shower, "Meteor Shower"},
        {GW::Constants::SkillID::Searing_Flames, "Searing Flames"},
        {GW::Constants::SkillID::Resurrection_Signet, "Resurrection Signet"},
    };
    constexpr static auto wait_ms = 200UL;
    constexpr static auto target_logic = Helper::Hero::TargetLogic::SEARCH_TARGET;
    constexpr static auto ignore_effect_agent_id = false;
    constexpr static auto check_for_effect = false;
    static auto last_time_target_changed = clock();

    const auto target = GW::Agents::GetTarget();
    auto player_target = target ? target->agent_id : 0;
    auto change_target_to_id = 0U;

    auto player_conditions = [&change_target_to_id]() {
        static auto _last_time_target_changed = clock();

        const auto agents_ptr = GW::Agents::GetAgentArray();
        if (!agents_ptr)
            return false;
        auto &agents = *agents_ptr;

        for (const auto *enemy : agents)
        {
            if (!enemy)
                continue;

            const auto player_pos = GetPlayerPos();
            const auto dist_to_enemy = GW::GetDistance(player_pos, enemy->pos);
            if (dist_to_enemy > GW::Constants::Range::Spellcast + 200.0F)
                continue;

            if (!enemy)
                continue;

            const auto enemy_living = enemy->GetAsAgentLiving();
            if (!enemy_living || enemy_living->allegiance != GW::Constants::Allegiance::Enemy)
                continue;

            const auto skill_id = static_cast<GW::Constants::SkillID>(enemy_living->skill);
            if (skill_id == GW::Constants::SkillID::No_Skill)
                continue;

            const auto rupt_it = std::find(skills_to_rupt.begin(), skills_to_rupt.end(), skill_id);
            if (rupt_it != skills_to_rupt.end())
            {
                const auto new_target_id = enemy->agent_id;
                const auto target = GW::Agents::GetTarget();
                if (target && target->agent_id != new_target_id && TIMER_DIFF(_last_time_target_changed) > 10)
                {
                    change_target_to_id = new_target_id;
                    _last_time_target_changed = clock();
                }
                Log::Info("Found skill to rupt: %s", rupt_names_map.at(*rupt_it));
                return true;
            }
        }

        return false;
    };

    const auto hero_conditions = [&change_target_to_id](const GW::AgentLiving *hero_living) {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto dist_to_enemy = GW::GetDistance(player_pos, hero_living->pos);
        if (dist_to_enemy > GW::Constants::Range::Spellcast + 200.0F)
            return false;

        if (change_target_to_id)
        {
            GW::GameThread::Enqueue([&, change_target_to_id] { GW::Agents::ChangeTarget(change_target_to_id); });
            change_target_to_id = 0;
        }

        return true;
    };

    for (const auto &[skill_id, skill_class] : skill_class_pairs)
    {
        if (Helper::Hero::HeroUseSkill_Main(skill_id,
                                            skill_class,
                                            "Rupted Skill",
                                            player_conditions,
                                            hero_conditions,
                                            wait_ms,
                                            target_logic,
                                            ignore_effect_agent_id,
                                            check_for_effect))
        {
            if (TIMER_DIFF(last_time_target_changed) > 10)
            {
                GW::GameThread::Enqueue([&, player_target] { GW::Agents::ChangeTarget(player_target); });
                last_time_target_changed = clock();
            }

            return true;
        }
    }

    return false;
}

} // namespace

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static SmartSkillRupts instance;
    return &instance;
}

void SmartSkillRupts::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

bool SmartSkillRupts::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void SmartSkillRupts::Terminate()
{
    ToolboxPlugin::Terminate();
}

void SmartSkillRupts::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);

    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"SmartSkillRupts");
}

void SmartSkillRupts::Update(float)
{
    if (!ValidateData(HelperActivationConditions, true))
        return;

    HeroSmarterSkills_Main();
}

bool SmartSkillRupts::HeroSmarterSkills_Main()
{
    if (!IsMapReady() || !IsExplorable())
        return false;

    if (RuptEnemies())
        return true;

    return false;
}

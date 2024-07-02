#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <random>
#include <string_view>

#include <GWCA/Constants/Constants.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/EffectMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Utilities/Hooker.h>

#include "DataLivings.h"
#include "Helper.h"
#include "HelperHero.h"
#include "HelperMaps.h"
#include "SmartHexAndCondRemove.h"
#include "Utils.h"

#include <imgui.h>

namespace
{
bool RemoveImportantConditions()
{
    constexpr static auto to_remove_conditions_melee = std::array{
        GW::Constants::SkillID::Blind,
        GW::Constants::SkillID::Weakness,
    };
    constexpr static auto to_remove_conditions_caster = std::array{
        GW::Constants::SkillID::Dazed,
    };
    constexpr static auto to_remove_conditions_all = std::array{
        GW::Constants::SkillID::Crippled,
    };
    const static auto cond_names_map = std::map<GW::Constants::SkillID, const char*>{
        {GW::Constants::SkillID::No_Skill, "Unk"},
        {GW::Constants::SkillID::Blind, "Blind"},
        {GW::Constants::SkillID::Weakness, "Weakness"},
        {GW::Constants::SkillID::Dazed, "Dazed"},
        {GW::Constants::SkillID::Crippled, "Crippled"},
    };
    const static auto skill_class_pairs = std::vector<std::tuple<GW::Constants::SkillID, GW::Constants::Profession>>{
        {GW::Constants::SkillID::Mend_Body_and_Soul, GW::Constants::Profession::Ritualist},
        {GW::Constants::SkillID::Dismiss_Condition, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Mend_Condition, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Smite_Condition, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Purge_Conditions, GW::Constants::Profession::Monk},
    };
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = Helper::Hero::TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (!me_living->GetIsConditioned())
            return false;

        const auto *const effects = GW::Effects::GetPlayerEffects();
        if (!effects || !effects->valid())
            return false;

        auto cond_lookup = [](const auto &cond_array, const GW::Effect &curr_cond) {
            const auto cond_it = std::find(cond_array.begin(), cond_array.end(), curr_cond.skill_id);
            const auto found_cond = cond_it != cond_array.end();
            const auto cond = found_cond ? *cond_it : GW::Constants::SkillID::No_Skill;
            return std::make_pair(cond, found_cond);
        };

        for (const auto &effect : *effects)
        {
            if (effect.agent_id != 0)
                continue;

            if (HoldsMeleeWeapon())
            {
                const auto [cond, found_cond] = cond_lookup(to_remove_conditions_melee, effect);
                if (found_cond)
                {
                    Log::Info("Found condition to remove: %s", cond_names_map.at(cond));
                    return true;
                }
            }
            else
            {
                const auto [cond, found_cond] = cond_lookup(to_remove_conditions_caster, effect);
                if (found_cond)
                {
                    Log::Info("Found condition to remove: %s", cond_names_map.at(cond));
                    return true;
                }
            }

            const auto [cond, found_cond] = cond_lookup(to_remove_conditions_all, effect);
            if (found_cond)
            {
                Log::Info("Found condition to remove: %s", cond_names_map.at(cond));
                return true;
            }
        }

        return false;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (!hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto distance = GW::GetDistance(player_pos, hero_living->pos);
        return distance <= GW::Constants::Range::Spellcast;
    };

    for (const auto &[skill_id, skill_class] : skill_class_pairs)
    {
        if (Helper::Hero::HeroUseSkill_Main(skill_id,
                                            skill_class,
                                            "Remove Cond",
                                            player_conditions,
                                            hero_conditions,
                                            wait_ms,
                                            target_logic,
                                            false))
            return true;
    }

    return false;
}

bool ShatterImportantHexes()
{
    constexpr static auto to_remove_hexes_melee = std::array{
        // Mesmer
        GW::Constants::SkillID::Ineptitude,
        GW::Constants::SkillID::Empathy,
        GW::Constants::SkillID::Crippling_Anguish,
        GW::Constants::SkillID::Clumsiness,
        GW::Constants::SkillID::Faintheartedness,
        // Ele
        GW::Constants::SkillID::Blurred_Vision,
    };
    constexpr static auto to_remove_hexes_caster = std::array{
        // Mesmer
        GW::Constants::SkillID::Panic,
        GW::Constants::SkillID::Backfire,
        GW::Constants::SkillID::Mistrust,
        GW::Constants::SkillID::Power_Leech,
        GW::Constants::SkillID::Soul_Leech,
    };
    constexpr static auto to_remove_hexes_all = std::array{
        // Mesmer
        GW::Constants::SkillID::Diversion,
        GW::Constants::SkillID::Visions_of_Regret,
        // Ele
        GW::Constants::SkillID::Deep_Freeze,
        GW::Constants::SkillID::Mind_Freeze,
        // Necro
        GW::Constants::SkillID::Spiteful_Spirit,
    };
    constexpr static auto to_remove_hexes_paragon = std::array{
        // Necro
        GW::Constants::SkillID::Vocal_Minority,
    };
    const static auto hex_names_map = std::map<GW::Constants::SkillID, const char*>{
        {GW::Constants::SkillID::No_Skill, "Unk"},
        {GW::Constants::SkillID::Ineptitude, "Ineptitude"},
        {GW::Constants::SkillID::Empathy, "Empathy"},
        {GW::Constants::SkillID::Crippling_Anguish, "Crippling Anguish"},
        {GW::Constants::SkillID::Clumsiness, "Clumsiness"},
        {GW::Constants::SkillID::Faintheartedness, "Faintheartedness"},
        {GW::Constants::SkillID::Blurred_Vision, "Blurred Vision"},
        {GW::Constants::SkillID::Panic, "Panic"},
        {GW::Constants::SkillID::Backfire, "Backfire"},
        {GW::Constants::SkillID::Mistrust, "Mistrust"},
        {GW::Constants::SkillID::Power_Leech, "Power Leech"},
        {GW::Constants::SkillID::Soul_Leech, "Soul Leech"},
        {GW::Constants::SkillID::Diversion, "Diversion"},
        {GW::Constants::SkillID::Visions_of_Regret, "Visions of Regret"},
        {GW::Constants::SkillID::Deep_Freeze, "Deep Freeze"},
        {GW::Constants::SkillID::Mind_Freeze, "Mind Freeze"},
        {GW::Constants::SkillID::Spiteful_Spirit, "Spiteful Spirit"},
        {GW::Constants::SkillID::Vocal_Minority, "Vocal Minority"},
    };
    const static auto skill_class_pairs = std::vector<std::tuple<GW::Constants::SkillID, GW::Constants::Profession>>{
        {GW::Constants::SkillID::Shatter_Hex, GW::Constants::Profession::Mesmer},
        {GW::Constants::SkillID::Remove_Hex, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Smite_Hex, GW::Constants::Profession::Monk},
    };
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = Helper::Hero::TargetLogic::NO_TARGET;

    auto player_conditions = []() {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (!me_living->GetIsHexed())
            return false;

        const auto *const effects = GW::Effects::GetPlayerEffects();
        if (!effects || !effects->valid())
            return false;

        auto hex_lookup = [](const auto &hex_array, const GW::Effect &curr_hex) {
            const auto hex_it = std::find(hex_array.begin(), hex_array.end(), curr_hex.skill_id);
            const auto found_hex = hex_it != hex_array.end();
            const auto hex = found_hex ? *hex_it : GW::Constants::SkillID::No_Skill;
            return std::make_pair(hex, found_hex);
        };
        for (const auto &effect : *effects)
        {
            if (effect.agent_id != 0)
                continue;

            if (HoldsMeleeWeapon())
            {
                const auto [hex, found_hex] = hex_lookup(to_remove_hexes_melee, effect);
                if (found_hex)
                {
                    Log::Info("Found hex to remove: %s", hex_names_map.at(hex));
                    return true;
                }
            }

            if (!HoldsMeleeWeapon())
            {
                const auto [hex, found_hex] = hex_lookup(to_remove_hexes_caster, effect);
                if (found_hex)
                {
                    Log::Info("Found hex to remove: %s", hex_names_map.at(hex));
                    return true;
                }
            }

            const auto primary = GetPrimaryClass();
            if (primary == GW::Constants::Profession::Paragon)
            {
                const auto [hex, found_hex] = hex_lookup(to_remove_hexes_paragon, effect);
                if (found_hex)
                {
                    Log::Info("Found hex to remove: %s", hex_names_map.at(hex));
                    return true;
                }
            }

            const auto [hex, found_hex] = hex_lookup(to_remove_hexes_all, effect);
            if (found_hex)
            {
                Log::Info("Found hex to remove: %s", hex_names_map.at(hex));
                return true;
            }
        }

        return false;
    };

    const auto hero_conditions = [](const GW::AgentLiving *hero_living) {
        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return false;

        if (!me_living || !hero_living)
            return false;

        const auto player_pos = GetPlayerPos();
        const auto distance = GW::GetDistance(player_pos, hero_living->pos);
        return distance <= GW::Constants::Range::Spellcast;
    };

    auto casted_skill = false;
    for (const auto &[skill_id, skill_class] : skill_class_pairs)
    {
        if (Helper::Hero::HeroUseSkill_Main(skill_id,
                                            skill_class,
                                            "Remove Hex",
                                            player_conditions,
                                            hero_conditions,
                                            wait_ms,
                                            target_logic,
                                            false))
            casted_skill = true;
    }

    return casted_skill;
}
} // namespace

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static SmartHexAndCondRemove instance;
    return &instance;
}

void SmartHexAndCondRemove::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

bool SmartHexAndCondRemove::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void SmartHexAndCondRemove::Terminate()
{
    ToolboxPlugin::Terminate();
}

void SmartHexAndCondRemove::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);

    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"Smart ST");
}

void SmartHexAndCondRemove::Update(float)
{
    if (!ValidateData(HelperActivationConditions, true))
        return;

    HeroSmarterSkills_Main();
}

bool SmartHexAndCondRemove::HeroSmarterSkills_Main()
{
    if (!IsMapReady() || !IsExplorable())
        return false;

    RemoveImportantConditions();
    ShatterImportantHexes();

    return false;
}

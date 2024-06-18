#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <tuple>
#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Context/GameContext.h>
#include <GWCA/Context/WorldContext.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Hero.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Utilities/Hooker.h>
#include <GWCA/Utilities/Scanner.h>

#include "ActionTypes.h"
#include "ActionsBase.h"
#include "DataPlayer.h"
#include "Helper.h"
#include "HelperAgents.h"
#include "HelperHero.h"
#include "HelperMaps.h"
#include "HelperSkill.h"
#include "HeroWindow.h"
#include "Logger.h"
#include "Utils.h"

#include <imgui.h>

namespace
{
constexpr auto IM_COLOR_RED = ImVec4(1.0F, 0.1F, 0.1F, 1.0F);
constexpr auto IM_COLOR_GREEN = ImVec4(0.1F, 0.9F, 0.1F, 1.0F);
constexpr auto IM_COLOR_BLUE = ImVec4(0.1F, 0.1F, 1.0F, 1.0F);

void OnTargetPing(GW::HookStatus *, GW::UI::UIMessage, void *wparam, void *)
{
    auto *instance = static_cast<HeroWindow *>(ToolboxPluginInstance());
    if (!instance)
        return;

    const auto packet = static_cast<GW::UI::UIPacket::kSendCallTarget *>(wparam);
    if (!packet || (packet->call_type != GW::CallTargetType::AttackingOrTargetting))
        return;

    const auto *ping_agent = GW::Agents::GetAgentByID(packet->agent_id);
    if (!ping_agent)
        return;

    Log::Info("Called OnTargetPing");

    const auto ping_distance = GW::GetDistance(ping_agent->pos, instance->player_data.pos);
    const auto ping_close = ping_distance < GW::Constants::Range::Spellcast + 200.0F;
    const auto ping_far = ping_distance > GW::Constants::Range::Spellcast + 200.0F;
    if (ping_close)
        instance->StopFollowing();
    else if (ping_far)
        instance->following_active = true;
}

void OnMapLoad(GW::HookStatus *, const GW::Packet::StoC::MapLoaded *)
{
    auto *instance = static_cast<HeroWindow *>(ToolboxPluginInstance());

    instance->ResetData();
}
} // namespace

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static HeroWindow instance;
    return &instance;
}

void HeroWindow::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);
    if (!GW::Initialize())
    {
        GW::Terminate();
        return;
    }

    GW::UI::RegisterUIMessageCallback(&AgentPinged_Entry, GW::UI::UIMessage::kSendCallTarget, OnTargetPing);

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::MapLoaded>(&MapLoaded_Entry, OnMapLoad);

    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"HeroWindow");
}

void HeroWindow::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

bool HeroWindow::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void HeroWindow::Terminate()
{
    ToolboxPlugin::Terminate();
    GW::StoC::RemoveCallbacks(&MapLoaded_Entry);
    GW::UI::RemoveUIMessageCallback(&AgentPinged_Entry);
}

void HeroWindow::StartFollowing()
{
    Log::Info("Heroes will follow the player!");
    following_active = true;
    current_hero_behaviour_before_follow = current_hero_behaviour;
    current_hero_behaviour = GW::HeroBehavior::AvoidCombat;
}

void HeroWindow::StopFollowing()
{
    Log::Info("Heroes stopped following the player!");
    following_active = false;
    GW::PartyMgr::UnflagAll();
    current_hero_behaviour = current_hero_behaviour_before_follow;
}

void HeroWindow::ToggleHeroBehaviour()
{
    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (!party_info || !player_data.living || !IsMapReady())
        return;

    Log::Info("Toggle hero hehaviour!");

    if (current_hero_behaviour == GW::HeroBehavior::Guard)
        current_hero_behaviour = GW::HeroBehavior::AvoidCombat;
    else if (current_hero_behaviour == GW::HeroBehavior::Fight)
        current_hero_behaviour = GW::HeroBehavior::Guard;
    else if (current_hero_behaviour == GW::HeroBehavior::AvoidCombat)
        current_hero_behaviour = GW::HeroBehavior::Fight;
    else
        return;

    for (const auto &hero : party_info->heroes)
    {
        if (hero.owner_player_id == player_data.living->login_number)
            GW::PartyMgr::SetHeroBehavior(hero.agent_id, current_hero_behaviour);
    }
}

void HeroWindow::FollowPlayer()
{
    if (!IsMapReady() || !IsExplorable() || !player_data.living || (follow_pos.x == 0.0F && follow_pos.y == 0.0F) ||
        !GW::PartyMgr::GetIsPartyLoaded() || GW::PartyMgr::GetIsPartyDefeated())
        return;

    GW::PartyMgr::FlagAll(follow_pos);
}

bool HeroWindow::HeroSkill_StartConditions(const GW::Constants::SkillID skill_id, const long wait_ms)
{
    if (!IsMapReady() || !IsExplorable() || hero_data.hero_vec.size() == 0)
        return false;

    if (!ActionABC::HasWaitedLongEnough(wait_ms))
        return false;

    if (skill_id != GW::Constants::SkillID::No_Skill && player_data.PlayerHasEffect(skill_id, true))
        return false;

    return true;
}

bool HeroWindow::SmartUseSkill(const GW::Constants::SkillID skill_id,
                               const GW::Constants::Profession skill_class,
                               const std::string_view skill_name,
                               std::function<bool(const DataPlayer &, const AgentLivingData &)> player_conditions,
                               std::function<bool(const DataPlayer &, const Hero &)> hero_conditions,
                               const long wait_ms,
                               const TargetLogic target_logic,
                               const uint32_t current_target_id)
{
    if (!player_conditions(player_data, livings_data))
        return false;

    if (!HeroSkill_StartConditions(skill_id, wait_ms))
        return false;

    if (hero_data.hero_class_idx_map.find(skill_class) == hero_data.hero_class_idx_map.end())
        return false;

    auto hero_idxs_zero_based = hero_data.hero_class_idx_map.at(skill_class);
    if (hero_idxs_zero_based.size() == 0)
        return false;

    for (const auto hero_idx_zero_based : hero_idxs_zero_based)
    {
        const auto &hero = hero_data.hero_vec.at(hero_idx_zero_based);

        if (HeroCastSkillIfAvailable(hero, player_data, skill_id, hero_conditions, target_logic, current_target_id))
        {
#ifdef _DEBUG
            Log::Info("Casted %s.", skill_name);
#else
            (void)skill_name;
#endif
            return true;
        }
    }

    return true;
}

void HeroWindow::ShatterImportantHexes()
{
    constexpr static auto to_remove_hexes_melee = std::array{
        // Mesmer
        GW::Constants::SkillID::Ineptitude,
        GW::Constants::SkillID::Empathy,
        GW::Constants::SkillID::Crippling_Anguish,
        // Necro
        GW::Constants::SkillID::Spiteful_Spirit,
        // Ele
        GW::Constants::SkillID::Blurred_Vision,
    };
    constexpr static auto to_remove_hexes_caster = std::array{
        // Mesmer
        GW::Constants::SkillID::Panic,
        GW::Constants::SkillID::Backfire,
        GW::Constants::SkillID::Mistrust,
        GW::Constants::SkillID::Power_Leech,
        // Necro
        GW::Constants::SkillID::Spiteful_Spirit,
        GW::Constants::SkillID::Soul_Leech,
    };
    constexpr static auto to_remove_hexes_all = std::array{
        // Mesmer
        GW::Constants::SkillID::Diversion,
        GW::Constants::SkillID::Visions_of_Regret,
        // Ele
        GW::Constants::SkillID::Deep_Freeze,
        GW::Constants::SkillID::Mind_Freeze,
    };
    constexpr static auto to_remove_hexes_paragon = std::array{
        // Necro
        GW::Constants::SkillID::Vocal_Minority,
    };

    const static auto skill_class_pairs = std::vector<std::tuple<GW::Constants::SkillID, GW::Constants::Profession>>{
        {GW::Constants::SkillID::Shatter_Hex, GW::Constants::Profession::Mesmer},
        {GW::Constants::SkillID::Remove_Hex, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Smite_Hex, GW::Constants::Profession::Monk},
    };
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &) {
        if (!player_data.living || !player_data.living->GetIsHexed())
            return false;

        const auto *const effects = GW::Effects::GetPlayerEffects();
        if (!effects || !effects->valid())
            return false;

        auto found_hex = [](const auto &hex_array, const GW::Effect &curr_hex) {
            return std::find(hex_array.begin(), hex_array.end(), curr_hex.skill_id) != hex_array.end();
        };

        for (const auto effect : *effects)
        {
            if (effect.agent_id != 0)
                continue;

            if (player_data.holds_melee_weapon)
            {
                if (found_hex(to_remove_hexes_melee, effect))
                    return true;
            }
            else
            {
                if (found_hex(to_remove_hexes_caster, effect))
                    return true;
            }

            if (found_hex(to_remove_hexes_all, effect))
                return true;

            if (player_data.primary == GW::Constants::Profession::Paragon && found_hex(to_remove_hexes_paragon, effect))
                return true;
        }

        return false;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!player_data.living || !hero.hero_living)
            return false;

        const auto spellcast_range = GW::Constants::Range::Spellcast;
        const auto distance = GW::GetDistance(player_data.living->pos, hero.hero_living->pos);
        return distance <= spellcast_range;
    };

    for (const auto &[skill_id, skill_class] : skill_class_pairs)
    {
        if (SmartUseSkill(skill_id,
                          skill_class,
                          "Remove Hex",
                          player_conditions,
                          hero_conditions,
                          wait_ms,
                          target_logic))
            return;
    }
}

void HeroWindow::RemoveImportantConditions()
{
    constexpr static auto to_remove_conditions_melee = std::array{
        GW::Constants::SkillID::Blind,
    };
    constexpr static auto to_remove_conditions_caster = std::array{
        GW::Constants::SkillID::Dazed,
    };
    constexpr static auto to_remove_conditions_all = std::array{
        GW::Constants::SkillID::Crippled,
    };

    const static auto skill_class_pairs = std::vector<std::tuple<GW::Constants::SkillID, GW::Constants::Profession>>{
        {GW::Constants::SkillID::Mend_Body_and_Soul, GW::Constants::Profession::Ritualist},
        {GW::Constants::SkillID::Dismiss_Condition, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Mend_Condition, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Smite_Condition, GW::Constants::Profession::Monk},
        {GW::Constants::SkillID::Purge_Conditions, GW::Constants::Profession::Monk},
    };
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &) {
        if (!player_data.living || !player_data.living->GetIsHexed())
            return false;

        const auto *const effects = GW::Effects::GetPlayerEffects();
        if (!effects || !effects->valid())
            return false;

        auto found_cond = [](const auto &cond_array, const GW::Effect &curr_cond) {
            return std::find(cond_array.begin(), cond_array.end(), curr_cond.skill_id) != cond_array.end();
        };

        for (const auto effect : *effects)
        {
            if (effect.agent_id != 0)
                continue;

            if (player_data.holds_melee_weapon)
            {
                if (found_cond(to_remove_conditions_melee, effect))
                    return true;
            }
            else
            {
                if (found_cond(to_remove_conditions_caster, effect))
                    return true;
            }

            if (found_cond(to_remove_conditions_all, effect))
                return true;
        }

        return false;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!player_data.living || !hero.hero_living)
            return false;

        const auto spellcast_range = GW::Constants::Range::Spellcast;
        const auto distance = GW::GetDistance(player_data.living->pos, hero.hero_living->pos);
        return distance <= spellcast_range;
    };

    for (const auto &[skill_id, skill_class] : skill_class_pairs)
    {
        SmartUseSkill(skill_id, skill_class, "Remove Cond", player_conditions, hero_conditions, wait_ms, target_logic);
    }
}

void HeroWindow::RuptEnemies()
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
    constexpr static auto wait_ms = 200UL;
    constexpr static auto target_logic = TargetLogic::SEARCH_TARGET;
    static auto last_time_target_changed = clock();

    auto player_target = player_data.target ? player_data.target->agent_id : 0;
    auto change_target_to_id = 0U;

    auto player_conditions = [&change_target_to_id](const DataPlayer &player_data, const AgentLivingData &) {
        static auto _last_time_target_changed = clock();

        const auto agents_ptr = GW::Agents::GetAgentArray();
        if (!agents_ptr)
            return false;
        auto &agents = *agents_ptr;

        for (const auto *enemy : agents)
        {
            if (!enemy)
                continue;

            const auto dist_to_enemy = GW::GetDistance(player_data.pos, enemy->pos);
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

            if (std::find(skills_to_rupt.begin(), skills_to_rupt.end(), skill_id) != skills_to_rupt.end())
            {
                const auto new_target_id = enemy->agent_id;
#ifdef _DEBUG
                Log::Info("Changed target to %d casting %d", new_target_id, skill_id);
#endif
                if (player_data.target && player_data.target->agent_id != new_target_id &&
                    TIMER_DIFF(_last_time_target_changed) > 10)
                {
                    change_target_to_id = new_target_id;
                    _last_time_target_changed = clock();
                }
                return true;
            }
        }

        return false;
    };

    const auto hero_conditions = [&change_target_to_id](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living || !player_data.living)
            return false;

        const auto dist_to_enemy = GW::GetDistance(player_data.living->pos, hero.hero_living->pos);
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
        if (SmartUseSkill(skill_id,
                          skill_class,
                          "Rupted Skill",
                          player_conditions,
                          hero_conditions,
                          wait_ms,
                          target_logic))
        {
            if (TIMER_DIFF(last_time_target_changed) > 10)
            {
                GW::GameThread::Enqueue([&, player_target] { GW::Agents::ChangeTarget(player_target); });
                last_time_target_changed = clock();
            }
        }
    }
}

void HeroWindow::UseSplinterOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Splinter_Weapon;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &livings_data) {
        const auto agents_ptr = GW::Agents::GetAgentArray();
        if (!agents_ptr)
            return false;
        auto &agents = *agents_ptr;

        const auto num_enemies_at_player = AgentLivingData::NumAgentsInRange(agents,
                                                                             player_data.pos,
                                                                             GW::Constants::Allegiance::Enemy,
                                                                             GW::Constants::Range::Nearby);

        const auto player_is_melee_attacking = player_data.holds_melee_weapon;
        const auto player_is_melee_class = player_data.is_melee_class;

        return num_enemies_at_player >= 2 && player_is_melee_attacking && player_is_melee_class;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast && hero.hero_living->energy > 0.25F;
    };

    SmartUseSkill(skill_id, skill_class, "Splinter", player_conditions, hero_conditions, wait_ms, target_logic);
}

void HeroWindow::UseHonorOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Strength_of_Honor;
    constexpr static auto skill_class = GW::Constants::Profession::Monk;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &) {
        const auto player_is_melee_attacking = player_data.holds_melee_weapon;
        const auto player_is_melee_class = player_data.is_melee_class;

        return player_is_melee_attacking && player_is_melee_class;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast && hero.hero_living->energy > 0.25F;
    };

    SmartUseSkill(skill_id, skill_class, "Honor", player_conditions, hero_conditions, wait_ms, target_logic);
}

void HeroWindow::UseShelterInFight()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Shelter;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &) {
        const auto agents_ptr = GW::Agents::GetAgentArray();
        if (!agents_ptr)
            return false;
        auto &agents = *agents_ptr;

        const auto num_enemies_in_aggro_of_player =
            AgentLivingData::NumAgentsInRange(agents,
                                              player_data.pos,
                                              GW::Constants::Allegiance::Enemy,
                                              GW::Constants::Range::Spellcast + 200.0F);

        const auto player_started_fight = num_enemies_in_aggro_of_player >= 3 && player_data.IsFighting();
        const auto has_skill_already = player_data.PlayerHasEffect(GW::Constants::SkillID::Shelter);

        return !has_skill_already && player_started_fight;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spirit - 100.0F;
    };

    SmartUseSkill(skill_id, skill_class, "Shelter", player_conditions, hero_conditions, wait_ms, target_logic);
}

void HeroWindow::UseUnionInFight()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Union;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &) {
        const auto agents_ptr = GW::Agents::GetAgentArray();
        if (!agents_ptr)
            return false;
        auto &agents = *agents_ptr;

        const auto num_enemies_in_aggro_of_player =
            AgentLivingData::NumAgentsInRange(agents,
                                              player_data.pos,
                                              GW::Constants::Allegiance::Enemy,
                                              GW::Constants::Range::Spellcast + 200.0F);

        const auto player_started_fight = num_enemies_in_aggro_of_player >= 3 && player_data.IsFighting();
        const auto has_skill_already = player_data.PlayerHasEffect(GW::Constants::SkillID::Union);

        return !has_skill_already && player_started_fight;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spirit - 100.0F;
    };

    SmartUseSkill(skill_id, skill_class, "Union", player_conditions, hero_conditions, wait_ms, target_logic);
}

void HeroWindow::UseSosInFight()
{
    constexpr static auto SOS1_AGENT_ID = uint32_t{4229};
    constexpr static auto SOS2_AGENT_ID = uint32_t{4230};
    constexpr static auto SOS3_AGENT_ID = uint32_t{4231};

    constexpr static auto skill_id = GW::Constants::SkillID::Signet_of_Spirits;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &livings_data) {
        const auto agents_ptr = GW::Agents::GetAgentArray();
        if (!agents_ptr)
            return false;
        auto &agents = *agents_ptr;

        const auto num_enemies_in_aggro_of_player = AgentLivingData::NumAgentsInRange(agents,
                                                                                      player_data.pos,
                                                                                      GW::Constants::Allegiance::Enemy,
                                                                                      GW::Constants::Range::Spellcast);

        const auto player_started_fight = num_enemies_in_aggro_of_player >= 3 && player_data.IsFighting();

        if (!player_started_fight)
            return false;

        const auto sos_spirits_in_range = FoundSpirit(player_data, livings_data.spirits, SOS1_AGENT_ID) &&
                                          FoundSpirit(player_data, livings_data.spirits, SOS2_AGENT_ID) &&
                                          FoundSpirit(player_data, livings_data.spirits, SOS3_AGENT_ID);

        return player_started_fight && !sos_spirits_in_range;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast;
    };

    SmartUseSkill(skill_id, skill_class, "SoS", player_conditions, hero_conditions, wait_ms, target_logic);
}

void HeroWindow::UseFallback()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Fall_Back;
    constexpr static auto skill_class = GW::Constants::Profession::Paragon;
    constexpr static auto wait_ms = 250UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &, const AgentLivingData &) { return true; };

    auto hero_conditions = [](const DataPlayer &player_data, const Hero &) {
        return !player_data.AnyTeamMemberHasEffect(GW::Constants::SkillID::Fall_Back);
    };

    SmartUseSkill(skill_id, skill_class, "FallBack", player_conditions, hero_conditions, wait_ms, target_logic);
}

void HeroWindow::UseBipOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Blood_is_Power;
    constexpr static auto skill_class = GW::Constants::Profession::Necromancer;
    constexpr static auto wait_ms = 500UL;
    constexpr static auto target_logic = TargetLogic::NO_TARGET;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &) {
        if (!player_data.living)
            return false;

        if (player_data.energy_perc > 0.30F && player_data.energy > 15)
            return false;

        if (player_data.living->energy_regen > 0.03F)
            return false;

        return true;
    };

    auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast && hero.hero_living->hp > 0.60F;
    };

    SmartUseSkill(skill_id, skill_class, "BiP", player_conditions, hero_conditions, wait_ms, target_logic);
}

bool HeroWindow::MesmerSpikeTarget(const Hero &hero) const
{
    constexpr static auto skill_id = GW::Constants::SkillID::Energy_Surge;
    constexpr static auto skill_class = GW::Constants::Profession::Mesmer;

    if (!hero.hero_living)
        return false;

    auto hero_conditions = [](const DataPlayer &, const Hero &) { return true; };

    if (hero.hero_living->primary == static_cast<uint8_t>(skill_class))
        return HeroCastSkillIfAvailable(hero, player_data, skill_id, hero_conditions, TargetLogic::PLAYER_TARGET);

    return false;
}

bool player_conditions_attack(const DataPlayer &player_data, const AgentLivingData &)
{
    if (!player_data.target)
        return false;

    const auto *target_living = player_data.target->GetAsAgentLiving();
    if (!target_living)
        return false;

    if (target_living->allegiance != GW::Constants::Allegiance::Enemy)
        return false;

    return true;
}

void HeroWindow::AttackTarget()
{
    if (!player_conditions_attack(player_data, livings_data))
        return;

    if (!HeroSkill_StartConditions(GW::Constants::SkillID::No_Skill))
        return;

    Log::Info("Mesmer Heroes will attack the players target!");

    for (const auto &hero : hero_data.hero_vec)
    {
        MesmerSpikeTarget(hero);
    }
}

void HeroWindow::ResetData()
{
    target_agent_id = 0U;
    follow_pos = GW::GamePos{};
    following_active = false;
    hero_data.hero_vec.clear();
}

void HeroWindow::HeroBehaviour_DrawAndLogic(const ImVec2 &im_button_size)
{
    switch (current_hero_behaviour)
    {
    case GW::HeroBehavior::Guard:
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COLOR_BLUE);
        break;
    }
    case GW::HeroBehavior::AvoidCombat:
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COLOR_GREEN);
        break;
    }
    case GW::HeroBehavior::Fight:
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COLOR_RED);
        break;
    }
    }

    if (ImGui::Button("Behaviour###toggleState", im_button_size))
        ToggleHeroBehaviour();

    ImGui::PopStyleColor();
}

void HeroWindow::HeroFollow_DrawAndLogic(const ImVec2 &im_button_size, bool &toggled_follow)
{
    static auto last_follow_trigger_ms = clock();
    static auto gen = std::mt19937{};
    static auto time_dist = std::uniform_int_distribution<long>(-10, 10);

    auto added_color_follow = false;

    if (following_active)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COLOR_GREEN);
        added_color_follow = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("Follow###followPlayer", im_button_size))
    {
        if (IsExplorable())
        {
            following_active = !following_active;
            toggled_follow = true;

            if (following_active)
                StartFollowing();
            else
                StopFollowing();
        }
    }

    if (IsExplorable() && following_active && TIMER_DIFF(last_follow_trigger_ms) > 800 + time_dist(gen))
    {
        FollowPlayer();
        last_follow_trigger_ms = clock();

        UseFallback();
    }
    else if (IsMapReady() && IsExplorable() && toggled_follow)
    {
        GW::PartyMgr::UnflagAll();
    }

    if (added_color_follow)
        ImGui::PopStyleColor();
}

void HeroWindow::HeroSpike_DrawAndLogic(const ImVec2 &im_button_size)
{
    ImGui::SameLine();

    if (ImGui::Button("Attack###attackTarget", im_button_size))
    {
        StopFollowing();

        if (IsExplorable())
        {
            if (!player_data.target)
                return;

            const auto target_distance = GW::GetDistance(player_data.pos, player_data.target->pos);

            const auto *const party_info = GW::PartyMgr::GetPartyInfo();
            if (party_info)
            {
                for (const auto &hero : party_info->heroes)
                {
                    const auto *hero_living = GW::Agents::GetAgentByID(hero.agent_id);
                    if (!hero_living)
                        continue;

                    if (GW::GetDistance(player_data.pos, hero_living->pos) > 800.0F || target_distance > 800.0F)
                    {
                        UseFallback();
                        break;
                    }
                }
            }

            AttackTarget();
        }
    }
}

void HeroWindow::HeroSmarterSkills_Logic()
{
    if (following_active || !IsExplorable())
        return;

    UseShelterInFight();
    UseUnionInFight();
    UseSosInFight();
    UseSplinterOnPlayer();
    ShatterImportantHexes();
    RemoveImportantConditions();
    UseBipOnPlayer();
    UseHonorOnPlayer();
    RuptEnemies();
}

void HeroWindow::HeroFollow_StuckCheck()
{
    static std::vector<uint32_t> same_position_counters(7U, 0);
    static std::vector<GW::GamePos> last_positions(7U, {});

    auto hero_idx = 0U;
    for (const auto &hero : hero_data.hero_vec)
    {
        if (!hero.hero_living || !hero.hero_living->agent_id || !player_data.living ||
            !player_data.living->GetIsMoving())
        {
            same_position_counters.at(hero_idx) = 0U;
            ++hero_idx;
            continue;
        }

        const auto *hero_agent = GW::Agents::GetAgentByID(hero.hero_living->agent_id);
        if (!hero_agent || GW::GetDistance(player_data.pos, hero_agent->pos) > (GW::Constants::Range::Compass - 500.0F))
        {
            same_position_counters.at(hero_idx) = 0U;
            ++hero_idx;
            continue;
        }

        if (hero_agent->pos == last_positions.at(hero_idx))
            ++same_position_counters.at(hero_idx);
        else
            same_position_counters.at(hero_idx) = 0U;

        if (same_position_counters.at(hero_idx) >= 1'000U)
        {
            Log::Info("Hero at position %d might be stuck!", hero_idx + 1U);
            same_position_counters.at(hero_idx) = 0U;
        }

        last_positions.at(hero_idx) = hero_agent->pos;

        ++hero_idx;
    }
}

void HeroWindow::HeroFollow_StopConditions()
{
    if (!IsExplorable() || !following_active)
    {
        ms_with_no_pos_change = 0U;
        time_at_last_pos_change = TIMER_INIT();
        return;
    }

    if (ms_with_no_pos_change >= 10'000)
    {
        StopFollowing();
        ms_with_no_pos_change = 0U;
        time_at_last_pos_change = TIMER_INIT();
    }

    if (player_data.IsAttacking() && following_active)
    {
        StopFollowing();
        ms_with_no_pos_change = 0U;
        time_at_last_pos_change = TIMER_INIT();
    }
}

void HeroWindow::UpdateInternalData()
{
    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (party_info)
        hero_data.Update(party_info->heroes);
    else
        hero_data.hero_vec.clear();

    if (player_data.pos == follow_pos && following_active)
    {
        ms_with_no_pos_change = TIMER_DIFF(time_at_last_pos_change);
    }
    else
    {
        ms_with_no_pos_change = 0U;
        time_at_last_pos_change = TIMER_INIT();
    }
    follow_pos = player_data.pos;

    if (player_data.target && player_data.target->agent_id)
        target_agent_id = player_data.target->agent_id;
    else
        target_agent_id = 0U;
}

void HeroWindow::Draw(IDirect3DDevice9 *)
{
    const auto *const party_info = GW::PartyMgr::GetPartyInfo();

    if (!player_data.ValidateData(HelperActivationConditions, true) || !party_info || party_info->heroes.size() == 0 ||
        (*GetVisiblePtr()) == false)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(240.0F, 45.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(),
                     can_close && show_closebutton ? GetVisiblePtr() : nullptr,
                     GetWinFlags(ImGuiWindowFlags_NoResize)))
    {
        const auto width = ImGui::GetWindowWidth();
        const auto im_button_size = ImVec2{width / 3.0F - 10.0F, 50.0F};

        auto toggled_follow = false;

        HeroBehaviour_DrawAndLogic(im_button_size);
        HeroFollow_DrawAndLogic(im_button_size, toggled_follow);
        HeroSpike_DrawAndLogic(im_button_size);
    }
    ImGui::End();
}

void HeroWindow::Update(float)
{
    if (!player_data.ValidateData(HelperActivationConditions, true))
    {
        ResetData();
        return;
    }

    player_data.Update();
    livings_data.Update();
    UpdateInternalData();

    HeroSmarterSkills_Logic();
    HeroFollow_StopConditions();
    HeroFollow_StuckCheck();
}

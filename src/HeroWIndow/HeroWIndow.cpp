#include <array>
#include <cmath>
#include <cstdint>
#include <random>

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
#include <GWCA/Utilities/Scanner.h>

#include "ActionTypes.h"
#include "ActionsBase.h"
#include "DataPlayer.h"
#include "Helper.h"
#include "HelperAgents.h"
#include "HelperHero.h"
#include "HelperMaps.h"
#include "HeroWindow.h"
#include "Logger.h"
#include "Utils.h"

#include <imgui.h>

namespace
{
constexpr auto IM_COLOR_RED = ImVec4(1.0F, 0.1F, 0.1F, 1.0F);
constexpr auto IM_COLOR_GREEN = ImVec4(0.1F, 0.9F, 0.1F, 1.0F);
constexpr auto IM_COLOR_BLUE = ImVec4(0.1F, 0.1F, 1.0F, 1.0F);

constexpr auto SOS1_AGENT_ID = uint32_t{4229};
constexpr auto SOS2_AGENT_ID = uint32_t{4230};
constexpr auto SOS3_AGENT_ID = uint32_t{4231};

void OnSkillActivaiton(GW::HookStatus *status, const GW::UI::UIMessage message_id, void *wParam, void *lParam)
{
    const struct Payload
    {
        uint32_t agent_id;
        GW::Constants::SkillID skill_id;
    } *payload = static_cast<Payload *>(wParam);

    if (payload->agent_id == GW::Agents::GetPlayerId() &&
        payload->skill_id == static_cast<GW::Constants::SkillID>(0U)) // TODO
    {
        status->blocked = true;
    }

    (void)message_id;
    (void)lParam;
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
    GW::Initialize();

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::MapLoaded>(
        &MapLoaded_Entry,
        [this](GW::HookStatus *, const GW::Packet::StoC::MapLoaded *) -> void { ResetData(); });

    // GW::UI::RegisterUIMessageCallback(&OnSkillActivated_Entry, GW::UI::UIMessage::kSkillActivated, OnSkillActivaiton);

    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"HeroWindow");
}

void HeroWindow::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

void HeroWindow::StopFollowing()
{
    following_active = false;
    GW::PartyMgr::UnflagAll();
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

void HeroWindow::SmartUseSkill(const GW::Constants::SkillID skill_id,
                               const GW::Constants::Profession skill_class,
                               const std::string_view skill_name,
                               const long wait_ms,
                               const bool use_player_target,
                               std::function<bool(const DataPlayer &, const AgentLivingData &)> player_conditions,
                               std::function<bool(const DataPlayer &, const Hero &)> hero_conditions)
{
    if (!player_conditions(player_data, livings_data))
        return;

    if (!HeroSkill_StartConditions(skill_id, wait_ms))
        return;

    auto hero_idxs_zero_based = hero_data.hero_class_idx_map[skill_class];
    if (hero_idxs_zero_based.size() == 0)
        return;

    for (const auto hero_idx_zero_based : hero_idxs_zero_based)
    {
        const auto &hero = hero_data.hero_vec[hero_idx_zero_based];

        if (HeroCastSkillIfAvailable(hero, player_data, skill_id, hero_conditions, use_player_target))
        {
#ifdef _DEBUG
            Log::Info("Casted %s.", skill_name);
#endif
            return;
        }
    }
}

void HeroWindow::UseSplinterOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Splinter_Weapon;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 100UL;
    constexpr static auto use_target = false;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &livings_data) {
        const auto num_enemies_at_player = std::count_if(livings_data.enemies.begin(),
                                                         livings_data.enemies.end(),
                                                         [&player_data](const GW::AgentLiving *enemy_living) {
                                                             if (!enemy_living)
                                                                 return false;

                                                             const auto dist =
                                                                 GW::GetDistance(enemy_living->pos, player_data.pos);

                                                             return dist < GW::Constants::Range::Nearby;
                                                         });


        const auto player_is_melee_attacking = player_data.IsAttacking() && player_data.holds_melee_weapon;

        return num_enemies_at_player >= 2 && player_is_melee_attacking;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast && hero.hero_living->energy > 0.50F;
    };

    return SmartUseSkill(skill_id, skill_class, "Splinter", wait_ms, use_target, player_conditions, hero_conditions);
}

void HeroWindow::UseHonorOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Strength_of_Honor;
    constexpr static auto skill_class = GW::Constants::Profession::Monk;
    constexpr static auto wait_ms = 100UL;
    constexpr static auto use_target = false;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &) {
        const auto player_is_melee_attacking = player_data.holds_melee_weapon;

        return player_is_melee_attacking; // TODO: And has Sin,Ranger,War,Derv,Para as class
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast && hero.hero_living->energy > 0.25F;
    };

    return SmartUseSkill(skill_id, skill_class, "Honor", wait_ms, use_target, player_conditions, hero_conditions);
}

void HeroWindow::UseShelterAtFightEnter()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Shelter;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 100UL;
    constexpr static auto use_target = false;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &livings_data) {
        const auto num_enemies_at_player = std::count_if(livings_data.enemies.begin(),
                                                         livings_data.enemies.end(),
                                                         [&player_data](const GW::AgentLiving *enemy_living) {
                                                             if (!enemy_living)
                                                                 return false;

                                                             const auto dist =
                                                                 GW::GetDistance(enemy_living->pos, player_data.pos);

                                                             return dist < GW::Constants::Range::Spellcast;
                                                         });

        const auto player_started_fight = num_enemies_at_player >= 4 && player_data.IsAttacking();

        return !player_data.PlayerHasEffect(GW::Constants::SkillID::Shelter) && player_started_fight;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spirit - 100.0F;
    };

    return SmartUseSkill(skill_id, skill_class, "Shelter", wait_ms, use_target, player_conditions, hero_conditions);
}

void HeroWindow::UseUnionAtFightEnter()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Union;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 100UL;
    constexpr static auto use_target = false;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &livings_data) {
        const auto num_enemies_at_player = std::count_if(livings_data.enemies.begin(),
                                                         livings_data.enemies.end(),
                                                         [&player_data](const GW::AgentLiving *enemy_living) {
                                                             if (!enemy_living)
                                                                 return false;

                                                             const auto dist =
                                                                 GW::GetDistance(enemy_living->pos, player_data.pos);

                                                             return dist < GW::Constants::Range::Spellcast;
                                                         });

        const auto player_started_fight = num_enemies_at_player >= 4 && player_data.IsAttacking();

        return !player_data.PlayerHasEffect(GW::Constants::SkillID::Union) && player_started_fight;
    };

    const auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spirit - 100.0F;
    };

    return SmartUseSkill(skill_id, skill_class, "Union", wait_ms, use_target, player_conditions, hero_conditions);
}

void HeroWindow::UseSosAtFightEnter()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Signet_of_Spirits;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;
    constexpr static auto wait_ms = 100UL;
    constexpr static auto use_target = false;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &livings_data) {
        const auto num_enemies_at_player = std::count_if(livings_data.enemies.begin(),
                                                         livings_data.enemies.end(),
                                                         [&player_data](const GW::AgentLiving *enemy_living) {
                                                             if (!enemy_living)
                                                                 return false;

                                                             const auto dist =
                                                                 GW::GetDistance(enemy_living->pos, player_data.pos);

                                                             return dist < GW::Constants::Range::Spellcast;
                                                         });

        const auto player_started_fight = num_enemies_at_player >= 4 && player_data.IsAttacking();

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

    return SmartUseSkill(skill_id, skill_class, "SoS", wait_ms, use_target, player_conditions, hero_conditions);
}

void HeroWindow::UseFallback()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Fall_Back;
    constexpr static auto skill_class = GW::Constants::Profession::Paragon;
    constexpr static auto wait_ms = 100UL;
    constexpr static auto use_target = false;

    auto player_conditions = [](const DataPlayer &, const AgentLivingData &) { return true; };

    auto hero_conditions = [](const DataPlayer &player_data, const Hero &) {
        return !player_data.AnyTeamMemberHasEffect(GW::Constants::SkillID::Fall_Back);
    };

    return SmartUseSkill(skill_id, skill_class, "FallBack", wait_ms, use_target, player_conditions, hero_conditions);
}

void HeroWindow::UseBipOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Blood_is_Power;
    constexpr static auto skill_class = GW::Constants::Profession::Necromancer;
    constexpr static auto wait_ms = 100UL;
    constexpr static auto use_target = false;

    auto player_conditions = [](const DataPlayer &player_data, const AgentLivingData &) {
        if (!player_data.living)
            return false;

        if (player_data.energy_perc > 0.30F)
            return false;

        if (player_data.living->energy_regen > 0.03F)
            return false;

        return true;
    };

    auto hero_conditions = [](const DataPlayer &player_data, const Hero &hero) {
        if (!hero.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast && hero.hero_living->hp > 0.80F;
    };

    return SmartUseSkill(skill_id, skill_class, "BiP", wait_ms, use_target, player_conditions, hero_conditions);
}

void HeroWindow::MesmerSpikeTarget(const Hero &hero) const
{
    constexpr static auto skill_id = GW::Constants::SkillID::Energy_Surge;
    constexpr static auto skill_class = GW::Constants::Profession::Mesmer;

    if (!hero.hero_living)
        return;

    auto hero_conditions = [](const DataPlayer &, const Hero &) { return true; };

    if (hero.hero_living->primary == static_cast<uint8_t>(skill_class))
        HeroCastSkillIfAvailable(hero, player_data, skill_id, hero_conditions, true);
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
                Log::Info("Heroes will follow the player!");
            else
                Log::Info("Heroes stopped following the player!");
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
            AttackTarget();
    }
}

void HeroWindow::HeroSmarterSkills_Logic()
{
    if (following_active || !IsExplorable())
        return;

    UseSplinterOnPlayer();
    UseBipOnPlayer();
    UseHonorOnPlayer();
    UseShelterAtFightEnter();
    UseUnionAtFightEnter();
    UseSosAtFightEnter();
}

void HeroWindow::HeroFollow_StopConditions()
{
    if (!IsExplorable() || !following_active)
        return;

    if (ms_with_no_pos_change >= 20'000 && following_active)
    {
        Log::Info("Players seesm to be afk. Stopping hero following.");
        StopFollowing();
        ms_with_no_pos_change = 0U;
    }

    if (player_data.IsAttacking() && following_active)
    {
        Log::Info("Player attacks enemies. Stopping hero following.");
        StopFollowing();
        ms_with_no_pos_change = 0U;
    }
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

        HeroFollow_StopConditions();
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

    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (party_info)
        hero_data.Update(party_info->heroes);
    else
        hero_data.hero_vec.clear();

    auto time_at_last_pos_change = 0;

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

    if (player_data.target)
        target_agent_id = player_data.target->agent_id;
    else
        target_agent_id = 0U;

    HeroSmarterSkills_Logic();
}

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
#include "HelperMaps.h"
#include "Logger.h"
#include "Utils.h"

#include <imgui.h>

#include "HeroWindow.h"

namespace
{
constexpr auto IM_COLOR_RED = ImVec4(1.0F, 0.1F, 0.1F, 1.0F);
constexpr auto IM_COLOR_GREEN = ImVec4(0.1F, 0.9F, 0.1F, 1.0F);
constexpr auto IM_COLOR_BLUE = ImVec4(0.1F, 0.1F, 1.0F, 1.0F);

constexpr auto SOS1_AGENT_ID = uint32_t{4229};
constexpr auto SOS2_AGENT_ID = uint32_t{4230};
constexpr auto SOS3_AGENT_ID = uint32_t{4231};

void HeroUseSkill(const uint32_t target_agent_id, const uint32_t skill_idx, const uint32_t hero_idx_zero_based)
{
    auto hero_action = GW::UI::ControlAction_Hero1Skill1;
    if (hero_idx_zero_based == 0)
        hero_action = GW::UI::ControlAction_Hero1Skill1;
    else if (hero_idx_zero_based == 1)
        hero_action = GW::UI::ControlAction_Hero2Skill1;
    else if (hero_idx_zero_based == 2)
        hero_action = GW::UI::ControlAction_Hero3Skill1;
    else if (hero_idx_zero_based == 3)
        hero_action = GW::UI::ControlAction_Hero4Skill1;
    else if (hero_idx_zero_based == 4)
        hero_action = GW::UI::ControlAction_Hero5Skill1;
    else if (hero_idx_zero_based == 5)
        hero_action = GW::UI::ControlAction_Hero6Skill1;
    else if (hero_idx_zero_based == 6)
        hero_action = GW::UI::ControlAction_Hero7Skill1;
    else
        return;

    const auto curr_target_id = GW::Agents::GetTargetId();

    GW::GameThread::Enqueue([=] {
        if (target_agent_id && target_agent_id != GW::Agents::GetTargetId())
            GW::Agents::ChangeTarget(target_agent_id);
        GW::UI::Keypress((GW::UI::ControlAction)(static_cast<uint32_t>(hero_action) + skill_idx));
        if (curr_target_id && target_agent_id != curr_target_id)
            GW::Agents::ChangeTarget(curr_target_id);
    });
}

bool HeroCastSkillIfAvailable(const HeroData &hero_data,
                              const DataPlayer &player_data,
                              const GW::Constants::SkillID skill_id,
                              std::function<bool(const DataPlayer &, const HeroData &)> cb_fn,
                              const bool use_player_target)
{
    if (!hero_data.hero_living || !hero_data.hero_living->agent_id)
        return false;

    if (!cb_fn(player_data, hero_data))
    {
        Log::Info("Cast cb fn  returned false");
        return false;
    }

    auto skill_idx = 0U;
    for (const auto &skill : hero_data.skills)
    {
        const auto has_skill_in_skillbar = skill.skill_id == skill_id;
        if (!has_skill_in_skillbar)
        {
            ++skill_idx;
            continue;
        }

        const auto *skill_data = GW::SkillbarMgr::GetSkillConstantData(skill_id);
        if (!skill_data)
        {
            ++skill_idx;
            continue;
        }

        const auto hero_energy =
            (static_cast<uint32_t>(hero_data.hero_living->energy) * hero_data.hero_living->max_energy);

        if (has_skill_in_skillbar && skill.GetRecharge() == 0 && hero_energy >= skill_data->GetEnergyCost())
        {
            HeroUseSkill(use_player_target ? player_data.target->agent_id : player_data.id,
                         skill_idx,
                         hero_data.hero_idx_zero_based);
            return true;
        }

        ++skill_idx;
    }

    return false;
}

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

bool HeroWindow::HeroSkill_StartConditions(const GW::Constants::SkillID skill_id, const long wait_time_ms)
{
    if (!IsMapReady() || !IsExplorable() || hero_data_vec.size() == 0)
        return false;

    if (!ActionABC::HasWaitedLongEnough(wait_time_ms))
        return false;

    if (skill_id != GW::Constants::SkillID::No_Skill && player_data.PlayerHasEffect(skill_id))
        return false;

    return true;
}

std::vector<uint32_t> HeroWindow::HeroSkill_GetHeroIndexWithCertainClass(const GW::Constants::Profession &skill_class)
{
    auto hero_idxs_with_certain_class = std::vector<uint32_t>{};

    auto hero_idx_zero_based = 0U;
    for (const auto &hero_data : hero_data_vec)
    {
        if (!hero_data.hero_living)
            continue;

        const auto has_skill_class = (hero_data.hero_living->primary == static_cast<uint8_t>(skill_class) ||
                                      hero_data.hero_living->secondary == static_cast<uint8_t>(skill_class));

        if (has_skill_class)
            hero_idxs_with_certain_class.push_back(hero_idx_zero_based);

        ++hero_idx_zero_based;
    }

    return hero_idxs_with_certain_class;
}

bool player_conditions_splinter(const DataPlayer &player_data, const AgentLivingData &livings_data)
{
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

void HeroWindow::UseSplinterOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Splinter_Weapon;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;

    if (!player_conditions_splinter(player_data, livings_data))
        return;

    if (!HeroSkill_StartConditions(skill_id, 100UL))
        return;

    auto hero_idxs_zero_based = HeroSkill_GetHeroIndexWithCertainClass(skill_class);
    if (hero_idxs_zero_based.size() == 0)
        return;

    const auto hero_conditions = [](const DataPlayer &player_data, const HeroData &hero_data) {
        if (!hero_data.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero_data.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast && hero_data.hero_living->energy > 0.50F;
    };

    for (const auto hero_idx_zero_based : hero_idxs_zero_based)
    {
        const auto &hero_data = hero_data_vec[hero_idx_zero_based];

        if (HeroCastSkillIfAvailable(hero_data, player_data, skill_id, hero_conditions, false))
        {
            Log::Info("Casted Splinter.");
            return;
        }
    }
}

bool player_conditions_honor(const DataPlayer &player_data, const AgentLivingData &livings_data)
{
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

void HeroWindow::UseHonorOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Strength_of_Honor;
    constexpr static auto skill_class = GW::Constants::Profession::Monk;

    if (!player_conditions_honor(player_data, livings_data))
        return;

    if (!HeroSkill_StartConditions(skill_id, 100UL))
        return;

    auto hero_idxs_zero_based = HeroSkill_GetHeroIndexWithCertainClass(skill_class);
    if (hero_idxs_zero_based.size() == 0)
        return;

    const auto hero_conditions = [](const DataPlayer &player_data, const HeroData &hero_data) {
        if (!hero_data.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero_data.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast && hero_data.hero_living->energy > 0.25F;
    };

    for (const auto hero_idx_zero_based : hero_idxs_zero_based)
    {
        const auto &hero_data = hero_data_vec[hero_idx_zero_based];

        if (HeroCastSkillIfAvailable(hero_data, player_data, skill_id, hero_conditions, false))
        {
            Log::Info("Casted Honor.");
            return;
        }
    }
}

bool player_conditions_shelter(const DataPlayer &player_data, const AgentLivingData &livings_data)
{
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

void HeroWindow::UseShelterAtFightEnter()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Shelter;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;

    if (!player_conditions_shelter(player_data, livings_data))
        return;

    if (!HeroSkill_StartConditions(skill_id, 100UL))
        return;

    auto hero_idxs_zero_based = HeroSkill_GetHeroIndexWithCertainClass(skill_class);
    if (hero_idxs_zero_based.size() == 0)
        return;

    const auto hero_conditions = [](const DataPlayer &player_data, const HeroData &hero_data) {
        if (!hero_data.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero_data.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spirit - 100.0F;
    };

    for (const auto hero_idx_zero_based : hero_idxs_zero_based)
    {
        const auto &hero_data = hero_data_vec[hero_idx_zero_based];

        if (HeroCastSkillIfAvailable(hero_data, player_data, skill_id, hero_conditions, false))
        {
            Log::Info("Casted Shelter.");
            return;
        }
    }
}

bool player_conditions_union(const DataPlayer &player_data, const AgentLivingData &livings_data)
{
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

void HeroWindow::UseUnionAtFightEnter()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Union;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;

    if (!player_conditions_union(player_data, livings_data))
        return;

    if (!HeroSkill_StartConditions(skill_id, 100UL))
        return;

    auto hero_idxs_zero_based = HeroSkill_GetHeroIndexWithCertainClass(skill_class);
    if (hero_idxs_zero_based.size() == 0)
        return;

    const auto hero_conditions = [](const DataPlayer &player_data, const HeroData &hero_data) {
        if (!hero_data.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero_data.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spirit - 100.0F;
    };

    for (const auto hero_idx_zero_based : hero_idxs_zero_based)
    {
        const auto &hero_data = hero_data_vec[hero_idx_zero_based];

        if (HeroCastSkillIfAvailable(hero_data, player_data, skill_id, hero_conditions, false))
        {
            Log::Info("Casted Union.");
            return;
        }
    }
}

bool player_conditions_sos(const DataPlayer &player_data, const AgentLivingData &livings_data)
{
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

void HeroWindow::UseSosAtFightEnter()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Signet_of_Spirits;
    constexpr static auto skill_class = GW::Constants::Profession::Ritualist;

    if (!player_conditions_sos(player_data, livings_data))
        return;

    if (!HeroSkill_StartConditions(skill_id, 100UL))
        return;

    auto hero_idxs_zero_based = HeroSkill_GetHeroIndexWithCertainClass(skill_class);
    if (hero_idxs_zero_based.size() == 0)
        return;

    const auto hero_conditions = [](const DataPlayer &player_data, const HeroData &hero_data) {
        if (!hero_data.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero_data.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast;
    };

    for (const auto hero_idx_zero_based : hero_idxs_zero_based)
    {
        const auto &hero_data = hero_data_vec[hero_idx_zero_based];

        if (HeroCastSkillIfAvailable(hero_data, player_data, skill_id, hero_conditions, false))
        {
            Log::Info("Casted SoS.");
            return;
        }
    }
}

bool player_conditions_bip(const DataPlayer &player_data, const AgentLivingData &)
{
    if (!player_data.living)
        return false;

    if (player_data.energy_perc > 0.30F)
        return false;

    if (player_data.living->energy_regen > 0.03F)
        return false;

    return true;
}

void HeroWindow::UseBipOnPlayer()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Blood_is_Power;
    constexpr static auto skill_class = GW::Constants::Profession::Necromancer;

    if (!player_conditions_bip(player_data, livings_data))
        return;

    if (!HeroSkill_StartConditions(skill_id, 100UL))
        return;

    auto hero_idxs_zero_based = HeroSkill_GetHeroIndexWithCertainClass(skill_class);
    if (hero_idxs_zero_based.size() == 0)
        return;

    auto hero_conditions = [](const DataPlayer &player_data, const HeroData &hero_data) {
        if (!hero_data.hero_living)
            return false;

        const auto dist = GW::GetDistance(hero_data.hero_living->pos, player_data.pos);

        return dist < GW::Constants::Range::Spellcast && hero_data.hero_living->hp > 0.80F;
    };

    for (const auto hero_idx_zero_based : hero_idxs_zero_based)
    {
        const auto &hero_data = hero_data_vec[hero_idx_zero_based];

        if (HeroCastSkillIfAvailable(hero_data, player_data, skill_id, hero_conditions, false))
        {
            Log::Info("Casted BIP.");
            return;
        }
    }
}

void HeroWindow::UseFallback()
{
    constexpr static auto skill_id = GW::Constants::SkillID::Fall_Back;
    constexpr static auto skill_class = GW::Constants::Profession::Paragon;

    if (!HeroSkill_StartConditions(skill_id))
        return;

    auto hero_idxs_zero_based = HeroSkill_GetHeroIndexWithCertainClass(skill_class);
    if (hero_idxs_zero_based.size() == 0)
        return;

    auto hero_conditions = [](const DataPlayer &player_data, const HeroData &) {
        return !player_data.AnyTeamMemberHasEffect(GW::Constants::SkillID::Fall_Back);
    };

    for (const auto hero_idx_zero_based : hero_idxs_zero_based)
    {
        const auto &hero_data = hero_data_vec[hero_idx_zero_based];

        if (HeroCastSkillIfAvailable(hero_data, player_data, skill_id, hero_conditions, false))
        {
            Log::Info("Used Fall Back.");
            return;
        }
    }
}

void HeroWindow::MesmerSpikeTarget(const HeroData &hero_data) const
{
    constexpr static auto skill_id = GW::Constants::SkillID::Energy_Surge;
    constexpr static auto skill_class = GW::Constants::Profession::Mesmer;

    if (!hero_data.hero_living)
        return;

    auto hero_conditions = [](const DataPlayer &, const HeroData &) { return true; };

    if (hero_data.hero_living->primary == static_cast<uint8_t>(skill_class))
        HeroCastSkillIfAvailable(hero_data, player_data, skill_id, hero_conditions, true);
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
    if (!player_conditions_attack)
        return;

    if (!HeroSkill_StartConditions(GW::Constants::SkillID::No_Skill))
        return;

    Log::Info("Mesmer Heroes will attack the players target!");

    for (const auto &hero : hero_data_vec)
    {
        MesmerSpikeTarget(hero);
    }
}

void HeroWindow::ResetData()
{
    target_agent_id = 0U;
    follow_pos = GW::GamePos{};
    following_active = false;
    hero_data_vec.clear();
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

    UseBipOnPlayer();
    UseSplinterOnPlayer();
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

bool HeroWindow::UpdateHeroData()
{
    hero_data_vec.clear();

    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (!party_info)
        return false;

    auto hero_idx_zero_based = 0U;
    auto skills = std::array<GW::SkillbarSkill, 8U>{};

    for (const auto &hero : party_info->heroes)
    {
        if (!hero.agent_id)
        {
            ++hero_idx_zero_based;
            continue;
        }

        auto *hero_agent = GW::Agents::GetAgentByID(hero.agent_id);
        if (!hero_agent)
        {
            ++hero_idx_zero_based;
            continue;
        }
        auto *hero_living = hero_agent->GetAsAgentLiving();
        if (!hero_living)
        {
            ++hero_idx_zero_based;
            continue;
        }

        const auto *skillbar_array = GW::SkillbarMgr::GetSkillbarArray();
        if (!skillbar_array)
        {
            ++hero_idx_zero_based;
            continue;
        }

        for (const auto &skillbar : *skillbar_array)
        {
            if (skillbar.agent_id == hero_living->agent_id)
            {
                for (uint32_t i = 0; i < 8; i++)
                {
                    skills[i] = skillbar.skills[i];
                }

                break;
            }
        }

        auto hero_data =
            HeroData{.hero_living = hero_living, .hero_idx_zero_based = hero_idx_zero_based, .skills = skills};
        hero_data_vec.push_back(hero_data);

        ++hero_idx_zero_based;
    }

    return true;
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
    UpdateHeroData();

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

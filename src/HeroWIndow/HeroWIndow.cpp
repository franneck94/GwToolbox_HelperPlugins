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
constexpr auto CTOS_ID_HERO_ACTION = 0xC;

constexpr auto IM_COLOR_RED = ImVec4(1.0F, 0.1F, 0.1F, 1.0F);
constexpr auto IM_COLOR_GREEN = ImVec4(0.1F, 0.9F, 0.1F, 1.0F);
constexpr auto IM_COLOR_BLUE = ImVec4(0.1F, 0.1F, 1.0F, 1.0F);

void HeroUseSkill(const uint32_t hero_agent_id,
                  const uint32_t target_agent_id,
                  const uint32_t skill_idx,
                  const uint32_t hero_idx)
{
    auto hero_action = GW::UI::ControlAction_Hero1Skill1;
    if (hero_idx == 1)
        hero_action = GW::UI::ControlAction_Hero1Skill1;
    else if (hero_idx == 2)
        hero_action = GW::UI::ControlAction_Hero2Skill1;
    else if (hero_idx == 3)
        hero_action = GW::UI::ControlAction_Hero3Skill1;
    else if (hero_idx == 4)
        hero_action = GW::UI::ControlAction_Hero4Skill1;
    else if (hero_idx == 5)
        hero_action = GW::UI::ControlAction_Hero5Skill1;
    else if (hero_idx == 6)
        hero_action = GW::UI::ControlAction_Hero6Skill1;
    else if (hero_idx == 7)
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

bool HeroCastSkillIfAvailable(const GW::HeroPartyMember &hero,
                              const GW::AgentLiving *hero_living,
                              const uint32_t target_agent_id,
                              const GW::Constants::SkillID skill_id,
                              const uint32_t hero_idx)
{
    if (!hero_living || (skill_id == GW::Constants::SkillID::No_Skill))
        return false;

    const auto *skillbar_array = GW::SkillbarMgr::GetSkillbarArray();
    if (!skillbar_array)
        return false;

    for (const auto &skillbar : *skillbar_array)
    {
        if (skillbar.agent_id != hero_living->agent_id)
            continue;

        auto skill_idx = 0U;
        for (const auto &skill : skillbar.skills)
        {
            const auto has_skill_in_skillbar = skill.skill_id == skill_id;
            if (!has_skill_in_skillbar)
            {
                ++skill_idx;
                continue;
            }

            const auto *skill_data = GW::SkillbarMgr::GetSkillConstantData(skill_id);
            if (!has_skill_in_skillbar)
            {
                ++skill_idx;
                continue;
            }

            const auto hero_energy = (static_cast<uint32_t>(hero_living->energy) * hero_living->max_energy);

            if (has_skill_in_skillbar && skill.GetRecharge() == 0 && hero_energy >= skill_data->GetEnergyCost())
            {
                HeroUseSkill(hero.agent_id, target_agent_id, skill_idx, hero_idx);
                return true;
            }

            ++skill_idx;
        }
    }

    return false;
}

// void OnSkillActivaiton2(GW::HookStatus *status, const GW::UI::UIMessage message_id, void *wParam, void *lParam)
// {
//     const struct Payload
//     {
//         uint32_t agent_id;
//         GW::Constants::SkillID skill_id;
//     } *payload = static_cast<Payload *>(wParam);

//     if (payload->agent_id == GW::Agents::GetPlayerId())
//     {
//         int i = 2;
//         status->blocked = true;
//     }
// }
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

    // GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericValueTarget>(
    //     &OnSkillActivated_Entry,
    //     [this](GW::HookStatus *status, GW::Packet::StoC::GenericValueTarget *packet) -> void {
    //         OnSkillActivaiton(status, packet);
    //     });

    // GW::UI::RegisterUIMessageCallback(&OnSkillActivated_Entry,
    //                                   GW::UI::UIMessage::kSkillActivated,
    //                                   OnSkillActivaiton2);

    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"HeroWindow");
}

void HeroWindow::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

uint32_t HeroWindow::GetNumPlayerHeroes()
{
    if (!party_heros)
        return 0U;

    auto num = uint32_t{0};
    for (const auto &hero : *party_heros)
    {
        if (hero.owner_player_id == player_data.living->login_number)
            ++num;
    }

    return num;
}

void HeroWindow::ToggleHeroBehaviour()
{
    if (!IsMapReady() || !party_heros)
        return;

    Log::Info("Toggle hero hehaviour!");

    if (current_hero_behaviour == GW::HeroBehavior::Guard)
        current_hero_behaviour = GW::HeroBehavior::AvoidCombat;
    else if (current_hero_behaviour == GW::HeroBehavior::Fight)
        current_hero_behaviour = GW::HeroBehavior::Guard;
    else if (current_hero_behaviour == GW::HeroBehavior::AvoidCombat)
        current_hero_behaviour = GW::HeroBehavior::Fight;

    for (const auto &hero : *party_heros)
    {
        if (hero.owner_player_id == player_data.living->login_number)
            GW::PartyMgr::SetHeroBehavior(hero.agent_id, current_hero_behaviour);
    }
}

void HeroWindow::FollowPlayer()
{
    if (!IsMapReady() || !IsExplorable() || (follow_pos.x == 0.0F && follow_pos.y == 0.0F) ||
        !GW::PartyMgr::GetIsPartyLoaded() || GW::PartyMgr::GetIsPartyDefeated())
        return;

    GW::PartyMgr::FlagAll(follow_pos);
}

void HeroWindow::UseBipOnPlayer()
{
    constexpr static auto wait_time_ms = 1000;

    if (!IsMapReady() || !IsExplorable() || !party_heros)
        return;

    if (!ActionABC::HasWaitedLongEnough(wait_time_ms))
        return;

    if (player_data.energy_perc > 0.30F)
        return;

    if (player_data.PlayerHasEffect(GW::Constants::SkillID::Blood_is_Power))
        return;

    if (player_data.living->energy_regen > 0.03F)
        return;

    auto hero_idx = 1U;
    for (const auto &hero : *party_heros)
    {
        if (hero.owner_player_id == player_data.living->login_number)
        {
            const auto *hero_agent = GW::Agents::GetAgentByID(hero.agent_id);
            if (!hero_agent)
            {
                ++hero_idx;
                continue;
            }
            const auto *hero_living = hero_agent->GetAsAgentLiving();
            if (!hero_living)
            {
                ++hero_idx;
                continue;
            }

            const auto dist = GW::GetDistance(hero_living->pos, player_data.pos);

            if (hero_living->primary == static_cast<uint8_t>(GW::Constants::Profession::Necromancer) &&
                dist < GW::Constants::Range::Spellcast && hero_living->hp > 0.80F)
            {
                if (HeroCastSkillIfAvailable(hero,
                                             hero_living,
                                             player_data.id,
                                             GW::Constants::SkillID::Blood_is_Power,
                                             hero_idx))
                {
                    Log::Info("Casted BIP.");
                    return;
                }
            }

            ++hero_idx;
        }
    }
}

void HeroWindow::MesmerSpikeTarget(const GW::HeroPartyMember &hero, const uint32_t hero_idx) const
{
    if (!IsMapReady() || !IsExplorable())
        return;

    const auto *hero_agent = GW::Agents::GetAgentByID(hero.agent_id);
    if (!hero_agent)
        return;
    const auto *hero_living = hero_agent->GetAsAgentLiving();
    if (!hero_living)
        return;

    if (hero_living->primary == static_cast<uint8_t>(GW::Constants::Profession::Mesmer))
        HeroCastSkillIfAvailable(hero, hero_living, target_agent_id, GW::Constants::SkillID::Energy_Surge, hero_idx);
}

void HeroWindow::UseFallback()
{
    constexpr static auto wait_time_ms = 200;

    if (!IsMapReady() || !IsExplorable() || !party_heros)
        return;

    if (!ActionABC::HasWaitedLongEnough(wait_time_ms))
        return;

    if (player_data.AnyTeamMemberHasEffect(GW::Constants::SkillID::Fall_Back))
        return;

    auto hero_idx = 1U;
    for (const auto &hero : *party_heros)
    {
        if (hero.owner_player_id == player_data.living->login_number)
        {
            const auto *hero_agent = GW::Agents::GetAgentByID(hero.agent_id);
            if (!hero_agent)
            {
                ++hero_idx;
                continue;
            }
            const auto *hero_living = hero_agent->GetAsAgentLiving();
            if (!hero_living)
            {
                ++hero_idx;
                continue;
            }

            if (hero_living->secondary == static_cast<uint8_t>(GW::Constants::Profession::Paragon))
            {
                if (HeroCastSkillIfAvailable(hero,
                                             hero_living,
                                             target_agent_id,
                                             GW::Constants::SkillID::Fall_Back,
                                             hero_idx))
                {
                    Log::Info("Used Fall Back.");
                    return;
                }
            }

            ++hero_idx;
        }
    }
}

void HeroWindow::AttackTarget()
{
    if (!IsMapReady() || !IsExplorable() || !target_agent_id || !party_heros)
        return;

    const auto *target_agent = GW::Agents::GetAgentByID(target_agent_id);
    if (!target_agent)
        return;
    const auto *target_living = target_agent->GetAsAgentLiving();
    if (!target_living)
        return;

    if (target_living->allegiance != GW::Constants::Allegiance::Enemy)
        return;

    Log::Info("Heroes will attack the players target!");

    auto hero_idx = 1U;
    for (const auto &hero : *party_heros)
    {
        if (hero.owner_player_id == player_data.living->login_number)
            MesmerSpikeTarget(hero, hero_idx);

        ++hero_idx;
    }
}

void HeroWindow::ResetData()
{
    party_heros = nullptr;
    target_agent_id = 0U;
    follow_pos = GW::GamePos{};
    following_active = false;
}

void SkillCallback(const uint32_t value_id,
                   const uint32_t caster_id,
                   const uint32_t target_id,
                   const uint32_t value,
                   const bool no_target,
                   GW::HookStatus *status)
{
    static constexpr auto sin_first_combo_skill = GW::Constants::SkillID::Fox_Fangs;
    const auto activated_skill_id = static_cast<GW::Constants::SkillID>(value);

    switch (value_id)
    {
    case GW::Packet::StoC::GenericValueID::effect_on_target:
    case GW::Packet::StoC::GenericValueID::attack_started:
    case GW::Packet::StoC::GenericValueID::attack_skill_activated:
    case GW::Packet::StoC::GenericValueID::skill_activated:
    {
        break;
    }
    default:
    {
        return;
    }
    }

    if (activated_skill_id == sin_first_combo_skill)
    {
        GW::UI::Keypress(GW::UI::ControlAction_CancelAction);
        GW::UI::Keypress(GW::UI::ControlAction_CancelAction);

        Log::Info("Casting of the skill has been blocked.");
    }
}

// void HeroWindow::OnSkillActivaiton(GW::HookStatus *status, GW::Packet::StoC::GenericValueTarget *packet) const
// {
//     const uint32_t value_id = packet->Value_id;
//     const uint32_t caster_id = packet->caster;
//     const uint32_t target_id = packet->target;
//     const uint32_t value = packet->value;
//     constexpr bool no_target = false;

//     if (target_id != player_data.id || (GW::Constants::SkillID)value == GW::Constants::SkillID::No_Skill)
//         return;

//     SkillCallback(value_id, caster_id, target_id, value, no_target, status);
// }

void HeroWindow::Draw(IDirect3DDevice9 *)
{
    static auto last_follow_trigger_ms = clock();
    static auto gen = std::mt19937{};
    static auto time_dist = std::uniform_int_distribution<long>(-10, 10);

    if (!player_data.ValidateData(HelperActivationConditions, true) || !party_heros || party_heros->size() == 0 ||
        GetNumPlayerHeroes() == 0 || (*GetVisiblePtr()) == false)
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

        auto added_color_follow = false;
        auto toggled_follow = false;

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

        if (following_active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COLOR_GREEN);
            added_color_follow = true;
        }

        ImGui::SameLine();

        if (ImGui::Button("Follow###followPlayer", im_button_size))
        {
            following_active = !following_active;
            toggled_follow = true;

            Log::Info("Heroes will follow the player!");
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
        else if (IsExplorable() && !following_active)
        {
            UseBipOnPlayer();
        }

        if (added_color_follow)
            ImGui::PopStyleColor();

        ImGui::SameLine();

        if (ImGui::Button("Attack###attackTarget", im_button_size))
        {
            if (IsExplorable())
                AttackTarget();
        }
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

    const auto *const party_info = GW::PartyMgr::GetPartyInfo();
    if (party_info)
    {
        party_heros = &party_info->heroes;
    }
    else
    {
        ResetData();
        return;
    }

    follow_pos = player_data.pos;
    if (player_data.target)
    {
        target_agent_id = player_data.target->agent_id;
    }
    else
    {
        target_agent_id = 0U;
    }
}

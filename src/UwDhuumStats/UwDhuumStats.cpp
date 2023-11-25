#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Packets/Opcodes.h>
#include <GWCA/Packets/StoC.h>
#include <GWCA/Utilities/Hook.h>

#include "ActionsBase.h"
#include "ActionsUw.h"
#include "DataPlayer.h"
#include "Helper.h"
#include "HelperUw.h"
#include "HelperUwPos.h"
#include "UtilsGui.h"
#include "UtilsMath.h"

#include "UwDhuumStats.h"

namespace
{
constexpr static auto TIME_WINDOW_DMG_S = long{180L};
constexpr static auto TIME_WINDOW_DMG_MS = (TIME_WINDOW_DMG_S * 1000L);
constexpr static auto TIME_WINDOW_REST_S = long{20L};
constexpr static auto TIME_WINDOW_REST_MS = (TIME_WINDOW_REST_S * 1000L);
constexpr static auto REST_SKILL_ID = uint32_t{3087};
constexpr static auto REST_SKILL_REAPER_ID = uint32_t{3079U};
} // namespace

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static UwDhuumStats instance;
    return &instance;
}

void UwDhuumStats::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);
    WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"UwDhuumStats");

    /* Skill on self or party player_data */
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericValue>(
        &SkillCasted_Entry,
        [this](GW::HookStatus *, GW::Packet::StoC::GenericValue *packet) -> void {
            const uint32_t value_id = packet->value_id;
            const uint32_t caster_id = packet->agent_id;
            const uint32_t target_id = 0U;
            const uint32_t value = packet->value;
            const bool no_target = true;
            SkillPacketCallback(value_id, caster_id, target_id, value, no_target);
        });

    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericModifier>(
        &Damage_Entry,
        [this](GW::HookStatus *, GW::Packet::StoC::GenericModifier *packet) -> void {
            const uint32_t type = packet->type;
            const uint32_t caster_id = packet->cause_id;
            const uint32_t target_id = packet->target_id;
            const float value = packet->value;
            DamagePacketCallback(type, caster_id, target_id, value);
        });
}

void UwDhuumStats::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

void UwDhuumStats::SkillPacketCallback(const uint32_t value_id,
                                       const uint32_t caster_id,
                                       const uint32_t target_id,
                                       const uint32_t value,
                                       const bool no_target)
{
    auto agent_id = caster_id;
    const auto activated_skill_id = value;

    // ignore non-skill packets
    switch (value_id)
    {
    case GW::Packet::StoC::GenericValueID::instant_skill_activated:
    case GW::Packet::StoC::GenericValueID::skill_activated:
    case GW::Packet::StoC::GenericValueID::skill_finished:
    case GW::Packet::StoC::GenericValueID::attack_skill_activated:
    case GW::Packet::StoC::GenericValueID::attack_skill_finished:
    {
        if (!no_target)
            agent_id = target_id;
        break;
    }
    default:
        return;
    }

    if (REST_SKILL_ID == activated_skill_id || REST_SKILL_REAPER_ID == activated_skill_id)
    {
        ++num_casted_rest;
        rests.push_back(clock());
    }
}

void UwDhuumStats::DamagePacketCallback(const uint32_t type,
                                        const uint32_t caster_id,
                                        const uint32_t target_id,
                                        const float value)
{
    if (!dhuum_id || target_id != dhuum_id || value >= 0 || !caster_id)
        return;

    switch (type)
    {
    case GW::Packet::StoC::P156_Type::damage:
    case GW::Packet::StoC::P156_Type::critical:
    case GW::Packet::StoC::P156_Type::armorignoring:
        break;
    default:
        return;
    }

    const auto caster_agent = GW::Agents::GetAgentByID(caster_id);
    if (!caster_agent)
        return;
    const auto caster_living = caster_agent->GetAsAgentLiving();
    if (!caster_living)
        return;

    if (caster_living->allegiance != GW::Constants::Allegiance::Ally_NonAttackable &&
        caster_living->allegiance != GW::Constants::Allegiance::Npc_Minipet &&
        caster_living->allegiance != GW::Constants::Allegiance::Minion &&
        caster_living->allegiance != GW::Constants::Allegiance::Spirit_Pet)
        return;

    const auto target_agent = GW::Agents::GetAgentByID(target_id);
    if (!target_agent)
        return;
    const auto target_living = target_agent->GetAsAgentLiving();
    if (!target_living)
        return;

    const auto dmg_f32 = -value * target_living->max_hp;
    ++num_attacks;

    const auto time = clock();
    damages.push_back(std::make_pair(time, dmg_f32));
}

static void FormatTime(const uint64_t &duration, size_t bufsize, char *buf)
{
    const auto msecs = std::chrono::milliseconds(duration);
    const auto secs = std::chrono::duration_cast<std::chrono::seconds>(msecs).count() % 60;
    const auto mins = std::chrono::duration_cast<std::chrono::minutes>(msecs).count() % 60;
    const auto hrs = std::chrono::duration_cast<std::chrono::hours>(msecs).count() % 60;
    if (hrs)
        snprintf(buf, bufsize, "%02d:%02d:%02llu", hrs, mins, secs);
    else
        snprintf(buf, bufsize, "%02d:%02llu", mins, secs);
}

void UwDhuumStats::Draw(IDirect3DDevice9 *)
{
    static char buffer[16]{'\0'};
    static auto entered_dhuum_room_first = false;

    if (!player_data.ValidateData(UwHelperActivationConditions, false))
    {
        return;
    }

    if (uw_metadata.load_cb_triggered)
    {
        entered_dhuum_room_first = false;
    }

    ImGui::SetNextWindowSize(ImVec2(150.0F, 200.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(),
                     can_close && show_closebutton ? GetVisiblePtr() : nullptr,
                     GetWinFlags(ImGuiWindowFlags_NoResize)))
    {
        const auto is_in_dhuum_fight = IsInDhuumFight(player_data.pos);
        const auto entered_dhuum_room = IsInDhuumRoom(player_data.pos, GW::Constants::Range::Compass);
        const auto dhuum_fight_done = DhuumFightDone(uw_metadata.num_finished_objectives);

        if (!entered_dhuum_room_first && is_in_dhuum_fight)
        {
            entered_dhuum_room_first = true;
            GW::Chat::SendChat('/', "damage reset");
        }

        ImGui::Text("Dhuum HP: %3.0f%%", dhuum_hp * 100.0F);
        const auto timer_ms = TIMER_DIFF(dhuum_fight_start_time_ms);
        ImGui::Text("Timer: %4.0f", static_cast<float>(timer_ms) / 1000.0F);
        ImGui::Separator();
        ImGui::Text("Num Rests: %u", num_casted_rest);
        ImGui::Text("Rests (per s): %0.2f", rests_per_s);
        ImGui::Text("ETA Rest (s): %2.0f", num_casted_rest > 0 && is_in_dhuum_fight ? eta_rest_s : 0.0F);
        ImGui::Separator();
        ImGui::Text("Damage (per s): %0.0f", damage_per_s);
        ImGui::Text("ETA Damage (s): %3.0f", num_attacks > 0 && dhuum_hp > 0.25F ? eta_damage_s : 0.0F);
        ImGui::Separator();
        const auto instance_time_ms = GW::Map::GetInstanceTime();
        const auto max_time_ms = static_cast<uint64_t>(std::max(eta_rest_s * 1000.0F, eta_damage_s * 1000.0F));
        const auto finished_ms = static_cast<uint64_t>(instance_time_ms) + max_time_ms;
        if (IsUw() && entered_dhuum_room && dhuum_hp < 1.0F && !dhuum_fight_done)
        {
            std::memset(buffer, '\0', sizeof(char) * 16);
            FormatTime(finished_ms, 16, buffer);
            ImGui::Text("ETA: %s", buffer);
        }
        else if (IsUw() && entered_dhuum_room && dhuum_fight_done)
        {
            ImGui::Text("Finished: %s", buffer);
        }
        else
        {
            ImGui::Text("ETA: n/a");
        }
    }
    ImGui::End();
}

void UwDhuumStats::ResetData()
{
    dhuum_fight_start_time_ms = clock();

    num_casted_rest = 0U;
    rests_per_s = 0.0F;
    eta_rest_s = 0.0F;
    rests.clear();

    num_attacks = 0U;
    damage_per_s = 0.0F;
    eta_damage_s = 0.0F;
    damages.clear();
}

void UwDhuumStats::RemoveOldData()
{
    const auto remove_rest_it = std::remove_if(rests.begin(), rests.end(), [=](const auto t) {
        return std::abs(TIMER_DIFF(t)) > TIME_WINDOW_REST_MS;
    });
    rests.erase(remove_rest_it, rests.end());

    const auto remove_dmg_it = std::remove_if(damages.begin(), damages.end(), [=](const auto p) {
        return std::abs(TIMER_DIFF(p.first)) > TIME_WINDOW_DMG_MS;
    });
    damages.erase(remove_dmg_it, damages.end());
}

void UwDhuumStats::UpdateRestData()
{
    rests_per_s = 0.0F;

    const auto idx_rests = rests.size();
    if (idx_rests > 0)
        rests_per_s = static_cast<float>(idx_rests) / static_cast<float>(TIME_WINDOW_REST_S);
    else
        rests_per_s = 0.0F;

    progress_perc = GetProgressValue();
    const auto total_num_rests = num_casted_rest / (progress_perc + FLT_EPSILON);
    const auto still_needed_rest = total_num_rests - num_casted_rest;
    if (progress_perc >= 0.0F && progress_perc < 1.0F && still_needed_rest > 0 && rests_per_s > 0.0F)
        eta_rest_s = still_needed_rest / rests_per_s;
    else if (still_needed_rest == 0 || progress_perc >= 1.0F)
        eta_rest_s = 0.0F;
    else
        eta_rest_s = eta_rest_s;
}

void UwDhuumStats::UpdateDamageData()
{
    damage_per_s = 0.0F;
    const auto idx_dmg = damages.size();
    for (const auto [time, dmg] : damages)
        damage_per_s += dmg;

    if (idx_dmg > 0)
        damage_per_s /= static_cast<float>(TIME_WINDOW_DMG_S);
    else
        damage_per_s = 0.0F;
    damage_per_s = std::roundf(damage_per_s);

    const auto damage_to_be_done = (dhuum_max_hp * dhuum_hp) - (dhuum_max_hp * 0.25F);
    if (dhuum_hp >= 0.25F && dhuum_hp < 1.0F && damage_per_s > 0.0F)
        eta_damage_s = damage_to_be_done / damage_per_s;
    else
        eta_damage_s = 0.0F;
}

void UwDhuumStats::Update(float)
{
    if (!player_data.ValidateData(UwHelperActivationConditions, false))
    {
        return;
    }

    player_data.Update();

    const auto is_in_dhuum_room = IsInDhuumRoom(player_data.pos, GW::Constants::Range::Compass);
    if (!is_in_dhuum_room)
    {
        return;
    }

    const auto is_in_dhuum_fight = IsInDhuumFight(player_data.pos);
    if (!is_in_dhuum_fight)
    {
        ResetData();
    }

    const auto dhuum_agent = GetDhuumAgent();
    if (dhuum_agent)
    {
        GetDhuumAgentData(dhuum_agent, dhuum_hp, dhuum_max_hp);
    }
    if (dhuum_agent)
    {
        dhuum_id = dhuum_agent->agent_id;
    }

    RemoveOldData();
    UpdateRestData();
    UpdateDamageData();
}

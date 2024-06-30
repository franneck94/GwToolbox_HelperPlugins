#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Utilities/Hooker.h>

#include "ActionsBase.h"
#include "ActionsUw.h"
#include "HelperPlayer.h"
#include "DataSkillbar.h"
#include "HelperAgents.h"
#include "HelperMaps.h"
#include "HelperUw.h"
#include "HelperUwPos.h"
#include "Utils.h"
#include "UtilsGui.h"
#include "UtilsMath.h"

#include <SimpleIni.h>

#include "UwRanger.h"

namespace
{
constexpr auto HEALING_SPRING_U16 = static_cast<uint16_t>(GW::Constants::SkillID::Healing_Spring);
constexpr auto MAX_TABLE_LENGTH = 6U;
static auto auto_target_active = false;

static const auto T1_IDS = std::array<uint32_t, 1U>{GW::Constants::ModelID::UW::ColdfireNight};
static const auto T2_IDS = std::array<uint32_t, 1U>{GW::Constants::ModelID::UW::ObsidianBehemoth};
static const auto GENERAL_IDS = std::array<uint32_t, 4U>{GW::Constants::ModelID::UW::TerrorwebDryder,
                                                         GW::Constants::ModelID::UW::FourHorseman,
                                                         GW::Constants::ModelID::UW::SkeletonOfDhuum1,
                                                         GW::Constants::ModelID::UW::SkeletonOfDhuum2};
} // namespace

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static UwRanger instance;
    return &instance;
}

void UwRanger::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);
    GW::Initialize();

    WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"UwRanger");
}

void UwRanger::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

bool UwRanger::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void UwRanger::Terminate()
{
    ToolboxPlugin::Terminate();
}

void AutoTargetAction::Update()
{
    if (!IsExplorable())
    {
        auto_target_active = false;
        action_state = ActionState::INACTIVE;
    }

    if (action_state == ActionState::ACTIVE)
    {
        auto_target_active = true;

        if (behemoth_livings && behemoth_livings->size() == 0)
        {
            auto_target_active = false;
            action_state = ActionState::INACTIVE;
        }
    }
    else
    {
        auto_target_active = false;
        action_state = ActionState::INACTIVE;
    }
}

RoutineState AutoTargetAction::Routine()
{
    return RoutineState::NONE;
}

void UwRanger::DrawSettings()
{
    static auto _attack_at_auto_target = attack_at_auto_target;
    const auto width = ImGui::GetWindowWidth();

    ImGui::Text("Also attack Behemoths wile auto target is active:");
    ImGui::SameLine(width * 0.5F);
    ImGui::PushItemWidth(width * 0.5F);
    ImGui::Checkbox("###attackAtAutoTarget", &_attack_at_auto_target);
    ImGui::PopItemWidth();
    attack_at_auto_target = _attack_at_auto_target;
}

void UwRanger::LoadSettings(const wchar_t *folder)
{
    ToolboxUIPlugin::LoadSettings(folder);
    ini.LoadFile(GetSettingFile(folder).c_str());
    attack_at_auto_target = ini.GetBoolValue(Name(), VAR_NAME(attack_at_auto_target), attack_at_auto_target);
}

void UwRanger::SaveSettings(const wchar_t *folder)
{
    ToolboxUIPlugin::SaveSettings(folder);
    ini.SetBoolValue(Name(), VAR_NAME(attack_at_auto_target), attack_at_auto_target);
    PLUGIN_ASSERT(ini.SaveFile(GetSettingFile(folder).c_str()) == SI_OK);
}

void UwRanger::DrawSplittedAgents(std::vector<GW::AgentLiving *> livings,
                                  const ImVec4 color,
                                  std::string_view label,
                                  const bool draw_time)
{
    auto idx = uint32_t{0};

    for (const auto living : livings)
    {
        if (!living || living->hp == 0.0F || living->GetIsDead())
        {
            continue;
        }

        ImGui::TableNextRow();

        const auto player_pos = GetPlayerPos();
        if (living->player_number == static_cast<uint32_t>(GW::Constants::ModelID::UW::ObsidianBehemoth) &&
            living->GetIsCasting() && living->skill == HEALING_SPRING_U16)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1F, 0.9F, 0.1F, 1.0F));
            last_casted_times_ms[living->agent_id] = clock();
        }
        else if (!draw_time &&
                 living->player_number == static_cast<uint32_t>(GW::Constants::ModelID::UW::ColdfireNight) &&
                 (GW::GetDistance(player_pos, living->pos) > 1800.0F || living->GetIsAttacking()))
        {
            if (GW::GetDistance(player_pos, living->pos) > 1800.0F)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.0F, 0.0F, 1.0F));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1F, 0.9F, 0.1F, 1.0F));
            }
            last_casted_times_ms[living->agent_id] = clock();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, color);
        }

        const auto distance = GW::GetDistance(player_pos, living->pos);
        ImGui::TableNextColumn();
        ImGui::Text("%3.0f%%", living->hp * 100.0F);
        ImGui::TableNextColumn();
        ImGui::Text("%4.0f", distance);

        if (draw_time)
        {
            ImGui::TableNextColumn();
            const auto timer_diff_ms = TIMER_DIFF(last_casted_times_ms[living->agent_id]);
            const auto timer_diff_s = timer_diff_ms / 1000;
            if (timer_diff_s > 40 ||
                living->player_number != static_cast<uint32_t>(GW::Constants::ModelID::UW::ObsidianBehemoth))
            {
                ImGui::Text(" - ");
            }
            else
            {
                ImGui::Text("%2ds", timer_diff_s);
            }
        }

        ImGui::PopStyleColor();
        ImGui::TableNextColumn();
        const auto _label = std::format("Target##{}{}", label.data(), idx);
        if (ImGui::Button(_label.data()))
        {
            ChangeTarget(living->agent_id);
        }

        ++idx;

        if (idx >= MAX_TABLE_LENGTH)
        {
            break;
        }
    }
}

void UwRanger::Draw(IDirect3DDevice9 *)
{
    if (!ValidateData(UwHelperActivationConditions, true) || !IsRangerTerra())
        return;

    ImGui::SetNextWindowSize(ImVec2(200.0F, 240.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(),
                     can_close && show_closebutton ? GetVisiblePtr() : nullptr,
                     GetWinFlags(ImGuiWindowFlags_NoResize)))
    {
        const auto width = ImGui::GetWindowWidth();
        auto_target.Draw(ImVec2(width, 35.0F));

        if (ImGui::BeginTable("RangerTable", 4))
        {
            ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthFixed, width * 0.15F);
            ImGui::TableSetupColumn("Dist.", ImGuiTableColumnFlags_WidthFixed, width * 0.2F);
            ImGui::TableSetupColumn("Cast.", ImGuiTableColumnFlags_WidthFixed, width * 0.2F);
            ImGui::TableSetupColumn("Target", ImGuiTableColumnFlags_WidthFixed, width * 0.4F);

            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            ImGui::TableNextColumn();
            ImGui::Text("HP");
            ImGui::TableNextColumn();
            ImGui::Text("Dist.");
            ImGui::TableNextColumn();
            ImGui::Text("Cast.");
            ImGui::TableNextColumn();
            ImGui::Text("Target");

            DrawSplittedAgents(horseman_livings, ImVec4(0.568F, 0.239F, 1.0F, 1.0F), "Horseman");
            DrawSplittedAgents(coldfire_livings, ImVec4(0.7F, 0.7F, 1.0F, 1.0F), "Coldfire");
            DrawSplittedAgents(behemoth_livings, ImVec4(1.0F, 1.0F, 1.0F, 1.0F), "Behemoth");
            DrawSplittedAgents(dryder_livings, ImVec4(0.94F, 0.31F, 0.09F, 1.0F), "Dryder");
            DrawSplittedAgents(skele_livings, ImVec4(0.1F, 0.8F, 0.9F, 1.0F), "Skele");
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void UwRanger::Update(float)
{
    filtered_livings.clear();
    coldfire_livings.clear();
    behemoth_livings.clear();
    dryder_livings.clear();
    skele_livings.clear();
    horseman_livings.clear();

    if (!ValidateData(UwHelperActivationConditions, true))
    {
        last_casted_times_ms.clear();
        return;
    }

    livings_data.Update();

    if (!IsRangerTerra() || IsLoading())
    {
        last_casted_times_ms.clear();
        return;
    }

    const auto player_pos = GetPlayerPos();
    const auto &pos = player_pos;
    FilterByIdsAndDistances(pos, livings_data.enemies, filtered_livings, T1_IDS, GW::Constants::Range::Compass);
    FilterByIdsAndDistances(pos, livings_data.enemies, filtered_livings, T2_IDS, 800.0F);
    FilterByIdsAndDistances(pos, livings_data.enemies, filtered_livings, GENERAL_IDS, 1500.0F);
    FilterByIdAndDistance(pos, filtered_livings, coldfire_livings, GW::Constants::ModelID::UW::ColdfireNight);
    FilterByIdAndDistance(pos, filtered_livings, behemoth_livings, GW::Constants::ModelID::UW::ObsidianBehemoth);
    FilterByIdAndDistance(pos, filtered_livings, dryder_livings, GW::Constants::ModelID::UW::TerrorwebDryder);
    FilterByIdAndDistance(pos, filtered_livings, skele_livings, GW::Constants::ModelID::UW::SkeletonOfDhuum1);
    FilterByIdAndDistance(pos, filtered_livings, skele_livings, GW::Constants::ModelID::UW::SkeletonOfDhuum2);
    FilterByIdAndDistance(pos, filtered_livings, horseman_livings, GW::Constants::ModelID::UW::FourHorseman);
    SortByDistance(coldfire_livings);
    SortByDistance(behemoth_livings);
    SortByDistance(dryder_livings);
    SortByDistance(skele_livings);
    SortByDistance(horseman_livings);

    auto_target.behemoth_livings = &behemoth_livings;
    auto_target.Update();

    if (!auto_target_active)
        return;

    const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
    if (!me_living)
        return;

    for (const auto living : behemoth_livings)
    {
        if (!living)
        {
            continue;
        }

        const auto dist = GW::GetDistance(player_pos, living->pos);
        if (dist < GW::Constants::Range::Earshot && living->GetIsCasting() && living->skill == HEALING_SPRING_U16)
        {
            ChangeTarget(living->agent_id);
            if (attack_at_auto_target && (!me_living->GetIsMoving() && !me_living->GetIsCasting()) &&
                GW::Agents::GetTarget())
            {
                AttackAgent(GW::Agents::GetTarget());
            }
        }
    }
}

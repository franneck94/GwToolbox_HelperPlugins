#include <cstdint>
#include <filesystem>

#include <GWCA/GWCA.h>
#include <GWCA/Managers/AgentMgr.h>

#include "AutoFollow.h"
#include "Helper.h"

#include <imgui.h>

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static AutoFollow instance;
    return &instance;
}

void AutoFollow::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);
}

void AutoFollow::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

void AutoFollowAction::Update()
{
    if (action_state == ActionState::ACTIVE)
    {
        const auto routine_state = Routine();

        if (routine_state == RoutineState::FINISHED)
        {
            action_state = ActionState::INACTIVE;
        }
    }
}

RoutineState AutoFollowAction::Routine()
{
    if (!player_data || !player_data->target || !player_data->target->GetIsLivingType())
    {
        return RoutineState::FINISHED;
    }

    if (player_data->living->GetIsMoving())
    {
        return RoutineState::ACTIVE;
    }
    GW::Agents::GoPlayer(player_data->target);

    return RoutineState::ACTIVE;
}

void AutoFollow::Draw(IDirect3DDevice9 *)
{
    if (!player_data.ValidateData(HelperActivationConditions, false))
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(125.0F, 50.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(),
                     can_close && show_closebutton ? GetVisiblePtr() : nullptr,
                     GetWinFlags() | ImGuiWindowFlags_NoDecoration))
    {
        const auto width = ImGui::GetWindowWidth();
        auto_follow.Draw(ImVec2(width, 35.0F), true);
    }
    ImGui::End();
}

void AutoFollow::Update(float)
{
    if (!player_data.ValidateData(HelperActivationConditions, false))
    {
        return;
    }

    player_data.Update();
    auto_follow.Update();
}

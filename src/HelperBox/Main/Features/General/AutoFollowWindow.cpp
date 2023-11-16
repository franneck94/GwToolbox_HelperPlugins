#include <array>
#include <cstdint>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/AgentMgr.h>

#include <ActionsBase.h>
#include <Base/HelperBox.h>
#include <DataPlayer.h>
#include <Helper.h>
#include <HelperUw.h>
#include <UtilsGui.h>
#include <UtilsMath.h>

#include <imgui.h>

#include "AutoFollowWindow.h"

void AutoFollowAction::Update()
{
    if (action_state == ActionState::ACTIVE)
    {
        const auto routine_state = Routine();

        if (routine_state == RoutineState::FINISHED)
            action_state = ActionState::INACTIVE;
    }
}

RoutineState AutoFollowAction::Routine()
{
    if (!player_data || !player_data->target || !player_data->target->GetIsLivingType())
        return RoutineState::FINISHED;

    if (player_data->living->GetIsMoving())
        return RoutineState::ACTIVE;

    GW::Agents::GoPlayer(player_data->target);

    return RoutineState::ACTIVE;
}

void AutoFollowWindow::Draw()
{
    if (!visible)
        return;

    if (!player_data.ValidateData(HelperActivationConditions, false))
        return;

    ImGui::SetNextWindowSize(ImVec2(125.0F, 50.0F), ImGuiCond_FirstUseEver);

    if (ImGui::Begin(Name(), nullptr, GetWinFlags() | ImGuiWindowFlags_NoDecoration))
    {
        const auto width = ImGui::GetWindowWidth();
        auto_follow.Draw(ImVec2(width, 35.0F));
    }
    ImGui::End();
}

void AutoFollowWindow::Update(float, const AgentLivingData &)
{
    if (!player_data.ValidateData(HelperActivationConditions, false))
        return;
    player_data.Update();

    auto_follow.Update();
}

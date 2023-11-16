#include <array>
#include <cstdint>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/AgentMgr.h>

#include "ActionsBase.h"
#include <Base/HelperBox.h>
#include "DataPlayer.h"
#include <DataSkill.h>
#include "Helper.h"
#include "HelperAgents.h"
#include "HelperUw.h"
#include "UtilsGui.h"
#include "UtilsMath.h"

#include <imgui.h>

#include "MimicWindow.h"

void MimicWindowAction::Update()
{
    if (action_state == ActionState::ACTIVE)
    {
        const auto routine_state = Routine();

        if (routine_state == RoutineState::FINISHED)
            action_state = ActionState::INACTIVE;
    }
}

RoutineState MimicWindowAction::Routine()
{
    if (!player_data || !player_data->target || !player_data->target->GetIsLivingType())
        return RoutineState::FINISHED;

    const auto target_living = player_data->target->GetAsAgentLiving();
    if (player_data->living->GetIsMoving() || target_living->GetIsMoving() || player_data->living->GetIsCasting())
        return RoutineState::ACTIVE;

    const auto enemies_in_aggro =
        FilterAgentsByRange(_livings_data->enemies, *player_data, GW::Constants::Range::Spellcast);
    const auto &[closest_agent, dist] = GetClosestEnemy(player_data, enemies_in_aggro);
    if (!closest_agent || dist > GW::Constants::Range::Spellcast)
        return RoutineState::ACTIVE;

    const auto skillbar_ = GW::SkillbarMgr::GetPlayerSkillbar();
    if (!skillbar_)
        return RoutineState::ACTIVE;

    auto skill_1 = DataSkill{(GW::Constants::SkillID)skillbar_->skills[0].skill_id, 0};
    skill_1.Update(skillbar_->skills);
    if (skill_1.CanBeCasted(player_data->energy))
    {
        skill_1.Cast(player_data->energy, closest_agent->agent_id);
        return RoutineState::ACTIVE;
    }

    auto skill_2 = DataSkill{(GW::Constants::SkillID)skillbar_->skills[0].skill_id, 1};
    skill_2.Update(skillbar_->skills);
    if (skill_2.CanBeCasted(player_data->energy))
    {
        skill_2.Cast(player_data->energy, closest_agent->agent_id);
        return RoutineState::ACTIVE;
    }

    auto skill_3 = DataSkill{(GW::Constants::SkillID)skillbar_->skills[0].skill_id, 2};
    skill_3.Update(skillbar_->skills);
    if (skill_3.CanBeCasted(player_data->energy))
    {
        skill_3.Cast(player_data->energy, closest_agent->agent_id);
        return RoutineState::ACTIVE;
    }

    return RoutineState::ACTIVE;
}

void MimicWindow::Draw()
{
    if (!visible)
        return;

    if (!player_data.ValidateData(HelperActivationConditions, false))
        return;

    ImGui::SetNextWindowSize(ImVec2(125.0F, 50.0F), ImGuiCond_FirstUseEver);

    if (ImGui::Begin(Name(), nullptr, GetWinFlags() | ImGuiWindowFlags_NoDecoration))
    {
        const auto width = ImGui::GetWindowWidth();
        mimic.Draw(ImVec2(width, 35.0F));
    }
    ImGui::End();
}

void MimicWindow::Update(float, const AgentLivingData &_livings_data)
{
    if (!player_data.ValidateData(HelperActivationConditions, false))
        return;
    player_data.Update();

    mimic._livings_data = &_livings_data;
    mimic.Update();
}

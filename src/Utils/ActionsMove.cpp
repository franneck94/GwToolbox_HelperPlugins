#include <algorithm>
#include <random>
#include <set>

#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/ChatMgr.h>

#include "ActionsMove.h"
#include "DataLivings.h"
#include "DataPlayer.h"
#include "Helper.h"
#include "HelperAgents.h"
#include "HelperUw.h"
#include "HelperUwPos.h"
#include "Logger.h"
#include "Utils.h"

void MoveABC::Execute() const
{
    static auto gen = std::mt19937{};
    static auto dist = std::uniform_real_distribution<float>(-0.05F, 0.05F);

    const auto me = GW::Agents::GetPlayer();
    if (!CanMove() || !me)
        return;

    if (GW::GetDistance(me->pos, pos) > 5000.0F)
    {
        Log::Info("Too far away!");
        return;
    }

    if (x != 0.0F && y != 0.0F)
    {
        const auto eps = dist(gen);
        GW::Agents::Move(x + eps, y + eps);
        Log::Info("%s - Moving to (%.0f, %.0f)", name.data(), x, y);
    }

    if (cb_fn.has_value())
        cb_fn.value()();
}

bool Move_CastSkillABC::UpdateMoveState(const DataPlayer &player_data, const AgentLivingData *, bool &move_ongoing)
{
    static auto started_cast = false;
    static auto timer_wait = clock();
    static auto timer_casting = clock();

    const auto skill_data = GW::SkillbarMgr::GetSkillConstantData(skill_cb->id);
    if (!skill_data)
        return true;
    const auto cast_time_s = (skill_data->activation + skill_data->aftercast) * 1000.0F;
    const auto timer_diff_wait = TIMER_DIFF(timer_wait);

    move_ongoing = true;

    if (player_data.living->GetIsMoving())
        timer_wait = clock();

    const auto reached_pos = GamePosCompare(player_data.pos, pos, 0.1F);
    if (!reached_pos)
        return false;

    const auto cast_cannot_start = (!player_data.CanCast() || timer_diff_wait < 600 || skill_cb->recharge > 0);
    if (!started_cast && cast_cannot_start)
        return false;

    if (!started_cast && RoutineState::FINISHED == skill_cb->Cast(player_data.energy))
    {
        timer_casting = clock();
        started_cast = true;
        return false;
    }

    const auto is_timer_exceeded = timer_diff_wait > cast_time_s;
    const auto timer_diff_casting = TIMER_DIFF(timer_casting);
    const auto is_casting = player_data.living->GetIsCasting() || TIMER_DIFF(timer_diff_casting) < 500;
    if (started_cast && (!is_timer_exceeded || is_casting || is_casting))
        return false;

    timer_casting = clock();
    started_cast = false;
    return true;
}

bool Move_WaitABC::UpdateMoveState(const DataPlayer &player_data,
                                   const AgentLivingData *livings_data,
                                   bool &move_ongoing)
{
    move_ongoing = true;

    static auto canceled_move = false;
    const auto aggro_free = CheckForAggroFree(player_data, livings_data, pos);
    if (aggro_free)
    {
        Log::Info("Aggro free, moving on");
        canceled_move = false;
        return true;
    }

    if (!canceled_move)
    {
        canceled_move = true;
        Log::Info("Canceled Movement based on aggro");
        CancelMovement();
        return false;
    }

    return false;
}

bool Move_DistanceABC::UpdateMoveState(const DataPlayer &player_data, const AgentLivingData *, bool &move_ongoing)
{
    move_ongoing = true;

    const auto trigger_id = GetUwTriggerRoleId(TriggerRole::LT);
    if (!trigger_id)
        return false;

    const auto trigger_agent = GW::Agents::GetAgentByID(trigger_id);
    if (!trigger_agent)
        return false;

    const auto dist = GW::GetDistance(player_data.pos, trigger_agent->pos);
    if (dist < dist_threshold)
        return false;

    Log::Info("Distance reached: %f", dist);
    return true;
}

bool Move_NoWaitABC::UpdateMoveState(const DataPlayer &, const AgentLivingData *, bool &move_ongoing)
{
    move_ongoing = true;
    return true;
}

bool Move_PositionABC::UpdateMoveState(const DataPlayer &, const AgentLivingData *, bool &move_ongoing)
{
    move_ongoing = true;

    const auto trigger_id = GetUwTriggerRoleId(role);
    if (!trigger_id)
        return false;

    const auto trigger_agent = GW::Agents::GetAgentByID(trigger_id);
    if (!trigger_agent)
        return false;

    if (IsNearToGamePos(trigger_pos, trigger_agent->pos, trigger_threshold))
        return true;

    return false;
}

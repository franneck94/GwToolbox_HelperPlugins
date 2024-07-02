#pragma once

#include <functional>
#include <map>
#include <optional>
#include <ranges>
#include <string>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/PartyMgr.h>

#include "ActionsBase.h"
#include "DataLivings.h"
#include "HelperPlayer.h"
#include "DataSkillbar.h"
#include "Helper.h"
#include "Logger.h"
#include "Utils.h"
#include "UtilsMath.h"

class TriggerABC
{
public:
    TriggerABC(const bool _is_proceeding_move) noexcept : is_proceeding_move(_is_proceeding_move) {};
    bool is_proceeding_move;
};

class MoveABC : public TriggerABC
{
public:
    MoveABC(const float _x,
            const float _y,
            std::string_view _name,
            const bool _is_proceeding_move,
            std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : x(_x), y(_y), pos({x, y, 0}), name(_name), cb_fn(_cb_fn), TriggerABC(_is_proceeding_move) {};
    virtual ~MoveABC() noexcept {};

    const char *Name() const noexcept
    {
        return name.data();
    }

    void Execute() const;

    virtual void ReTriggerMessage() const
    {
        Log::Info("Retrigger current move: %s", name.data());
    };
    virtual void TriggerMessage() const
    {
        Log::Info("Ongoing to next move: %s", name.data());
    };
    virtual void NoTriggerMessage() const
    {
        Log::Info("Waiting...");
    };

    virtual bool UpdateMoveState(const AgentLivingData *livings_data, bool &move_ongoing) = 0;

    template <uint32_t N>
    static void SkipNonFullteamMoves(const bool is_fullteam_lt,
                                     const std::array<uint32_t, N> &full_team_moves,
                                     const uint32_t num_moves,
                                     uint32_t &move_idx)
    {
        static auto last_move_idx = 0U;

        if (!is_fullteam_lt)
            return;

        if (move_idx < full_team_moves[0])
            move_idx = full_team_moves[0];

        while (true)
        {
            const auto curr_move_is_full_team_it = std::find_if(full_team_moves.begin(),
                                                                full_team_moves.end(),
                                                                [move_idx](const auto idx) { return idx == move_idx; });
            if (curr_move_is_full_team_it == full_team_moves.end())
            {
                if (move_idx < num_moves - 2U && move_idx > 1)
                {
                    if (last_move_idx > move_idx)
                    {
                        if (move_idx - 1 > 0)
                            --move_idx;
                    }
                    else
                    {
                        if (move_idx + 1 <= num_moves)
                            ++move_idx;
                    }
                }
            }
            else
            {
                break;
            }
        }

        last_move_idx = move_idx;
    }

    template <uint32_t N>
    static void UpdatedUwMoves(const AgentLivingData *livings_data,
                               const std::array<MoveABC *, N> &moves,
                               uint32_t &move_idx,
                               bool &move_ongoing)
    {
        static auto trigger_timer_ms = clock();
        static auto already_reached_pos = false;

        if (!move_ongoing)
            return;

        if (move_idx >= moves.size() - 1U)
            return;

        const auto can_be_finished = moves[move_idx]->UpdateMoveState(livings_data, move_ongoing);

        const auto me_living = GW::Agents::GetPlayerAsAgentLiving();
        if (!me_living)
            return;

        const auto is_moving = me_living->GetIsMoving();
        const auto player_pos = GetPlayerPos();
        const auto reached_pos = GamePosCompare(player_pos, moves[move_idx]->pos, 0.1F);
        if (!already_reached_pos && reached_pos)
            already_reached_pos = true;

        if (!already_reached_pos && is_moving)
            return;

        if (!already_reached_pos && !is_moving && can_be_finished)
        {
            static auto last_trigger_time_ms = clock();

            const auto last_trigger_time_diff_ms = TIMER_DIFF(last_trigger_time_ms);
            if (last_trigger_time_diff_ms == 0 || last_trigger_time_diff_ms >= MoveABC::TRIGGER_TIMER_THRESHOLD_MS)
            {
                last_trigger_time_ms = clock();
                moves[move_idx]->ReTriggerMessage();
                moves[move_idx]->Execute();
                trigger_timer_ms = clock();
            }
            return;
        }

        if (already_reached_pos &&
            (!moves[move_idx]->is_casting_action || (can_be_finished && moves[move_idx]->is_casting_action)))
        {
            const auto last_trigger_timer_diff = TIMER_DIFF(trigger_timer_ms);
            if (last_trigger_timer_diff < MoveABC::TRIGGER_TIMER_THRESHOLD_MS)
                return;
            trigger_timer_ms = clock();

            already_reached_pos = false;
            move_ongoing = false;
            const auto proceed = moves[move_idx]->is_proceeding_move;
            ++move_idx;
            const auto trigger = !moves[move_idx]->is_distance_based;
            if (proceed)
            {
                move_ongoing = true;
                if (trigger)
                {
                    moves[move_idx]->TriggerMessage();
                    moves[move_idx]->Execute();
                }
                else
                {
                    moves[move_idx]->NoTriggerMessage();
                }
            }
        }
    }

    template <uint32_t N>
    static uint32_t GetFirstCloseMove(const std::array<MoveABC *, N> &moves)
    {
        auto idx = 0U;

        auto reversed_moves = std::ranges::reverse_view{moves};
        const auto player_pos = GetPlayerPos();
        for (const auto move : reversed_moves)
        {
            const auto dist_to_move = GW::GetDistance(player_pos, move->pos);
            if (dist_to_move < GW::Constants::Range::Spellcast)
                return idx;

            ++idx;
        }

        return 0U;
    }

    static void LtMoveTrigger(bool &ready,
                              bool &ongoing,
                              const bool trigger_take,
                              const bool trigger_move,
                              const bool is_moving,
                              const MoveABC *move)
    {
        if (!move)
            return;

        if (ready && (trigger_move || trigger_take))
        {
            move->Execute();
            Log::Info("Starting action %s", move->name.data());
            if (trigger_take)
            {
                ready = false;
                ongoing = true;
            }
            else if (trigger_move && is_moving)
            {
                ready = false;
                ongoing = true;
            }
        }
        else if (ready && ongoing)
            ready = false;
        else if (ready && !trigger_move && !trigger_take)
            ready = false;
    }

private:
    float x = 0.0F;
    float y = 0.0F;

public:
    GW::GamePos pos;
    std::string name;
    std::optional<std::function<bool()>> cb_fn = std::nullopt;
    bool is_distance_based = false;
    bool is_casting_action = false;

    static constexpr long TRIGGER_TIMER_THRESHOLD_MS = 500L;
};

class Move_NoWaitABC : public MoveABC
{
public:
    Move_NoWaitABC(const float _x,
                   const float _y,
                   const std::string &_name,
                   const bool _is_proceeding_move,
                   std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : MoveABC(_x, _y, _name, _is_proceeding_move, _cb_fn) {};

    bool UpdateMoveState(const AgentLivingData *livings_data, bool &move_ongoing) override;
};

class Move_NoWaitAndContinue : public Move_NoWaitABC
{
public:
    Move_NoWaitAndContinue(const float _x,
                           const float _y,
                           const std::string &_name,
                           std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : Move_NoWaitABC(_x, _y, _name, true, _cb_fn) {};
};

class Move_NoWaitAndStop : public Move_NoWaitABC
{
public:
    Move_NoWaitAndStop(const float _x,
                       const float _y,
                       const std::string &_name,
                       std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : Move_NoWaitABC(_x, _y, _name, false, _cb_fn) {};
};

class Move_WaitABC : public MoveABC
{
public:
    Move_WaitABC(const float _x,
                 const float _y,
                 const std::string &_name,
                 const bool _is_proceeding_move,
                 std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : MoveABC(_x, _y, _name, _is_proceeding_move, _cb_fn) {};

    bool UpdateMoveState(const AgentLivingData *livings_data, bool &move_ongoing) override;
};

class Move_WaitAndContinue : public Move_WaitABC
{
public:
    Move_WaitAndContinue(const float _x,
                         const float _y,
                         const std::string &_name,
                         std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : Move_WaitABC(_x, _y, _name, true, _cb_fn) {};
};

class Move_WaitAndStop : public Move_WaitABC
{
public:
    Move_WaitAndStop(const float _x,
                     const float _y,
                     const std::string &_name,
                     std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : Move_WaitABC(_x, _y, _name, false, _cb_fn) {};
};

class Move_CastSkillABC : public MoveABC
{
public:
    Move_CastSkillABC(const float _x,
                      const float _y,
                      const std::string &_name,
                      const bool _is_proceeding_move,
                      const DataSkill *_skill_cb,
                      std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : MoveABC(_x, _y, _name, _is_proceeding_move, _cb_fn), skill_cb(_skill_cb)
    {
        is_casting_action = true;
    };

    bool UpdateMoveState(const AgentLivingData *livings_data, bool &move_ongoing) override;

    const DataSkill *skill_cb = nullptr;
};

class Move_CastSkillAndContinue : public Move_CastSkillABC
{
public:
    Move_CastSkillAndContinue(const float _x,
                              const float _y,
                              const std::string &_name,
                              const DataSkill *_skill_cb,
                              std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : Move_CastSkillABC(_x, _y, _name, true, _skill_cb, _cb_fn) {};
};

class Move_CastSkillAndStop : public Move_CastSkillABC
{
public:
    Move_CastSkillAndStop(const float _x,
                          const float _y,
                          const std::string &_name,
                          const DataSkill *_skill_cb,
                          std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : Move_CastSkillABC(_x, _y, _name, false, _skill_cb, _cb_fn) {};
};

class Move_DistanceABC : public MoveABC
{
public:
    Move_DistanceABC(const float _x,
                     const float _y,
                     const std::string &_name,
                     const bool _is_proceeding_move,
                     const float _dist_threshold,
                     std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : MoveABC(_x, _y, _name, _is_proceeding_move, _cb_fn), dist_threshold(_dist_threshold)
    {
        is_distance_based = true;
    };

    bool UpdateMoveState(const AgentLivingData *livings_data, bool &move_ongoing) override;

    void NoTriggerMessage() const override
    {
        Log::Info("Waiting for distance %f", dist_threshold);
    }

    float dist_threshold;
};

class Move_DistanceAndContinue : public Move_DistanceABC
{
public:
    Move_DistanceAndContinue(const float _x,
                             const float _y,
                             const std::string &_name,
                             const float _dist_threshold,
                             std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : Move_DistanceABC(_x, _y, _name, true, _dist_threshold, _cb_fn) {};
};

class Move_DistanceAndStop : public Move_DistanceABC
{
public:
    Move_DistanceAndStop(const float _x,
                         const float _y,
                         const std::string &_name,
                         const float _dist_threshold,
                         std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : Move_DistanceABC(_x, _y, _name, false, _dist_threshold, _cb_fn) {};
};

class Move_PositionABC : public MoveABC
{
public:
    Move_PositionABC(const float _x,
                     const float _y,
                     const std::string &_name,
                     const bool _is_proceeding_move,
                     const GW::GamePos &_trigger_pos,
                     const float _trigger_threshold,
                     const TriggerRole _role,
                     std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : MoveABC(_x, _y, _name, _is_proceeding_move, _cb_fn), trigger_pos(_trigger_pos),
          trigger_threshold(_trigger_threshold), role(_role)
    {
        is_distance_based = true;
    };

    bool UpdateMoveState(const AgentLivingData *livings_data, bool &move_ongoing) override;

    void NoTriggerMessage() const override
    {
        Log::Info("Waiting for position (%0.2f, %0.2f) with threshold %0.2f",
                  trigger_pos.x,
                  trigger_pos.y,
                  trigger_threshold);
    };

    GW::GamePos trigger_pos;
    TriggerRole role;
    float trigger_threshold;
};

class Move_PositionAndContinue : public Move_PositionABC
{
public:
    Move_PositionAndContinue(const float _x,
                             const float _y,
                             const std::string &_name,
                             const GW::GamePos &_trigger_pos,
                             const float _trigger_threshold,
                             const TriggerRole _role,
                             std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : Move_PositionABC(_x, _y, _name, true, _trigger_pos, _trigger_threshold, _role, _cb_fn) {};
};

class Move_PositionAndStop : public Move_PositionABC
{
public:
    Move_PositionAndStop(const float _x,
                         const float _y,
                         const std::string &_name,
                         const GW::GamePos &_trigger_pos,
                         const float _trigger_threshold,
                         const TriggerRole _role,
                         std::optional<std::function<bool()>> _cb_fn = std::nullopt)
        : Move_PositionABC(_x, _y, _name, false, _trigger_pos, _trigger_threshold, _role, _cb_fn) {};
};

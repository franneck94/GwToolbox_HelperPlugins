#pragma once

enum class ActionState
{
    INACTIVE = 0,
    ACTIVE = 1,
    ON_HOLD = 2,
};

ActionState StateNegation(const ActionState state);

void StateOnActive(ActionState &state);

void StateOnHold(ActionState &state);

enum class RoutineState
{
    NONE = 0,
    ACTIVE = 1,
    FINISHED = 2
};

void ResetState(RoutineState &state);

enum class TriggerRole
{
    LT,
    EMO,
    DB,
};

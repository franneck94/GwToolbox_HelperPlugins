#pragma once

#include <cstdint>
#include <string_view>

#include "ActionTypes.h"
#include "DataPlayer.h"

#include <imgui.h>

class ActionABC
{
public:
    constexpr static auto TIMER_THRESHOLD_MS = long{200U};

    static bool HasWaitedLongEnough(long timer_threshold_ms = TIMER_THRESHOLD_MS);

    ActionABC(std::string_view _text) noexcept : text(_text) {};
    virtual ~ActionABC() noexcept {};

    void Draw(ImVec2 button_size = ImVec2(100.0, 50.0), bool allow_in_outpost = false);
    virtual RoutineState Routine() = 0;
    virtual void Update() = 0;

    virtual bool PauseRoutine() noexcept
    {
        return false;
    }
    bool ResumeRoutine() noexcept
    {
        return !PauseRoutine();
    }

    std::string_view text;

    ActionState action_state = ActionState::INACTIVE;
};

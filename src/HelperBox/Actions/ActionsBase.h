#pragma once

#include <cstdint>
#include <string_view>

#include <ActionTypes.h>
#include <DataPlayer.h>
#include <UtilsGui.h>

#include <imgui.h>

class ActionABC
{
public:
    constexpr static auto TIMER_THRESHOLD_MS = long{200U};

    static bool HasWaitedLongEnough(const long timer_threshold_ms = TIMER_THRESHOLD_MS);

    ActionABC(DataPlayer *p, std::string_view t) noexcept : player_data(p), text(t){};
    virtual ~ActionABC() noexcept {};

    void Draw(const ImVec2 button_size = ImVec2(100.0, 50.0));
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

    DataPlayer *player_data;
    std::string_view text;

    ActionState action_state = ActionState::INACTIVE;
};

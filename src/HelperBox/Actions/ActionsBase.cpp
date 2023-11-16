#include <cstdint>
#include <map>

#include <HelperMaps.h>
#include <Utils.h>
#include <UtilsGui.h>

#include "ActionsBase.h"

namespace
{
static const auto ACTIVE_COLOR = ImVec4{0.0F, 200.0F, 0.0F, 80.0F};
static const auto INACTIVE_COLOR = ImVec4{41.0F, 74.0F, 122.0F, 80.0F};
static const auto ON_HOLD_COLOR = ImVec4{255.0F, 226.0F, 0.0F, 80.0F};

static auto COLOR_MAPPING = std::map<uint32_t, ImVec4>{{static_cast<uint32_t>(ActionState::INACTIVE), INACTIVE_COLOR},
                                                       {static_cast<uint32_t>(ActionState::ACTIVE), ACTIVE_COLOR},
                                                       {static_cast<uint32_t>(ActionState::ON_HOLD), ON_HOLD_COLOR}};

}; // namespace

void ActionABC::Draw(const ImVec2 button_size)
{
    if (!IsExplorable())
        action_state = ActionState::INACTIVE;

    const auto &color = COLOR_MAPPING[static_cast<uint32_t>(action_state)];
    DrawButton(action_state, color, text, button_size);
}

bool ActionABC::HasWaitedLongEnough(const long timer_threshold_ms)
{
    static auto timer_last_cast_ms = clock();

    const auto last_cast_diff_ms = TIMER_DIFF(timer_last_cast_ms);
    if (last_cast_diff_ms < timer_threshold_ms)
        return false;

    timer_last_cast_ms = clock();
    return true;
}

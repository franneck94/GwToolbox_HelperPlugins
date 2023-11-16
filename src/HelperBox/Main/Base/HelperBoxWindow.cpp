#include <Base/HelperBoxWindow.h>

#include <imgui.h>

#include "HelperBoxWindow.h"

ImGuiWindowFlags HelperBoxWindow::GetWinFlags(ImGuiWindowFlags flags) const
{
    if (lock_move)
        flags |= ImGuiWindowFlags_NoMove;
    if (lock_size)
        flags |= ImGuiWindowFlags_NoResize;

    flags |= ImGuiWindowFlags_NoScrollbar;

    return flags;
}

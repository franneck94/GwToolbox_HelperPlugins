#include <cstdint>

#include <Base/HelperBox.h>

#include <imgui.h>

#include "MainWindow.h"

void MainWindow::LoadSettings(CSimpleIni *ini)
{
    HelperBoxWindow::LoadSettings(ini);
    pending_refresh_buttons = true;
}

void MainWindow::SaveSettings(CSimpleIni *ini)
{
    HelperBoxWindow::SaveSettings(ini);
}

void MainWindow::RefreshButtons()
{
    pending_refresh_buttons = false;
    const auto &ui = HelperBox::Instance().GetUIElements();
    modules_to_draw.clear();
    for (auto &ui_module : ui)
    {
        const auto weighting = 1.0F;
        auto it = modules_to_draw.begin();
        for (it = modules_to_draw.begin(); it != modules_to_draw.end(); it++)
        {
            if (it->first > weighting)
                break;
        }
        modules_to_draw.insert(it, {weighting, ui_module});
    }
}

void MainWindow::Draw()
{
    if (!visible)
        return;

    if (pending_refresh_buttons)
        RefreshButtons();

    static bool open = true;
    ImGui::SetNextWindowSize(ImVec2(110.0f, 300.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(), show_closebutton ? &open : nullptr, GetWinFlags()))
    {
        auto drawn = false;
        const auto msize = modules_to_draw.size();
        for (size_t i = 0; i < msize; i++)
        {
            ImGui::PushID(static_cast<int>(i));
            if (drawn)
                ImGui::Separator();

            drawn = true;
            auto &ui_module = modules_to_draw[i].second;
            ui_module->DrawTabButton();
            ImGui::PopID();
        }
    }
    ImGui::End();

    if (!open)
        HelperBox::Instance().StartSelfDestruct();
}

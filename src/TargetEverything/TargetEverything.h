#pragma once

#include <IconsFontAwesome5.h>
#include <ToolboxPlugin.h>

class TargetEverything : public ToolboxPlugin {
public:
    const char* Name() const override { return "TargetEverything"; }
    const char* Icon() const override { return ICON_FA_CROSSHAIRS; }

    void Initialize(ImGuiContext*, ImGuiAllocFns, HMODULE) override;
    void SignalTerminate() override;
    bool CanTerminate() override;
    void Terminate() override;
};

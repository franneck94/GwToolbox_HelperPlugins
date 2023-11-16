#pragma once

#include <ToolboxPlugin.h>

class LongFinger : public ToolboxPlugin {
public:
    const char* Name() const override { return "LongFinger"; }
    const char* Icon() const override { return ICON_FA_LOCK_OPEN; }

    void Initialize(ImGuiContext*, ImGuiAllocFns, HMODULE) override;
    void SignalTerminate() override;
    bool CanTerminate() override;
    void Terminate() override;
};

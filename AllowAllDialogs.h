#pragma once

#include <ToolboxPlugin.h>

class AllowAllDialogs : public ToolboxPlugin {
public:
    const char* Name() const override { return "Allow All Dialogs"; }

    void Initialize(ImGuiContext*, ImGuiAllocFns, HMODULE) override;
    void SignalTerminate();
    bool CanTerminate();
    void Terminate() override;
};

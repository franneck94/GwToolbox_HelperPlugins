#pragma once

#include <ToolboxPlugin.h>

class AllowAllDialogs : public ToolboxPlugin
{
public:
    [[nodiscard]] const char *Name() const override
    {
        return "Allow All Dialogs";
    }

    void Initialize(ImGuiContext *, ImGuiAllocFns, HMODULE) override;
    void SignalTerminate() override;
    bool CanTerminate() override;
    void Terminate() override;
};

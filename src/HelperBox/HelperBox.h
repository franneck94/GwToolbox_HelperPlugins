#pragma once

#include <ToolboxUIPlugin.h>

class HelperBox : public ToolboxUIPlugin
{
public:
    HelperBox()
    {
        can_show_in_main_window = true;
        can_close = true;
    }

    const char *Name() const override
    {
        return "HelperBox";
    }
    const char *Icon() const override
    {
        return ICON_FA_COMMENT_DOTS;
    }

    void Initialize(ImGuiContext *, ImGuiAllocFns, HMODULE) override;
    void SignalTerminate() override;
    void Draw(IDirect3DDevice9 *pDevice) override;
    void DrawSettings() override;
    bool HasSettings() const override
    {
        return true;
    }
    void LoadSettings(const wchar_t *folder) override;
    void SaveSettings(const wchar_t *folder) override;
};

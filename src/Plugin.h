#pragma once

#include <ToolboxUIPlugin.h>

class Plugin : public ToolboxUIPlugin {
public:
    Plugin()
    {
        show_closebutton = false;
        show_title = false;
        can_collapse = false;
        can_close = false;
        is_resizable = true;
        is_movable = true;
        lock_size = true;
        lock_move = true;
    }

    const char* Name() const override { return "Plugin"; }

    void Draw(IDirect3DDevice9*) override;
    void Initialize(ImGuiContext*, ImGuiAllocFns, HMODULE) override;
    void SignalTerminate() override;
};

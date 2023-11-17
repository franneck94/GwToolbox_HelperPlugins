#pragma once

#include <cstdint>

#include <ToolboxUIPlugin.h>

#include "ActionsBase.h"
#include "DataLivings.h"
#include "DataPlayer.h"

class AutoFollowAction : public ActionABC
{
public:
    AutoFollowAction(DataPlayer *p) : ActionABC(p, "Follow")
    {
    }

    RoutineState Routine() override;
    void Update() override;
};

class AutoFollow : public ToolboxUIPlugin
{
public:
    AutoFollow() : player_data({}), auto_follow(&player_data)
    {
        can_show_in_main_window = true;
        can_close = true;
    }

    ~AutoFollow(){};

    const char *Name() const override
    {
        return "Auto Follow";
    }

    const char *Icon() const override
    {
        return ICON_FA_COMMENT_DOTS;
    }

    void Initialize(ImGuiContext *, ImGuiAllocFns, HMODULE) override;
    void SignalTerminate() override;
    void Draw(IDirect3DDevice9 *) override;
    bool HasSettings() const override
    {
        return false;
    }
    void Update(float delta) override;

private:
    DataPlayer player_data;
    AutoFollowAction auto_follow;
};

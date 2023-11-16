#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Party.h>

#include "Hotkeys.h"
#include <ActionsBase.h>
#include <Base/HelperBoxWindow.h>
#include <DataPlayer.h>

#include <SimpleIni.h>
#include <imgui.h>

class HotkeysWindow : public HelperBoxWindow
{
public:
    HotkeysWindow(){};
    ~HotkeysWindow(){};

    static HotkeysWindow &Instance()
    {
        static HotkeysWindow instance;
        return instance;
    }

    const char *Name() const override
    {
        return "Hotkeys";
    }

    void Initialize() override
    {
        HelperBoxWindow::Initialize();
    }

    void Terminate() override
    {
        HelperBoxWindow::Terminate();

        for (TBHotkey *hotkey : hotkeys)
        {
            delete hotkey;
        }
    }


    bool CheckSetValidHotkeys();

    void Update(float delta, const AgentLivingData &) override;
    void Draw() override;
    bool WndProc(UINT Message, WPARAM wParam, LPARAM lParam) override;
    void DrawSettingInternal() override;
    void LoadSettings(CSimpleIni *ini) override;
    void SaveSettings(CSimpleIni *ini) override;

    TBHotkey *current_hotkey = nullptr;
    std::vector<TBHotkey *> hotkeys;
    std::vector<TBHotkey *> valid_hotkeys;

    bool need_to_check_valid_hotkeys = true;
    long max_id_ = 0;
    bool block_hotkeys = false;
    bool map_change_triggered = false;

    bool HandleMapChange();
};

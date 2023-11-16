#pragma once

#include <utility>
#include <vector>

#include <Base/HelperBoxUIElement.h>
#include <Base/HelperBoxWindow.h>

class MainWindow : public HelperBoxWindow
{
    MainWindow()
    {
        visible = true;
        can_show_in_main_window = false;
    };
    ~MainWindow(){};

public:
    static MainWindow &Instance()
    {
        static MainWindow instance;
        return instance;
    }

    const char *Name() const override
    {
        return "Main Window";
    }

    const char *SettingsName() const override
    {
        return "HelperBox";
    }

    void Draw() override;
    void LoadSettings(CSimpleIni *ini) override;
    void SaveSettings(CSimpleIni *ini) override;
    void DrawSettingInternal() override{};
    void RegisterSettingsContent() override{};

    void RefreshButtons();
    bool pending_refresh_buttons = true;

private:
    std::vector<std::pair<float, HelperBoxUIElement *>> modules_to_draw{};
};

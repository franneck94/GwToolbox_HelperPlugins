#pragma once

#include <Base/HelperBoxUIElement.h>

#include <SimpleIni.h>
#include <imgui.h>

class HelperBoxTheme : public HelperBoxUIElement
{
    HelperBoxTheme();
    ~HelperBoxTheme()
    {
        if (windows_ini)
            delete windows_ini;
        if (inifile)
            delete inifile;
    }

public:
    static HelperBoxTheme &Instance()
    {
        static HelperBoxTheme instance;
        return instance;
    }

    const char *Name() const override
    {
        return "Theme";
    }

    void Terminate() override;
    void LoadSettings(CSimpleIni *ini) override;
    void SaveSettings(CSimpleIni *ini) override;
    void Draw() override;
    void ShowVisibleRadio() override{};

    void SaveUILayout();
    void LoadUILayout();

    void DrawSettingInternal() override;

    CSimpleIni *GetLayoutIni();

private:
    ImGuiStyle DefaultTheme();

    float font_global_scale = 1.0F;
    ImGuiStyle ini_style;
    bool layout_dirty = false;

    CSimpleIni *inifile = nullptr;
    CSimpleIni *windows_ini = nullptr;
};

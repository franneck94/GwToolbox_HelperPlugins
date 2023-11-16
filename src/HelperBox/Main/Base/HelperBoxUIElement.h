#pragma once

#include <Base/HelperBoxModule.h>

#include <d3d9.h>

class HelperBoxUIElement : public HelperBoxModule
{
    friend class HelperBoxSettings;

public:
    virtual void Draw(){};
    virtual const char *UIName() const;

    virtual void Initialize() override;
    virtual void Terminate() override;

    virtual void LoadSettings(CSimpleIni *ini) override;
    virtual void SaveSettings(CSimpleIni *ini) override;
    virtual bool DrawTabButton();

    virtual void RegisterSettingsContent() override;

    void DrawSizeAndPositionSettings();

    bool visible = false;
    bool lock_move = false;
    bool lock_size = false;
    bool show_closebutton = true;

    bool *GetVisiblePtr(bool force_show = false)
    {
        if (!has_closebutton || show_closebutton || force_show)
            return &visible;
        return nullptr;
    }

protected:
    bool can_show_in_main_window = true;
    bool has_closebutton = false;
    bool is_resizable = true;
    bool is_movable = true;

    virtual void ShowVisibleRadio();
    IDirect3DTexture9 *button_texture = nullptr;
};

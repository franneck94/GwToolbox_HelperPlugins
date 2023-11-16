#pragma once

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/GameEntities/Item.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/UIMgr.h>

#include <HelperMaps.h>

#include <Logger.h>
#include <SimpleIni.h>
#include <imgui.h>

class TBHotkey
{
public:
    enum Op
    {
        Op_None,
        Op_MoveUp,
        Op_MoveDown,
        Op_Delete,
        Op_BlockInput,
    };

    static bool show_active_in_header;
    static bool hotkeys_changed;
    static WORD *key_out;
    static DWORD *mod_out;

    static TBHotkey *HotkeyFactory(CSimpleIni *ini, const char *section);
    static void HotkeySelector(WORD *key, DWORD *modifier = nullptr);

    bool pressed = false;
    bool active = true;
    bool show_message_in_emote_channel = true;
    bool trigger_on_explorable = false;
    bool trigger_on_outpost = false;

    int instance_type = -1;

    long hotkey = 0;
    long modifier = 0;

    TBHotkey(CSimpleIni *ini, const char *section);
    virtual ~TBHotkey(){};

    virtual bool CanUse();
    virtual bool IsValid(const GW::Constants::InstanceType _instance_type);

    virtual void Save(CSimpleIni *ini, const char *section) const;
    bool Draw(Op *op);
    virtual const char *Name() const = 0;
    virtual bool Draw() = 0;
    virtual void Execute() = 0;
    virtual void Toggle()
    {
        return Execute();
    };
    virtual int Description(char *buf, size_t bufsz) = 0;

    const unsigned int ui_id = 0;
    static unsigned int cur_ui_id;
};

class HotkeyChestOpen : public TBHotkey
{
public:
    enum class ChestType
    {
        OpenXunlaiChest,
        OpenLockedChest,
    };

    const int n_actions = 2;
    ChestType action;

    static const char *IniSection()
    {
        return "ChestType";
    }
    const char *Name() const override
    {
        return IniSection();
    }

    HotkeyChestOpen(CSimpleIni *ini, const char *section);
    void Save(CSimpleIni *ini, const char *section) const override;
    bool Draw() override;
    void Execute() override;
    static bool GetText(void *, int idx, const char **out_text);
    int Description(char *buf, size_t bufsz) override;
};

class HotkeyTargetMinipet : public TBHotkey
{
public:
    char id[140] = {'\0'};
    char name[140] = {'\0'};

    static const char *IniSection()
    {
        return "Target";
    }
    const char *Name() const override
    {
        return IniSection();
    }

    HotkeyTargetMinipet(CSimpleIni *ini, const char *section);

    void Save(CSimpleIni *ini, const char *section) const override;
    bool Draw() override;
    int Description(char *buf, size_t bufsz) override;
    void Execute() override;
};

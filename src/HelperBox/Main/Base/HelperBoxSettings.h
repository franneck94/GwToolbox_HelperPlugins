#pragma once

#include <vector>

#include <Base/HelperBoxModule.h>
#include <Base/HelperBoxUIElement.h>

#include <SimpleIni.h>

class HelperBoxSettings : public HelperBoxUIElement
{
    HelperBoxSettings(){};
    HelperBoxSettings(const HelperBoxSettings &) = delete;
    ~HelperBoxSettings(){};

public:
    static HelperBoxSettings &Instance()
    {
        static HelperBoxSettings instance;
        return instance;
    }

    const char *Name() const override
    {
        return "Helper Settings";
    }

    void LoadModules(CSimpleIni *ini);

    void LoadSettings(CSimpleIni *ini) override;
    void SaveSettings(CSimpleIni *ini) override;
    void Draw() override;

    const std::vector<HelperBoxModule *> &GetOptionalModules() const
    {
        return optional_modules;
    }

    static bool move_all;

private:
    std::vector<HelperBoxModule *> optional_modules;

    bool use_emo = true;
    bool use_mainteam = true;
    bool use_terra = true;
    bool use_db = true;
    bool use_dhuum_stats = true;
    bool use_follow = true;
    bool use_cancel = true;
};

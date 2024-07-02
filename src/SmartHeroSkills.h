#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <map>

#include <ToolboxUIPlugin.h>

#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Hero.h>
#include <GWCA/GameEntities/Party.h>

class HeroSmartSkillBase : public ToolboxUIPlugin
{
public:
    HeroSmartSkillBase() {};
    ~HeroSmartSkillBase() {};

    const char *Icon() const override
    {
        return ICON_FA_CROWN;
    }

    void SignalTerminate() override;
    bool CanTerminate() override;
    void Terminate() override;
    bool HasSettings() const override
    {
        return false;
    }
    void Draw(IDirect3DDevice9 *) override {};

    /* MAIN CYCLE FUNCTIONS */

    virtual bool HeroSmarterSkills_Main();
};

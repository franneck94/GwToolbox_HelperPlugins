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

#include "ActionsBase.h"
#include "DataLivings.h"
#include "HelperHero.h"
#include "HelperPlayer.h"

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

    bool HeroSmarterSkills_Main();
};

class SmartBip : public HeroSmartSkillBase
{
public:
    SmartBip() {};
    ~SmartBip() {};

    const char *Name() const override
    {
        return "SmartBip";
    }

    void Initialize(ImGuiContext *, ImGuiAllocFns, HMODULE) override;
    void Update(float delta) override;

    /* MAIN CYCLE FUNCTIONS */

    bool HeroSmarterSkills_Main();
};

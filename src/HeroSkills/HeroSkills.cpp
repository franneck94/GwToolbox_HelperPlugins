#include <array>
#include <cmath>
#include <cstdint>
#include <random>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Context/GameContext.h>
#include <GWCA/Context/WorldContext.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Hero.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Player.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/PartyMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Utilities/Hooker.h>
#include <GWCA/Utilities/Scanner.h>

#include "ActionTypes.h"
#include "ActionsBase.h"
#include "Defines.h"
#include "Helper.h"
#include "HelperAgents.h"
#include "HelperHero.h"
#include "HelperMaps.h"
#include "HelperPlayer.h"
#include "HelperSkill.h"
#include "HeroSkills.h"
#include "HeroSmartSkills.h"
#include "Logger.h"
#include "Utils.h"
#include "UtilsGui.h"
#include "UtilsMath.h"

#include <imgui.h>

void HeroSmartSkillBase::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

bool HeroSmartSkillBase::CanTerminate()
{
    return GW::HookBase::GetInHookCount() == 0;
}

void HeroSmartSkillBase::Terminate()
{
    ToolboxPlugin::Terminate();
}

DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static SmartBip instance;
    return &instance;
}

void SmartBip::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);

    GW::Chat::WriteChat(GW::Chat::CHANNEL_GWCA1, L"Initialized", L"SmartBip");
}

void SmartBip::Update(float)
{
    if (!ValidateData(HelperActivationConditions, true))
        return;

    HeroSmarterSkills_Main();
}

bool SmartBip::HeroSmarterSkills_Main()
{
    if (!IsMapReady() || !IsExplorable())
        return false;

    // if (HeroSmartSkills::UseBipOnPlayer())
    //     return true;
    HeroSmartSkills::UseSplinterOnPlayer();
    HeroSmartSkills::UseShelterInFight();
    HeroSmartSkills::UseUnionInFight();
    HeroSmartSkills::UseSosInFight();
    HeroSmartSkills::UseVigSpiritOnPlayer();
    HeroSmartSkills::RemoveImportantConditions();
    HeroSmartSkills::ShatterImportantHexes();
    HeroSmartSkills::UseHonorOnPlayer();
    HeroSmartSkills::RuptEnemies();

    return false;
}

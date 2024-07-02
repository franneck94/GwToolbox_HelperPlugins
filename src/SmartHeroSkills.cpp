#include <array>
#include <cmath>
#include <cstdint>
#include <random>

#include "GWCA/Utilities/Hooker.h"
#include <GWCA/Constants/Constants.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Hero.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/Managers/AgentMgr.h>

#include "HelperHero.h"
#include "SmartHeroSkills.h"
#include "Utils.h"

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

#include <cstdint>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Managers/GuildMgr.h>
#include <GWCA/Managers/MapMgr.h>

#include "Helper.h"

bool IsLoading()
{
    return GW::Map::GetInstanceType() == GW::Constants::InstanceType::Loading;
}

bool IsExplorable()
{
    return GW::Map::GetInstanceType() == GW::Constants::InstanceType::Explorable;
}

bool IsOutpost()
{
    return GW::Map::GetInstanceType() == GW::Constants::InstanceType::Outpost;
}

bool IsMapReady()
{
    return (!IsLoading() && !GW::Map::GetIsObserving() && !GW::Map::GetIsInCinematic());
}

bool IsEndGameEntryOutpost()
{
    return (
#ifdef _DEBUG
        GW::Map::GetMapID() == GW::Constants::MapID::Great_Temple_of_Balthazar_outpost ||
        GW::Map::GetMapID() == GW::Constants::MapID::Isle_of_the_Nameless ||
        GW::Map::GetMapID() == GW::Constants::MapID::Ember_Light_Camp_outpost ||
#endif
        GW::Map::GetMapID() == GW::Constants::MapID::Embark_Beach ||
        GW::Map::GetMapID() == GW::Constants::MapID::Temple_of_the_Ages ||
        GW::Map::GetMapID() == GW::Constants::MapID::Chantry_of_Secrets_outpost ||
        GW::Map::GetMapID() == GW::Constants::MapID::Zin_Ku_Corridor_outpost);
}

bool IsFowEntryOutpost()
{
    return IsEndGameEntryOutpost();
}

bool IsFow()
{
    return (GW::Map::GetMapID() == GW::Constants::MapID::The_Fissure_of_Woe);
}

bool IsDoa()
{
    return (GW::Map::GetMapID() == GW::Constants::MapID::Domain_of_Anguish);
}

bool IsDoaEntryOutpost()
{
    return (GW::Map::GetMapID() == GW::Constants::MapID::Gate_of_Torment_outpost);
}

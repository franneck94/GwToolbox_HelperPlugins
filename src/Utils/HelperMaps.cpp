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

int RegionFromDistrict(const GW::Constants::District district)
{
    switch (district)
    {
    case GW::Constants::District::International:
        return GW::Constants::Region::International;
    case GW::Constants::District::American:
        return GW::Constants::Region::America;
    case GW::Constants::District::EuropeEnglish:
    case GW::Constants::District::EuropeFrench:
    case GW::Constants::District::EuropeGerman:
    case GW::Constants::District::EuropeItalian:
    case GW::Constants::District::EuropeSpanish:
    case GW::Constants::District::EuropePolish:
    case GW::Constants::District::EuropeRussian:
        return GW::Constants::Region::Europe;
    case GW::Constants::District::AsiaKorean:
        return GW::Constants::Region::Korea;
    case GW::Constants::District::AsiaChinese:
        return GW::Constants::Region::China;
    case GW::Constants::District::AsiaJapanese:
        return GW::Constants::Region::Japan;
    default:
        break;
    }

    return GW::Map::GetRegion();
}

int LanguageFromDistrict(const GW::Constants::District district)
{
    switch (district)
    {
    case GW::Constants::District::EuropeEnglish:
        return GW::Constants::EuropeLanguage::English;
    case GW::Constants::District::EuropeFrench:
        return GW::Constants::EuropeLanguage::French;
    case GW::Constants::District::EuropeGerman:
        return GW::Constants::EuropeLanguage::German;
    case GW::Constants::District::EuropeItalian:
        return GW::Constants::EuropeLanguage::Italian;
    case GW::Constants::District::EuropeSpanish:
        return GW::Constants::EuropeLanguage::Spanish;
    case GW::Constants::District::EuropePolish:
        return GW::Constants::EuropeLanguage::Polish;
    case GW::Constants::District::EuropeRussian:
        return GW::Constants::EuropeLanguage::Russian;
    case GW::Constants::District::AsiaKorean:
    case GW::Constants::District::AsiaChinese:
    case GW::Constants::District::AsiaJapanese:
    case GW::Constants::District::International:
    case GW::Constants::District::American:
        return 0;
    default:
        break;
    }

    return GW::Map::GetLanguage();
}

bool IsAlreadyInOutpost(const GW::Constants::MapID outpost_id,
                        const GW::Constants::District district,
                        const uint32_t district_number)
{
    return GW::Map::GetInstanceType() == GW::Constants::InstanceType::Outpost && GW::Map::GetMapID() == outpost_id &&
           RegionFromDistrict(district) == GW::Map::GetRegion() &&
           LanguageFromDistrict(district) == GW::Map::GetLanguage() &&
           (!district_number || district_number == static_cast<uint32_t>(GW::Map::GetDistrict()));
}

bool IsInGH()
{
    auto *p = GW::GuildMgr::GetPlayerGuild();
    return p && p == GW::GuildMgr::GetCurrentGH();
}

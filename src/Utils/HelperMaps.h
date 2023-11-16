#pragma once

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/Maps.h>

bool IsLoading();

bool IsExplorable();

bool IsOutpost();

bool IsMapReady();

bool IsEndGameEntryOutpost();

bool IsFowEntryOutpost();

bool IsDoa();

bool IsDoaEntryOutpost();

int RegionFromDistrict(const GW::Constants::District district);

int LanguageFromDistrict(const GW::Constants::District district);

bool IsAlreadyInOutpost(const GW::Constants::MapID outpost_id,
                        const GW::Constants::District district,
                        const uint32_t district_number);

bool IsInGH();

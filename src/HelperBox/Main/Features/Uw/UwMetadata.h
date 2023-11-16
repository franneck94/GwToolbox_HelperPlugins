#pragma once

#include <array>
#include <cstdint>

#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Packets/StoC.h>
#include <GWCA/Utilities/Hook.h>

#include <ActionsBase.h>
#include <DataPlayer.h>

class UwMetadata
{
public:
    UwMetadata();

    static UwMetadata &Instance()
    {
        static UwMetadata instance;
        return instance;
    }

    GW::HookEntry MapLoaded_Entry;
    bool load_cb_triggered = false;

    GW::HookEntry ObjectiveDone_Entry;
    uint32_t num_finished_objectives = 0U;

    GW::HookEntry SendChat_Entry;
    bool lt_is_ready = false;
    bool emo_is_ready = false;
    bool db_is_ready = false;
};

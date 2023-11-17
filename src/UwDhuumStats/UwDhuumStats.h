#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <vector>

#include <ToolboxUIPlugin.h>

#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Packets/StoC.h>
#include <GWCA/Utilities/Hook.h>

#include "ActionsBase.h"
#include "ActionsUw.h"
#include "DataLivings.h"
#include "DataPlayer.h"
#include "HelperCallbacks.h"
#include "UwMetadata.h"

#include <imgui.h>

class UwDhuumStats : public ToolboxUIPlugin
{
private:
    void SkillPacketCallback(const uint32_t value_id,
                             const uint32_t caster_id,
                             const uint32_t target_id,
                             const uint32_t value,
                             const bool no_target);
    void DamagePacketCallback(const uint32_t type,
                              const uint32_t caster_id,
                              const uint32_t target_id,
                              const float value);

    void ResetData();
    void RemoveOldData();
    void UpdateRestData();
    void UpdateDamageData();

public:
    UwDhuumStats() : player_data({}), rests({}), damages({}), livings_data({}), uw_metadata({})
    {
        /* Skill on self or party player_data */
        GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericValue>(
            &SkillCasted_Entry,
            [this](GW::HookStatus *, GW::Packet::StoC::GenericValue *packet) -> void {
                const uint32_t value_id = packet->value_id;
                const uint32_t caster_id = packet->agent_id;
                const uint32_t target_id = 0U;
                const uint32_t value = packet->value;
                const bool no_target = true;
                SkillPacketCallback(value_id, caster_id, target_id, value, no_target);
            });

        GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericModifier>(
            &Damage_Entry,
            [this](GW::HookStatus *, GW::Packet::StoC::GenericModifier *packet) -> void {
                const uint32_t type = packet->type;
                const uint32_t caster_id = packet->cause_id;
                const uint32_t target_id = packet->target_id;
                const float value = packet->value;
                DamagePacketCallback(type, caster_id, target_id, value);
            });
    };
    ~UwDhuumStats(){};

    const char *Name() const override
    {
        return "DhuumStatsWindow";
    }

    const char *Icon() const override
    {
        return ICON_FA_COMMENT_DOTS;
    }

    void Initialize(ImGuiContext *, ImGuiAllocFns, HMODULE) override;
    void SignalTerminate() override;
    void Draw(IDirect3DDevice9 *) override;
    bool HasSettings() const override
    {
        return false;
    }
    void Update(float delta) override;

private:
    DataPlayer player_data;
    AgentLivingData livings_data;
    UwMetadata uw_metadata;

    GW::HookEntry SkillCasted_Entry;
    GW::HookEntry Damage_Entry;

    uint32_t dhuum_id = 0U;
    float dhuum_hp = 0.0F;
    uint32_t dhuum_max_hp = 0U;

    long dhuum_fight_start_time_ms = 0L;
    bool dhuum_fight_active = false;

    uint32_t num_casted_rest = 0U;
    float progress_perc = 0.0F;
    float rests_per_s = 0.0F;
    float eta_rest_s = 0.0F;
    std::vector<long> rests;

    uint32_t num_attacks = 0U;
    float damage_per_s = 0.0F;
    float eta_damage_s = 0.0F;
    std::vector<std::pair<long, float>> damages;
};

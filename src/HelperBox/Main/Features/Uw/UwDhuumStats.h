#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <vector>

#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Packets/StoC.h>
#include <GWCA/Utilities/Hook.h>

#include <ActionsBase.h>
#include <ActionsUw.h>
#include <Base/HelperBoxWindow.h>
#include <DataPlayer.h>
#include <Features/Uw/UwMetadata.h>
#include <HelperCallbacks.h>

#include <SimpleIni.h>
#include <imgui.h>

class UwDhuumStats : public HelperBoxWindow
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
    UwDhuumStats() : player_data({}), rests({}), damages({})
    {
        /* Skill on self or party player_data */
        GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericValue>(
            &SkillCasted_Entry,
            [this](GW::HookStatus *, GW::Packet::StoC::GenericValue *packet) -> void {
                const uint32_t value_id = packet->Value_id;
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

    static UwDhuumStats &Instance()
    {
        static UwDhuumStats instance;
        return instance;
    }

    const char *Name() const override
    {
        return "DhuumStatsWindow";
    }

    void Draw() override;
    void Update(float delta, const AgentLivingData &) override;

private:
    DataPlayer player_data;
    const AgentLivingData *livings_data = nullptr;

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

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include <GWCA/GameEntities/Agent.h>

#include <ActionsBase.h>
#include <ActionsUw.h>
#include <Base/HelperBoxWindow.h>
#include <DataLivings.h>
#include <DataPlayer.h>
#include <Features/Uw/UwMetadata.h>

#include <SimpleIni.h>
#include <imgui.h>

struct SpikeSkillInfo
{
    uint32_t last_id;
    GW::Constants::SkillID last_skill;
};

struct TriggeredSpikeSkillInfo
{
    GW::Constants::SkillID triggered_skill_id;
    uint32_t target_id;
};

class LtRoutine : public MesmerActionABC
{
public:
    LtRoutine(DataPlayer *p, MesmerSkillbarData *s, const AgentLivingData *a)
        : MesmerActionABC(p, "LtRoutine", s), livings_data(a)
    {
        GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericValueTarget>(
            &GenericValueTarget_Entry,
            [this](GW::HookStatus *status, GW::Packet::StoC::GenericValueTarget *packet) -> void {
                UNREFERENCED_PARAMETER(status);

                const uint32_t value_id = packet->Value_id;
                const uint32_t caster_id = packet->caster;
                const uint32_t target_id = packet->target;
                const uint32_t value = packet->value;
                const bool no_target = true;
                SkillPacketCallback(value_id, caster_id, target_id, value, no_target);
            });
    };

    RoutineState Routine() override;
    void Update() override;

private:
    void SkillPacketCallback(const uint32_t value_id,
                             const uint32_t caster_id,
                             const uint32_t target_id,
                             const uint32_t value,
                             const bool no_target);

    static bool EnemyShouldGetEmpathy(const std::vector<GW::AgentLiving *> &enemies, const GW::AgentLiving *enemy);
    bool DoNeedVisage() const;
    bool ReadyForSpike() const;
    bool RoutineSelfEnches() const;
    bool RoutineSpikeBall(const auto include_graspings);
    bool CastSingleHexOnEnemy(const GW::AgentLiving *enemy, SpikeSkillInfo &spike_skill, const DataSkill &skill);
    bool CastHexesOnEnemyType(const std::vector<GW::AgentLiving *> &filtered_enemies,
                              SpikeSkillInfo &spike_skill,
                              const bool use_empathy);

public:
    const AgentLivingData *livings_data = nullptr;
    std::vector<GW::AgentLiving *> enemies_in_aggro = {};
    bool load_cb_triggered = false;

private:
    std::vector<GW::AgentLiving *> nightmares = {};
    SpikeSkillInfo nightmare_spike = {};
    std::vector<GW::AgentLiving *> dryders = {};
    SpikeSkillInfo dryder_spike = {};
    std::vector<GW::AgentLiving *> dryders_silver = {};
    SpikeSkillInfo dryder_silver_spike = {};
    std::vector<GW::AgentLiving *> aatxes = {};
    SpikeSkillInfo aatxe_spike = {};
    std::vector<GW::AgentLiving *> graspings = {};
    SpikeSkillInfo graspings_spike = {};
    std::vector<GW::AgentLiving *> mindblades = {};
    SpikeSkillInfo mindblades_spike = {};
    std::vector<GW::AgentLiving *> horsemans = {};
    SpikeSkillInfo horsemans_spike = {};
    std::vector<GW::AgentLiving *> smites = {};
    SpikeSkillInfo smites_spike = {};
    std::vector<GW::AgentLiving *> coldfires = {};
    SpikeSkillInfo coldfires_spike = {};
    std::vector<GW::AgentLiving *> skeles = {};
    SpikeSkillInfo skeles_spike = {};
    std::vector<GW::AgentLiving *> collector = {};
    SpikeSkillInfo collector_spike = {};
    std::vector<GW::AgentLiving *> thresher = {};
    SpikeSkillInfo thresher_spike = {};

    bool starting_active = false;
    long delay_ms = 200L;

    TriggeredSpikeSkillInfo triggered_spike_skill = {};
    GW::HookEntry GenericValueTarget_Entry;
};

class UwMesmer : public HelperBoxWindow
{
public:
    UwMesmer()
        : player_data({}), filtered_livings({}), aatxe_livings({}), dryder_livings({}), skele_livings({}), skillbar({}),
          lt_routine(&player_data, &skillbar, livings_data)
    {
        if (skillbar.ValidateData())
            skillbar.Load();
    };
    ~UwMesmer(){};

    static UwMesmer &Instance()
    {
        static UwMesmer instance;
        return instance;
    }

    const char *Name() const override
    {
        return "UwMesmer";
    }

    void Draw() override;
    void Update(float delta, const AgentLivingData &) override;

private:
    void DrawSplittedAgents(std::vector<GW::AgentLiving *> livings, const ImVec4 color, std::string_view label);

    DataPlayer player_data;
    const AgentLivingData *livings_data = nullptr;
    MesmerSkillbarData skillbar;
    LtRoutine lt_routine;

    std::vector<GW::AgentLiving *> filtered_livings;
    std::vector<GW::AgentLiving *> aatxe_livings;
    std::vector<GW::AgentLiving *> dryder_livings;
    std::vector<GW::AgentLiving *> nightmare_livings;
    std::vector<GW::AgentLiving *> skele_livings;
    std::vector<GW::AgentLiving *> horseman_livings;
    std::vector<GW::AgentLiving *> keeper_livings;
};

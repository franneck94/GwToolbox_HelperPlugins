#pragma once

#include <array>
#include <cstdint>

#include <GWCA/Packets/StoC.h>
#include <GWCA/Utilities/Hook.h>

#include "ActionsBase.h"
#include <ActionsMove.h>
#include <Base/HelperBoxWindow.h>
#include "DataLivings.h"
#include "DataPlayer.h"
#include <Features/Uw/UwMetadata.h>
#include <HelperCallbacks.h>
#include "HelperUw.h"
#include "UtilsGui.h"

#include <SimpleIni.h>
#include <imgui.h>

class DbRoutine : public DbActionABC
{
public:
    DbRoutine(DataPlayer *p, DbSkillbarData *s, const AgentLivingData *a)
        : DbActionABC(p, "DbRoutine", s), livings_data(a){};

    RoutineState Routine() override;
    void Update() override;

private:
    bool PauseRoutine() noexcept override;

    bool CastPiOnTarget() const;
    bool RoutineKillSkele() const;
    bool RoutineKillEnemiesStandard() const;
    bool RoutineValeSpirits() const;
    bool RoutineDhuumRecharge() const;
    bool RoutineDhuumDamage() const;

public:
    const AgentLivingData *livings_data = nullptr;
    uint32_t num_finished_objectives = 0U;
};

class UwDhuumBitch : public HelperBoxWindow
{
public:
    UwDhuumBitch();
    ~UwDhuumBitch(){};

    static UwDhuumBitch &Instance()
    {
        static UwDhuumBitch instance;
        return instance;
    }

    const char *Name() const override
    {
        return "UwDhuumBitch";
    }

    void Initialize() override
    {
        HelperBoxWindow::Initialize();
        first_frame = true;
    }

    void LoadSettings(CSimpleIni *ini) override
    {
        HelperBoxWindow::LoadSettings(ini);
        show_debug_map = ini->GetBoolValue(Name(), VAR_NAME(show_debug_map), show_debug_map);
    }

    void SaveSettings(CSimpleIni *ini) override
    {
        HelperBoxWindow::SaveSettings(ini);
        ini->SetBoolValue(Name(), VAR_NAME(show_debug_map), show_debug_map);
    }

    void DrawSettingInternal() override
    {
#ifdef _DEBUG
        const auto width = ImGui::GetWindowWidth();
        ImGui::Text("Show Debug Map:");
        ImGui::SameLine(width * 0.5F);
        ImGui::PushItemWidth(width * 0.5F);
        ImGui::Checkbox("debugMapActive", &show_debug_map);
        ImGui::PopItemWidth();
#endif
    }

    void Draw() override;
    void Update(float delta, const AgentLivingData &) override;

private:
    void UpdateUw();
    void UpdateUwEntry();

    // Settings
    bool show_debug_map = true;

    bool first_frame = false;
    DataPlayer player_data;
    const AgentLivingData *livings_data = nullptr;
    DbSkillbarData skillbar;

    DbRoutine db_routine;

    std::function<bool()> target_reaper_fn = [&]() { return TargetReaper(player_data, livings_data->npcs); };
    std::function<bool()> talk_reaper_fn = [&]() { return TalkReaper(player_data, livings_data->npcs); };
    std::function<bool()> cast_sq = [&]() {
        skillbar.sq.Cast(player_data.energy);
        return true;
    };

    static inline const auto SKILLS_START_TRIGGER = GW::GamePos{-790.53F, 9529.63F, 0};

    uint32_t move_idx = 0;
    const std::array<MoveABC *, 63> moves = {
        new Move_WaitAndContinue{1248.00F, 6965.51F, "Go Spawn"},
        new Move_PositionAndContinue{613.38F, 7097.03F, "Start Skills", SKILLS_START_TRIGGER, 180.0F, TriggerRole::LT},
        new Move_WaitAndContinue{613.38F, 7097.03F, "Cast SQ", cast_sq},
        new Move_WaitAndContinue{314.57F, 7511.54F, "Go EoE "},
        new Move_CastSkillAndContinue{314.57F, 7511.54F, "EoE 1", &skillbar.eoe},
        new Move_WaitAndContinue{314.57F, 7511.54F, "Go EoE 1"},
        new Move_WaitAndContinue{1319.41F, 7299.941F, "Go Qz"},
        new Move_CastSkillAndContinue{1319.41F, 7299.94F, "Cast Qz", &skillbar.qz},
        new Move_WaitAndContinue{1319.41F, 7299.941F, "Go Qz"},
        new Move_WaitAndContinue{985.70F, 7325.54F, "Go Chamber 1"}, // 10
        new Move_WaitAndContinue{-634.07F, 9071.42F, "Go Chamber 2"},
        new Move_WaitAndContinue{-1522.58F, 10634.12F, "Go Lab 1"},
        new Move_WaitAndContinue{-2726.856F, 10239.48F, "Go Lab 2"},
        new Move_WaitAndContinue{-2828.35F, 10020.46F, "Go Lab 3"},
        new Move_WaitAndContinue{-4012.72F, 11130.53F, "Go Lab 4", cast_sq},
        new Move_CastSkillAndContinue{-4012.72F, 11130.53F, "Cast EoE 2", &skillbar.eoe},
        new Move_WaitAndContinue{-4012.72F, 11130.53F, "Go Lab 4"},
        new Move_WaitAndContinue{-4390.63F, 11647.44F, "Go Lab 5"},
        new Move_WaitAndStop{-5751.45F, 12746.52F, "Go Lab Reaper", target_reaper_fn},
        new Move_NoWaitAndContinue{-5751.45F, 12746.52F, "Talk Lab", talk_reaper_fn}, // 20
        new Move_NoWaitAndContinue{-5751.45F, 12746.52F, "Accept Lab", [&]() { return AcceptChamber(); }},
        new Move_NoWaitAndContinue{-5751.45F, 12746.52F, "Take Restore", [&]() { return TakeRestore(); }},
        new Move_WaitAndContinue{-5751.45F, 12746.52F, "Take Escort", [&]() { return TakeEscort(); }},
        new Move_WaitAndContinue{-6622.24F, 10387.12F, "Go Basement 1", cast_sq},
        new Move_CastSkillAndContinue{-6622.24F, 10387.12F, "Cast EoE 3", &skillbar.eoe},
        new Move_NoWaitAndContinue{-6343.61F, 9979.46F, "Go Basement 1"},
        new Move_DistanceAndContinue{-5183.64F, 8876.31F, "Go Basement 2", 2200.0F},
        new Move_DistanceAndContinue{-6241.24F, 7945.73F, "Go Basement 3", 2000.0F},
        new Move_DistanceAndContinue{-8798.22F, 5643.86F, "Go Basement 4", 4000.0F},
        new Move_WaitAndContinue{-8518.53F, 4765.09F, "Go Basement 5", cast_sq}, // 30
        new Move_CastSkillAndContinue{-8518.53F, 4765.09F, "Cast EoE 4", &skillbar.eoe},
        new Move_WaitAndContinue{-8518.53F, 4765.09F, "Go Basement 5"},
        new Move_DistanceAndContinue{-7289.94F, 3283.81F, "Go Vale Door", 3400.0F},
        new Move_DistanceAndContinue{-7846.65F, 2234.26F, "Go Vale Bridge", 3400.0F},
        new Move_WaitAndContinue{-8764.08F, 2156.60F, "Go Vale Entry"},
        new Move_DistanceAndContinue{-12264.12F, 1821.18F, "Go Vale House", 3400.0F},
        new Move_CastSkillAndContinue{-12264.12F, 1821.18F, "Cast EoE 5", &skillbar.eoe},
        new Move_DistanceAndContinue{-12264.12F, 1821.18F, "Go  Vale House", 3500.0F},
        new Move_DistanceAndContinue{-12145.44F, 1101.74F, "Go  Vale Center", 3200.0F},
        new Move_NoWaitAndContinue{-13760.19F, 358.15F, "Go Spirits 1"}, // 40
        new Move_DistanceAndContinue{-13872.34F, 2332.34F, "Go Spirits 2", 4900.0F},
        new Move_WaitAndStop{-13312.71F, 5165.071F, "Go Vale Reaper", target_reaper_fn},
        new Move_NoWaitAndContinue{8685.21F, 6344.59F, "Go Pits Start"},
        new Move_NoWaitAndContinue{12566.49F, 7812.503F, "Go Pits 1"},
        new Move_CastSkillAndContinue{12566.49F, 7812.503F, "Cast Winnow 1", &skillbar.winnow},
        new Move_NoWaitAndContinue{12566.49F, 7812.503F, "Go Pits 1"},
        new Move_NoWaitAndStop{8685.21F, 6344.59F, "Go Pits Reaper", target_reaper_fn},
        new Move_NoWaitAndContinue{8685.21F, 6344.59F, "Talk Pits", talk_reaper_fn},
        new Move_NoWaitAndStop{8685.21F, 6344.59F, "Take Pits", [&]() { return TakePits(); }},
        new Move_NoWaitAndContinue{11368.55F, -17974.64F, "Go Planes Reaper", target_reaper_fn}, // 50
        new Move_NoWaitAndStop{11368.55F, -17974.64F, "Talk Planes", talk_reaper_fn},
        new Move_NoWaitAndContinue{11368.55F, -17974.64F, "Take Planes", [&]() { return TakePlanes(); }},
        new Move_NoWaitAndContinue{9120.00F, -18432.003F, "Go Planes 1"},
        new Move_CastSkillAndContinue{9120.00F, -18432.003F, "Cast Winnow 2", &skillbar.winnow},
        new Move_NoWaitAndContinue{9120.00F, -18432.003F, "Waiting"},
        new Move_CastSkillAndContinue{9120.00F, -18432.003F, "Cast EoE 6", &skillbar.eoe},
        new Move_NoWaitAndStop{11368.55F, -17974.64F, "Finish Planes"},
        new Move_NoWaitAndContinue{-235.05F, 18496.461F, "Go To Dhuum 1"},
        new Move_NoWaitAndContinue{-2537.51F, 19139.91F, "Go To Dhuum 2"},
        new Move_NoWaitAndContinue{-6202.59F, 18704.91F, "Go To Dhuum 3"}, // 60
        new Move_NoWaitAndContinue{-9567.56F, 17288.916F, "Go To Dhuum 4"},
        new Move_NoWaitAndContinue{-13127.69F, 17284.64F, "Go To Dhuum 5"},
        new Move_NoWaitAndStop{-16410.75F, 17294.47F, "Go To Dhuum 6"},
    };
};

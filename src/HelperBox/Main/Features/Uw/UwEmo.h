#pragma once

#include <array>
#include <cstdint>

#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Packets/StoC.h>
#include <GWCA/Utilities/Hook.h>

#include <ActionsBase.h>
#include <ActionsMove.h>
#include <ActionsUw.h>
#include <Base/HelperBoxWindow.h>
#include <DataLivings.h>
#include <DataPlayer.h>
#include <Features/Uw/UwMetadata.h>
#include <Helper.h>
#include <HelperAgents.h>
#include <HelperItems.h>
#include <HelperUw.h>
#include <UtilsGui.h>

#include <SimpleIni.h>
#include <imgui.h>

class EmoRoutine : public EmoActionABC
{
public:
    EmoRoutine(DataPlayer *p, EmoSkillbarData *s, uint32_t *_bag_idx, uint32_t *_slot_idx, const AgentLivingData *a);

    RoutineState Routine() override;
    void Update() override;

private:
    bool PauseRoutine() noexcept override;

    bool BondLtAtStartRoutine() const;
    bool RoutineSelfBonds() const;
    bool RoutineDbBeforeDhuum() const;
    bool RoutineWhenInRangeBondLT() const;
    bool RoutineLtAtFusePulls() const;
    bool RoutineEscortSpirits() const;
    bool RoutineCanthaGuards() const;
    bool RoutineDbAtSpirits() const;
    bool DropBondsLT() const;
    bool RoutineTurtle() const;
    bool RoutineDbAtDhuum() const;
    bool RoutineWisdom() const;
    bool RoutineGDW() const;
    bool RoutineTurtleGDW() const;
    bool RoutineKeepPlayerAlive() const;
    bool DropAllBonds() const;

    const uint32_t *bag_idx = nullptr;
    const uint32_t *slot_idx = nullptr;

    GW::HookEntry Summon_AgentAdd_Entry;
    bool found_turtle = false;
    uint32_t turtle_id = 0;

public:
    const GW::Agent *lt_agent = nullptr;
    const GW::Agent *db_agent = nullptr;
    const AgentLivingData *livings_data = nullptr;
    bool used_canthas = false;
    uint32_t num_finished_objectives = 0U;

private:
    std::vector<PlayerMapping> party_members{};
    bool party_data_valid = false;
};

class UwEmo : public HelperBoxWindow
{
public:
    UwEmo();
    ~UwEmo(){};

    static UwEmo &Instance()
    {
        static UwEmo instance;
        return instance;
    }

    const char *Name() const override
    {
        return "UwEmo";
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
        bag_idx = ini->GetLongValue(Name(), VAR_NAME(bag_idx), bag_idx);
        slot_idx = ini->GetLongValue(Name(), VAR_NAME(slot_idx), slot_idx);
    }

    void SaveSettings(CSimpleIni *ini) override
    {
        HelperBoxWindow::SaveSettings(ini);
        ini->SetBoolValue(Name(), VAR_NAME(show_debug_map), show_debug_map);
        ini->SetLongValue(Name(), VAR_NAME(bag_idx), bag_idx);
        ini->SetLongValue(Name(), VAR_NAME(slot_idx), slot_idx);
    }

    void DrawSettingInternal() override
    {
        static auto _bag_idx = static_cast<int>(bag_idx);
        static auto _slot_idx = static_cast<int>(slot_idx);

        const auto width = ImGui::GetWindowWidth();
        ImGui::Text("Low HP Armor Slots:");

        ImGui::Text("Bag Idx (starts at 1):");
        ImGui::SameLine(width * 0.5F);
        ImGui::PushItemWidth(width * 0.5F);
        ImGui::InputInt("###inputBagIdx", &_bag_idx, 1, 1);
        ImGui::PopItemWidth();
        bag_idx = _bag_idx;

        ImGui::Text("First Armor Piece Idx (starts at 1):");
        ImGui::SameLine(width * 0.5F);
        ImGui::PushItemWidth(width * 0.5F);
        ImGui::InputInt("###inputStartSlot", &_slot_idx, 1, 1);
        ImGui::PopItemWidth();
        slot_idx = _slot_idx;

#ifdef _DEBUG
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

    bool first_frame = false;
    EmoSkillbarData skillbar;
    DataPlayer player_data;
    const AgentLivingData *livings_data = nullptr;
    EmoRoutine emo_routine;

    // Settings
    bool show_debug_map = true;
    uint32_t bag_idx = static_cast<uint32_t>(-1);
    uint32_t slot_idx = static_cast<uint32_t>(-1);

    std::function<bool()> swap_to_high_armor_fn = [&]() { return HighArmor(bag_idx, slot_idx); };
    std::function<bool()> swap_to_low_armor_fn = [&]() { return LowArmor(bag_idx, slot_idx); };
    std::function<bool()> target_reaper_fn = [&]() { return TargetReaper(player_data, livings_data->npcs); };
    std::function<bool()> target_lt_fn = [&]() { return TargetTrigger(player_data, TriggerRole::LT); };
    std::function<bool()> talk_reaper_fn = [&]() { return TalkReaper(player_data, livings_data->npcs); };
    std::function<bool()> take_uwg_fn = [&]() { return TakeUWG(); };

    static inline const auto SKILLS_START_TRIGGER = GW::GamePos{-790.53F, 9529.63F, 0};
    static inline const auto KEEPER3_TRIGGER = GW::GamePos{-2655.90F, 13362.98F, 0};
    static inline const auto KEEPER4_TRIGGER = GW::GamePos{161.13F, 13122.947F, 0};
    static inline const auto KEEPER6_TRIGGER = GW::GamePos{3023.24F, 9980.01F, 0};

    uint32_t move_idx = 0;
    std::array<MoveABC *, 55> moves = {
        new Move_WaitAndContinue{1248.00F, 6965.51F, "Go Spawn", swap_to_high_armor_fn},
        new Move_PositionAndContinue{985.70F, 7325.54F, "Go Chamber 1", SKILLS_START_TRIGGER, 180.0F, TriggerRole::LT},
        new Move_WaitAndContinue{-634.07F, 9071.42F, "Go Chamber 2", swap_to_high_armor_fn},
        new Move_WaitAndContinue{-1522.58F, 10634.12F, "Go Lab 1"},
        new Move_WaitAndContinue{-2726.856F, 10239.48F, "Go Lab 2"}, // 5
        new Move_WaitAndContinue{-2828.35F, 10020.46F, "Go Lab 3"},
        new Move_WaitAndContinue{-4012.72F, 11130.53F, "Go Lab 4"},
        new Move_WaitAndContinue{-4390.63F, 11647.44F, "Go Lab 5"},
        new Move_WaitAndContinue{-5751.45F, 12746.52F, "Go Lab Reaper", target_lt_fn},
        new Move_DistanceAndContinue{-5830.34F, 11781.70F, "Go Lab 6", 3700.0F}, // 10
        new Move_NoWaitAndContinue{-6263.33F, 9899.79F, "Go Fuse 1", swap_to_low_armor_fn},
        new Move_DistanceAndContinue{-5183.64F, 8876.31F, "Go Basement 1", 2000.0F},
        new Move_DistanceAndContinue{-6241.24F, 7945.73F, "Go Basement 2", 2000.0F},
        new Move_DistanceAndContinue{-8798.22F, 5643.86F, "Go Basement 3", 4000.0F},
        new Move_DistanceAndContinue{-8049.28F, 4259.61F, "Go Fuse 2", 3400.0F}, // 15
        new Move_DistanceAndContinue{-7289.94F, 3283.81F, "Go Vale Door", 3400.0F, swap_to_high_armor_fn},
        new Move_DistanceAndContinue{-7846.65F, 2234.26F, "Go Vale Bridge", 3400.0F},
        new Move_WaitAndContinue{-8764.08F, 2156.60F, "Go Vale Entry"},
        new Move_DistanceAndContinue{-12264.12F, 1821.18F, "Go Vale House", 3400.0F},
        new Move_DistanceAndContinue{-12145.44F, 1101.74F, "Go Vale Center", 3200.0F}, // 20
        new Move_NoWaitAndContinue{-13872.34F, 2332.34F, "Go Spirits 1"},
        new Move_WaitAndContinue{-13760.19F, 358.15F, "Go Spirits 2"},
        new Move_NoWaitAndContinue{-12145.44F, 1101.74F, "Go Vale Center"},
        new Move_WaitAndContinue{-8764.08F, 2156.60F, "Go Vale Entry"},
        new Move_NoWaitAndContinue{-7980.55F, 4308.90F, "Go Basement 4"}, // 25
        new Move_WaitAndContinue{-6241.24F, 7945.73F, "Go Basement 5"},
        new Move_NoWaitAndStop{-5751.45F, 12746.52F, "Go Lab Reaper", target_reaper_fn},
        new Move_NoWaitAndContinue{-5751.45F, 12746.52F, "Talk Lab Reaper", talk_reaper_fn},
        new Move_NoWaitAndContinue{-5751.45F, 12746.52F, "Take UWG", take_uwg_fn},
        new Move_NoWaitAndContinue{-6035.29F, 11285.14F, "Go Keeper 1"}, // 30
        new Move_WaitAndContinue{-6511.41F, 12447.65F, "Go Keeper 2"},
        new Move_PositionAndContinue{-3881.71F, 11280.04F, "Go Keeper 3", KEEPER3_TRIGGER, 300.0F, TriggerRole::LT},
        new Move_PositionAndContinue{-1502.45F, 9737.64F, "Go Keeper 4/5", KEEPER4_TRIGGER, 500.0F, TriggerRole::LT},
        new Move_PositionAndContinue{-266.03F, 9304.26F, "Go Lab 1", KEEPER6_TRIGGER, 400.0F, TriggerRole::LT},
        new Move_NoWaitAndStop{1207.05F, 7732.16F, "Go Keeper 6"}, // 35
        new Move_NoWaitAndContinue{1543.75F, 10709.27F, "Go To Wastes 1"},
        new Move_NoWaitAndContinue{2532.49F, 10349.75F, "Go To Wastes 2"},
        new Move_NoWaitAndContinue{3360.00F, 9404.54F, "Go To Wastes 3"},
        new Move_NoWaitAndContinue{4398.38F, 7507.83F, "Go To Wastes 4"},
        new Move_NoWaitAndContinue{6599.55F, 11016.60F, "Go To Wastes 5"}, // 40
        new Move_NoWaitAndStop{6633.37F, 15385.31F, "Go Wastes 1"},
        new Move_NoWaitAndContinue{6054.83F, 18997.46F, "Go Wastes 2"},
        new Move_WaitAndContinue{4968.64F, 16555.77F, "Go Wastes 3"},
        new Move_WaitAndStop{2152.55F, 16893.93F, "Go Wastes 4"},
        new Move_NoWaitAndContinue{8685.21F, 6344.59F, "Go Pits Start"}, // 45
        new Move_NoWaitAndContinue{12566.49F, 7812.503F, "Go Pits 1"},
        new Move_NoWaitAndStop{11958.36F, 6281.43F, "Go Pits 2"},
        new Move_NoWaitAndContinue{11368.55F, -17974.64F, "Go Planes Start"},
        new Move_WaitAndStop{12160.99F, -16830.55F, "Go Planes 1"},
        new Move_NoWaitAndContinue{-235.05F, 18496.461F, "Go To Dhuum 1"}, // 50
        new Move_NoWaitAndContinue{-2537.51F, 19139.91F, "Go To Dhuum 2"},
        new Move_NoWaitAndContinue{-6202.59F, 18704.91F, "Go To Dhuum 3"},
        new Move_NoWaitAndContinue{-9567.56F, 17288.916F, "Go To Dhuum 4", swap_to_low_armor_fn},
        new Move_NoWaitAndContinue{-13127.69F, 17284.64F, "Go To Dhuum 5"},
        new Move_NoWaitAndStop{-16105.50F, 17284.84F, "Go To Dhuum 6"}, // 55
    };
};

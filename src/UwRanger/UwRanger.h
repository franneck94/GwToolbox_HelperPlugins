#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include <ToolboxUIPlugin.h>

#include <GWCA/GameEntities/Agent.h>

#include "ActionsBase.h"
#include "DataLivings.h"
#include "DataPlayer.h"
#include "Utils.h"

#include <imgui.h>

class AutoTargetAction : public ActionABC
{
public:
    AutoTargetAction() : ActionABC("Auto Target") {};

    RoutineState Routine() override;
    void Update() override;

    std::vector<GW::AgentLiving *> *behemoth_livings = nullptr;
};

class UwRanger : public ToolboxUIPlugin
{
public:
    UwRanger()
        : filtered_livings({}), auto_target(), last_casted_times_ms({}),
          livings_data({}) {};
    ~UwRanger() {};

    static UwRanger &Instance()
    {
        static UwRanger instance;
        return instance;
    }

    const char *Name() const override
    {
        return "UWRanger";
    }

    const char *Icon() const override
    {
        return ICON_FA_COMMENT_DOTS;
    }

    void Initialize(ImGuiContext *, ImGuiAllocFns, HMODULE) override;
    void SignalTerminate() override;
    bool CanTerminate();
    void Terminate();

    void Draw(IDirect3DDevice9 *) override;
    void DrawSettings() override;
    bool HasSettings() const override
    {
        return true;
    }
    void LoadSettings(const wchar_t *folder) override;
    void SaveSettings(const wchar_t *folder) override;
    void Update(float delta) override;

private:
    void DrawSplittedAgents(std::vector<GW::AgentLiving *> livings,
                            const ImVec4 color,
                            std::string_view label,
                            const bool draw_time = true);

    // Settings
    bool attack_at_auto_target = true;

    std::map<uint32_t, clock_t> last_casted_times_ms;

    std::vector<GW::AgentLiving *> filtered_livings;
    std::vector<GW::AgentLiving *> coldfire_livings;
    std::vector<GW::AgentLiving *> behemoth_livings;
    std::vector<GW::AgentLiving *> dryder_livings;
    std::vector<GW::AgentLiving *> skele_livings;
    std::vector<GW::AgentLiving *> horseman_livings;

    AutoTargetAction auto_target;

    AgentLivingData livings_data;
};

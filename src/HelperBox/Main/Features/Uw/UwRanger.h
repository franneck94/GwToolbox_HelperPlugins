#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include <GWCA/GameEntities/Agent.h>

#include <ActionsBase.h>
#include <Base/HelperBoxWindow.h>
#include <DataPlayer.h>
#include <Utils.h>

#include <SimpleIni.h>
#include <imgui.h>

class AutoTargetAction : public ActionABC
{
public:
    AutoTargetAction(DataPlayer *p) : ActionABC(p, "Auto Target"){};

    RoutineState Routine() override;
    void Update() override;

    std::vector<GW::AgentLiving *> *behemoth_livings = nullptr;
};

class UwRanger : public HelperBoxWindow
{
public:
    UwRanger() : player_data({}), filtered_livings({}), auto_target(&player_data), last_casted_times_ms({}){};
    ~UwRanger(){};

    static UwRanger &Instance()
    {
        static UwRanger instance;
        return instance;
    }

    const char *Name() const override
    {
        return "UwRanger";
    }

    void Draw() override;
    void Update(float delta, const AgentLivingData &) override;

    void DrawSettingInternal() override
    {
        static auto _attack_at_auto_target = attack_at_auto_target;
        const auto width = ImGui::GetWindowWidth();

        ImGui::Text("Also attack Behemoths wile auto target is active:");
        ImGui::SameLine(width * 0.5F);
        ImGui::PushItemWidth(width * 0.5F);
        ImGui::Checkbox("###attackAtAutoTarget", &_attack_at_auto_target);
        ImGui::PopItemWidth();
        attack_at_auto_target = _attack_at_auto_target;
    }

private:
    void DrawSplittedAgents(std::vector<GW::AgentLiving *> livings,
                            const ImVec4 color,
                            std::string_view label,
                            const bool draw_time = true);

    // Settings
    bool attack_at_auto_target = true;

    DataPlayer player_data;
    std::map<uint32_t, clock_t> last_casted_times_ms;

    std::vector<GW::AgentLiving *> filtered_livings;
    std::vector<GW::AgentLiving *> coldfire_livings;
    std::vector<GW::AgentLiving *> behemoth_livings;
    std::vector<GW::AgentLiving *> dryder_livings;
    std::vector<GW::AgentLiving *> skele_livings;
    std::vector<GW::AgentLiving *> horseman_livings;

    std::vector<uint32_t> king_path_coldfire_ids;

    AutoTargetAction auto_target;
};

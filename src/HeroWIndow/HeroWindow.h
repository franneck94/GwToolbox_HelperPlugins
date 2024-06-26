#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <map>

#include <ToolboxUIPlugin.h>

#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Hero.h>
#include <GWCA/GameEntities/Party.h>

#include "ActionsBase.h"
#include "DataLivings.h"
#include "HelperPlayer.h"
#include "HelperHero.h"

class HeroWindow : public ToolboxUIPlugin
{
public:
    HeroWindow() {};
    ~HeroWindow() {};

    const char *Name() const override
    {
        return "HeroWindow";
    }

    const char *Icon() const override
    {
        return ICON_FA_CROWN;
    }

    void Initialize(ImGuiContext *, ImGuiAllocFns, HMODULE) override;
    void SignalTerminate() override;
    bool CanTerminate() override;
    void Terminate() override;
    bool HasSettings() const override
    {
        return true;
    }
    void LoadSettings(const wchar_t *folder);
    void SaveSettings(const wchar_t *folder);

    void Draw(IDirect3DDevice9 *) override;
    void Update(float delta) override;

    /* MAIN CYCLE FUNCTIONS */

    void UpdateInternalData();
    void ResetData();

    void HeroSmarterFollow_Main();
    void HeroSmarterFlagging_Main();

    /* INTERNAL FUNCTIONS FOLLOW */

    void StartFollowing();
    void StopFollowing();

private:
    void FollowPlayer();
    void HeroFollow_DrawAndLogic(const ImVec2 &im_button_size);
    void HeroFollow_StopConditions();
    void HeroFollow_Flagging();
    void HeroFollow_StartWhileRunning();
    void HeroFollow_StuckCheck();
    void HeroFollow_AttackTarget();

    /* INTERNAL FUNCTIONS BEHAVIOUR */

    void HeroBehaviour_DrawAndLogic(const ImVec2 &im_button_size);
    void ToggleHeroBehaviour();

    /* INTERNAL FUNCTIONS ATTACK */

    void HeroSpike_DrawAndLogic(const ImVec2 &im_button_size);

public:
    GW::HeroBehavior current_hero_behaviour = GW::HeroBehavior::Guard;
    GW::HeroBehavior current_hero_behaviour_before_follow = GW::HeroBehavior::Guard;
    GW::GamePos follow_pos = {};
    long ms_with_no_pos_change = 0;
    long move_time_ms = 0;
    long time_at_last_pos_change;
    uint32_t target_agent_id = 0;
    bool following_active = false;
    bool toggled_follow = false;

    GW::HookEntry AgentCalled_Entry;
    GW::HookEntry GenericValueTarget_Entry;
    uint32_t ping_target_id = 0;

    GW::HookEntry MapLoaded_Entry;
    bool load_cb_triggered = false;

    GW::HookEntry OnSkillActivated_Entry;

    bool show_debug_map = false;
};

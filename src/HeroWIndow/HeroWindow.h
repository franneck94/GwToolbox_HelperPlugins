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
#include "DataHero.h"
#include "DataLivings.h"
#include "DataPlayer.h"

class HeroWindow : public ToolboxUIPlugin
{
public:
    HeroWindow() : player_data({}){};
    ~HeroWindow(){};

    const char *Name() const override
    {
        return "HeroWindow";
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
        return true;
    }
    void Update(float delta) override;

private:
    bool UpdateHeroData();

    void HeroBehaviour_DrawAndLogic(const ImVec2 &im_button_size);

    void HeroSpike_DrawAndLogic(const ImVec2 &im_button_size);

    void HeroFollow_DrawAndLogic(const ImVec2 &im_button_size, bool &toggled_follow);
    void HeroFollow_StopConditions();
    void StopFollowing();

    void SmartUseSkill(const GW::Constants::SkillID skill_id,
                       const GW::Constants::Profession skill_class,
                       const std::string_view skill_name,
                       const long wait_time_ms,
                       const bool use_player_target,
                       std::function<bool(const DataPlayer &, const AgentLivingData &)> player_conditions,
                       std::function<bool(const DataPlayer &, const Hero &)> hero_conditions);
    bool HeroSkill_StartConditions(const GW::Constants::SkillID skill_id, const long wait_time_ms = 0UL);
    void HeroSmarterSkills_Logic();
    void UseBipOnPlayer();
    void UseSplinterOnPlayer();
    void UseHonorOnPlayer();
    void UseShelterAtFightEnter();
    void UseUnionAtFightEnter();
    void UseSosAtFightEnter();
    void ShatterImportantHexes();

    void ToggleHeroBehaviour();
    void FollowPlayer();
    void UseFallback();
    void MesmerSpikeTarget(const Hero &hero_data) const;
    void AttackTarget();
    void ResetData();

    DataPlayer player_data;
    AgentLivingData livings_data;
    HeroData hero_data;

    GW::HeroBehavior current_hero_behaviour = GW::HeroBehavior::Guard;
    GW::GamePos follow_pos = {};
    long ms_with_no_pos_change = 0;
    long time_at_last_pos_change;
    uint32_t target_agent_id = 0;
    bool following_active = false;

    GW::HookEntry MapLoaded_Entry;
    bool load_cb_triggered = false;

    GW::HookEntry OnSkillActivated_Entry;
};

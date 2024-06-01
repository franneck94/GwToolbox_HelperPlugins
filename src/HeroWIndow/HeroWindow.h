#pragma once

#include <cstdint>

#include <ToolboxUIPlugin.h>

#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Hero.h>
#include <GWCA/GameEntities/Party.h>

#include "ActionsBase.h"
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
    uint32_t GetNumPlayerHeroes();
    void ToggleHeroBehaviour();
    void FollowPlayer();
    void UseBipOnPlayer();
    void UseFallback();
    void MesmerSpikeTarget(const GW::HeroPartyMember &hero, const uint32_t hero_idx) const;
    void AttackTarget();
    void ResetData();
    // void OnSkillActivaiton(GW::HookStatus *status, GW::Packet::StoC::GenericValueTarget *packet) const;

    DataPlayer player_data;
    const GW::Array<GW::HeroPartyMember> *party_heros = nullptr;

    GW::HeroBehavior current_hero_behaviour = GW::HeroBehavior::Guard;
    GW::GamePos follow_pos = {};
    uint32_t target_agent_id = 0;
    bool following_active = false;

    GW::HookEntry MapLoaded_Entry;
    bool load_cb_triggered = false;

    // GW::HookEntry OnSkillActivated_Entry;
};

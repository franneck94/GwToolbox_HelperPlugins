#pragma once

#include <cstdint>

#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Party.h>

#include <ActionsBase.h>
#include <Base/HelperBoxWindow.h>
#include <DataPlayer.h>

#include <SimpleIni.h>
#include <imgui.h>

enum class HeroBehaviour : int
{
    AVOID_COMBAT = 2,
    GUARD = 1,
    ATTACK = 0,
};

class HeroWindow : public HelperBoxWindow
{
public:
    HeroWindow() : player_data({})
    {
        GW::StoC::RegisterPacketCallback<GW::Packet::StoC::MapLoaded>(
            &MapLoaded_Entry,
            [this](GW::HookStatus *, GW::Packet::StoC::MapLoaded *) -> void { ResetData(); });
    };
    ~HeroWindow(){};

    static HeroWindow &Instance()
    {
        static HeroWindow instance;
        return instance;
    }

    const char *Name() const override
    {
        return "HeroWindow";
    }

    void Draw() override;
    void Update(float delta, const AgentLivingData &) override;

private:
    uint32_t GetNumPlayerHeroes();
    void ToggleHeroBehaviour();
    void FollowPlayer();
    void UseBipOnPlayer();
    void UseFallback();
    bool MesmerSpikeTarget(const GW::HeroPartyMember &hero);
    void AttackTarget();
    void ResetData();

    DataPlayer player_data;
    const GW::Array<GW::HeroPartyMember> *party_heros = nullptr;

    HeroBehaviour current_hero_behaviour = HeroBehaviour::GUARD;
    GW::GamePos follow_pos = {};
    uint32_t target_agent_id = 0;
    bool following_active = false;

    GW::HookEntry MapLoaded_Entry;
    bool load_cb_triggered = false;
};

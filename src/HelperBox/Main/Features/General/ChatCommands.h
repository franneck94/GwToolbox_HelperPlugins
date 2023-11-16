#pragma once

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Utilities/Hook.h>

#include <Base/HelperBoxModule.h>
#include <Base/HelperBoxUIElement.h>
#include <DataLivings.h>
#include <Utils.h>

class ChatCommands : public HelperBoxModule
{
public:
    ChatCommands(){};
    ~ChatCommands(){};

private:
    struct BaseUseSkill
    {
        uint32_t slot = 0; // 1-8 range
        float skill_usage_delay = 0.0F;
        clock_t skill_timer = clock();

        virtual void Update() = 0;
        void CastSelectedSkill(const uint32_t current_energy,
                               const GW::Skillbar *skillbar,
                               const uint32_t target_id = 0);
    };

    struct UseSkill : public BaseUseSkill
    {
        void Update() override;
    };
    UseSkill useskill;

    struct DhuumUseSkill : public BaseUseSkill
    {
        void Update() override;
    };
    DhuumUseSkill dhuum_useskill;

public:
    static ChatCommands &Instance()
    {
        static ChatCommands instance;
        return instance;
    }

    const char *Name() const override
    {
        return "Chat Commands";
    }
    const char *SettingsName() const override
    {
        return "Chat Settings";
    }

    void Initialize() override;
    void Update(float delta, const AgentLivingData &livings_data) override;

    static void CmdHB(const wchar_t *message, int argc, LPWSTR *argv);
    static void CmdDhuumUseSkill(const wchar_t *, int argc, LPWSTR *argv);
    static void CmdUseSkill(const wchar_t *, int argc, LPWSTR *argv);
};

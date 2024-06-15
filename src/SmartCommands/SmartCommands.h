#pragma once

#include <ToolboxUIPlugin.h>

#include <GWCA/GameEntities/Skill.h>

#include "DataLivings.h"
#include "Utils.h"
#include "UwMetadata.h"

class SmartCommands : public ToolboxUIPlugin
{
public:
    SmartCommands(){};
    ~SmartCommands(){};

public:
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

    struct DhuumUseSkill : public BaseUseSkill
    {
        void Update() override;
    };

public:
    const char *Name() const override
    {
        return "SmartCommands";
    }

    const char *Icon() const override
    {
        return ICON_FA_COMMENT_DOTS;
    }

    void Initialize(ImGuiContext*, ImGuiAllocFns, HMODULE) override;
    void SignalTerminate() override;
    bool CanTerminate() override;
    void Terminate() override;

    void Draw(IDirect3DDevice9 *) override {};
    bool HasSettings() const override
    {
        return false;
    }
    void Update(float delta) override;

    static void CmdDhuumUseSkill(GW::HookStatus *, const wchar_t *, int argc, const LPWSTR *argv);
    static void CmdUseSkill(GW::HookStatus *, const wchar_t *, int argc, const LPWSTR *argv);
};

#include <cstdlib>
#include <string>

#include <GWCA/Context/ItemContext.h>
#include <GWCA/Context/WorldContext.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>

#include "HelperAgents.h"
#include "HelperUw.h"
#include "HelperUwPos.h"
#include "Logger.h"
#include <Base/HelperBox.h>
#include <Base/HelperBoxWindow.h>
#include <Base/MainWindow.h>
#include <Features/Uw/UwMetadata.h>
#include <HelperItems.h>
#include <HelperMaps.h>

#include "ChatCommands.h"

namespace
{
constexpr static auto COOKIE_ID = uint32_t{28433};
}; // namespace

void ChatCommands::Initialize()
{
    HelperBoxModule::Initialize();
    GW::Chat::CreateCommand(L"hb", ChatCommands::CmdHB);
    GW::Chat::CreateCommand(L"use", ChatCommands::CmdUseSkill);
    GW::Chat::CreateCommand(L"dhuum", ChatCommands::CmdDhuumUseSkill);
}

void ChatCommands::Update(float, const AgentLivingData &)
{
    if (!IsMapReady())
    {
        useskill.slot = 0;
        dhuum_useskill.slot = 0;
        return;
    }

    useskill.Update();
    dhuum_useskill.Update();
}

void ChatCommands::CmdHB(const wchar_t *, int argc, LPWSTR *argv)
{
    if (argc == 2)
    {
        const std::wstring arg = argv[1];
        if (arg == L"close" || arg == L"quit" || arg == L"exit")
        {
            HelperBox::Instance().StartSelfDestruct();
            return;
        }
    }

    if (argc < 3)
    {
        const std::wstring arg = argv[1];
        if (arg == L"hide")
        {
            MainWindow::Instance().visible = false;
            return;
        }
        if (arg == L"show")
        {
            MainWindow::Instance().visible = true;
            return;
        }
        if (arg == L"save")
        {
            HelperBox::Instance().SaveSettings();
            return;
        }
        if (arg == L"load")
        {
            HelperBox::Instance().OpenSettingsFile();
            HelperBox::Instance().LoadModuleSettings();
            return;
        }
    }
}

void ChatCommands::BaseUseSkill::CastSelectedSkill(const uint32_t current_energy,
                                                   const GW::Skillbar *skillbar,
                                                   const uint32_t target_id)
{
    const auto lslot = slot - 1;
    const auto &skill = skillbar->skills[lslot];
    const auto skilldata = GW::SkillbarMgr::GetSkillConstantData(skill.skill_id);
    if (!skilldata)
        return;

    const auto enough_energy = current_energy > skilldata->energy_cost;
    const auto enough_adrenaline =
        (skilldata->adrenaline == 0) || (skilldata->adrenaline > 0 && skill.adrenaline_a >= skilldata->adrenaline);
    if (skill.GetRecharge() == 0 && enough_energy && enough_adrenaline)
    {
        if (target_id)
            GW::SkillbarMgr::UseSkill(lslot, target_id);
        else
            GW::SkillbarMgr::UseSkill(lslot, GW::Agents::GetTargetId());
        skill_usage_delay = skilldata->activation + skilldata->aftercast;
        skill_timer = clock();
    }
}

void ChatCommands::UseSkill::Update()
{
    if (slot == 0)
        return;
    if ((clock() - skill_timer) / 1000.0f < skill_usage_delay)
        return;
    const auto skillbar = GW::SkillbarMgr::GetPlayerSkillbar();
    if (!skillbar || !skillbar->IsValid())
    {
        slot = 0;
        return;
    }
    const auto me = GW::Agents::GetPlayer();
    if (!me)
        return;
    const auto me_living = me->GetAsAgentLiving();
    if (!me_living)
        return;

    const auto current_energy = static_cast<uint32_t>(me_living->energy * me_living->max_energy);
    CastSelectedSkill(current_energy, skillbar);
}

void ChatCommands::DhuumUseSkill::Update()
{
    if (slot == 0)
        return;

    const auto me_living = GetPlayerAsLiving();
    if (!me_living || !IsUw() || !IsInDhuumRoom(me_living->pos))
    {
        slot = 0;
        return;
    }

    auto target_id = uint32_t{0};
    const auto target = GetTargetAsLiving();
    if (target && target->allegiance == GW::Constants::Allegiance::Enemy && me_living && !me_living->GetIsAttacking())
        AttackAgent(target);

    if ((clock() - skill_timer) / 1000.0f < skill_usage_delay)
        return;
    const auto skillbar = GW::SkillbarMgr::GetPlayerSkillbar();
    if (!skillbar || !skillbar->IsValid())
    {
        slot = 0;
        return;
    }

    const auto progress_perc = GetProgressValue();
    if (UwMetadata::Instance().num_finished_objectives <= 10 && progress_perc > 0.0F && progress_perc < 1.0F)
    {
        slot = 1;

        const auto item_context = GW::ItemContext::instance();
        const auto world_context = GW::WorldContext::instance();
        if (world_context && item_context)
        {
            static auto last_time_cookie = clock();
            if (TIMER_DIFF(last_time_cookie) > 500 && world_context->morale <= 85)
            {
                UseInventoryItem(COOKIE_ID, 1, item_context->bags_array.size());
                last_time_cookie = clock();
                return;
            }
        }
    }
    else if (progress_perc == 1.0F)
    {
        slot = 5;
        if (target)
            target_id = target->agent_id;
    }
    else
    {
        slot = 0;
        return;
    }

    if (!slot)
        return;

    const auto current_energy = static_cast<uint32_t>((me_living->energy * me_living->max_energy));
    CastSelectedSkill(current_energy, skillbar, target_id);
}

void ChatCommands::CmdDhuumUseSkill(const wchar_t *, int argc, LPWSTR *argv)
{
    if (!IsMapReady() || !IsUw())
        return;

    auto &dhuum_useskill = Instance().dhuum_useskill;

    dhuum_useskill.skill_usage_delay = 0.0F;
    dhuum_useskill.slot = 0;

    if (argc < 2)
    {
        Log::Info("Please enter dhuum start/stop");
        return;
    }
    const auto arg1 = std::wstring{argv[1]};
    if (arg1 != L"start" || arg1 == L"end" || arg1 == L"stop")
    {
        Log::Info("Please enter dhuum start/stop");
        return;
    }

    dhuum_useskill.slot = static_cast<uint32_t>(-1);
}

void ChatCommands::CmdUseSkill(const wchar_t *, int argc, LPWSTR *argv)
{
    if (!IsMapReady() || !IsExplorable())
        return;

    auto &useskill = Instance().useskill;

    useskill.skill_usage_delay = 0.0F;
    useskill.slot = 0;

    if (argc < 2)
        return;

    const auto arg1 = std::wstring{argv[1]};
    const auto arg1_int = _wtoi(arg1.data());
    if (arg1_int == 0 || arg1_int < 1 || arg1_int > 8)
        return;

    useskill.slot = arg1_int;
}

#include <algorithm>
#include <cstdint>
#include <vector>

#include <GWCA/Constants/Maps.h>
#include <GWCA/Context/CharContext.h>
#include <GWCA/Context/GameContext.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/PartyMgr.h>

#include "ActionsBase.h"
#include "HelperPlayer.h"
#include "Helper.h"
#include "HelperAgents.h"
#include "HelperDialogs.h"
#include "HelperUwPos.h"
#include "UtilsMath.h"

#include "HelperUw.h"

bool UwHelperActivationConditions(const bool need_party_loaded)
{
    if (!HelperActivationConditions(need_party_loaded))
        return false;

    if (!IsUwEntryOutpost() && !IsUw())
        return false;

    return true;
}


std::pair<uint32_t, bool> GetTankId()
{
    std::vector<PlayerMapping> party_members;
    const auto success = GetPartyMembers(party_members);
    if (!success)
        return {0, false};

    if (party_members.size() == 1)
        return {0, false};

    auto tank_idx = uint32_t{0};
    switch (GW::Map::GetMapID())
    {
    case GW::Constants::MapID::The_Underworld:
    {
        tank_idx = party_members.size() - 2;
        break;
    }
    default:
    {
        tank_idx = 0;
        break;
    }
    }

    const auto tank = party_members[tank_idx];
    return {tank.id, true};
}

uint32_t GetEmoId()
{
    std::vector<PlayerMapping> party_members;
    const auto success = GetPartyMembers(party_members);
    if (!success)
        return 0;

    for (const auto &member : party_members)
    {
        const auto agent = GW::Agents::GetAgentByID(member.id);
        const auto living = agent ? agent->GetAsAgentLiving() : nullptr;
        if (!living)
            continue;

        if (living->primary == static_cast<uint8_t>(GW::Constants::Profession::Elementalist) &&
            living->secondary == static_cast<uint8_t>(GW::Constants::Profession::Monk))
        {
            return agent->agent_id;
        }
    }

    return 0;
}

uint32_t GetDhuumBitchId()
{
    const auto party_size = GW::PartyMgr::GetPartySize();

    std::vector<PlayerMapping> party_members;
    const auto success = GetPartyMembers(party_members);
    if (!success)
    {
        return 0;
    }

    for (const auto &member : party_members)
    {
        const auto agent = GW::Agents::GetAgentByID(member.id);
        const auto living = agent ? agent->GetAsAgentLiving() : nullptr;
        if (!living)
            continue;

        if (party_size <= 6)
        {
            if (living->secondary == static_cast<uint8_t>(GW::Constants::Profession::Ranger))
                return agent->agent_id;
        }
        else
        {
            if (living->primary == static_cast<uint8_t>(GW::Constants::Profession::Ritualist) &&
                living->secondary == static_cast<uint8_t>(GW::Constants::Profession::Ranger))
                return agent->agent_id;
        }
    }

    return 0;
}

bool IsEmo()
{
    const auto primary = GetPrimaryClass();
    const auto secondary = GetSecondaryClass();

    return (primary == GW::Constants::Profession::Elementalist && secondary == GW::Constants::Profession::Monk);
}

bool IsDhuumBitch()
{
    const auto primary = GetPrimaryClass();
    const auto secondary = GetSecondaryClass();

    return ((primary == GW::Constants::Profession::Ritualist || primary == GW::Constants::Profession::Dervish) &&
            secondary == GW::Constants::Profession::Ranger);
}

bool IsUwMesmer()
{
    const auto primary = GetPrimaryClass();
    const auto secondary = GetSecondaryClass();

    return (primary == GW::Constants::Profession::Mesmer &&
            (secondary == GW::Constants::Profession::Ranger || secondary == GW::Constants::Profession::Elementalist ||
             secondary == GW::Constants::Profession::Assassin));
}

bool IsSpiker()
{
    const auto primary = GetPrimaryClass();
    const auto secondary = GetSecondaryClass();

    return (primary == GW::Constants::Profession::Mesmer && secondary == GW::Constants::Profession::Ranger);
}

bool IsLT()
{
    const auto primary = GetPrimaryClass();
    const auto secondary = GetSecondaryClass();
    if (primary == GW::Constants::Profession::Mesmer && secondary == GW::Constants::Profession::Assassin)
        return true;

    // Check if Me/E has Mantra of Earth => T4 build
    const auto *skillbar = GW::SkillbarMgr::GetPlayerSkillbar();
    if (skillbar)
    {
        for (const auto skill : skillbar->skills)
        {
            if (skill.skill_id == GW::Constants::SkillID::Mantra_of_Earth)
                return false;
        }
    }

    return (primary == GW::Constants::Profession::Mesmer && secondary == GW::Constants::Profession::Elementalist);
}

bool IsRangerTerra()
{
    const auto primary = GetPrimaryClass();
    const auto secondary = GetSecondaryClass();

    return (primary == GW::Constants::Profession::Ranger && secondary == GW::Constants::Profession::Assassin);
}

bool IsMesmerTerra()
{
    const auto primary = GetPrimaryClass();
    const auto secondary = GetSecondaryClass();

    if (primary != GW::Constants::Profession::Mesmer || secondary != GW::Constants::Profession::Elementalist)
        return false;

    return !IsLT();
}

const GW::Agent *GetDhuumAgent()
{
    const auto *agents_array = GW::Agents::GetAgentArray();
    if (!agents_array || !agents_array->valid())
        return nullptr;

    const GW::Agent *dhuum_agent = nullptr;

    for (const auto agent : *agents_array)
    {
        const auto living = agent ? agent->GetAsAgentLiving() : nullptr;
        if (!living)
            continue;

        if (living->player_number == static_cast<uint16_t>(GW::Constants::ModelID::UW::Dhuum))
        {
            dhuum_agent = agent;
            break;
        }
    }

    return dhuum_agent;
}

bool IsInDhuumFight(const GW::GamePos &player_pos)
{
    if (!IsUw())
        return false;

    if (!IsInDhuumRoom(player_pos) && !IsInVale(player_pos)) // Vale for spirits respawn
        return false;

    const auto progress_perc = GetProgressValue();
    if (progress_perc >= 0.0F && progress_perc <= 1.0F)
        return true;

    const auto *dhuum_agent = GetDhuumAgent();
    if (!dhuum_agent)
        return false;

    const auto *dhuum_living = dhuum_agent->GetAsAgentLiving();
    if (!dhuum_living)
        return false;

    return dhuum_living->allegiance == GW::Constants::Allegiance::Enemy;
}

void GetDhuumAgentData(const GW::Agent *dhuum_agent, float &dhuum_hp, uint32_t &dhuum_max_hp)
{
    if (!dhuum_agent)
        return;

    const auto *dhuum_living = dhuum_agent->GetAsAgentLiving();
    if (!dhuum_living)
        return;

    dhuum_hp = dhuum_living->hp;
    dhuum_max_hp = dhuum_living->max_hp;
}

bool TankIsFullteamLT()
{
    const auto [lt_id, has_lt] = GetTankId();
    if (!lt_id || !has_lt)
        return false;

    const auto *lt_agent = GW::Agents::GetAgentByID(lt_id);
    if (!lt_agent)
        return false;

    const auto *lt_living = lt_agent->GetAsAgentLiving();
    if (!lt_living)
        return false;

    if (lt_living->primary == static_cast<uint8_t>(GW::Constants::Profession::Mesmer) &&
        lt_living->secondary == static_cast<uint8_t>(GW::Constants::Profession::Assassin))
        return true;

    return false;
}

bool TankIsSoloLT()
{
    const auto [lt_id, has_lt] = GetTankId();
    if (!lt_id || !has_lt)
        return false;

    const auto lt_agent = GW::Agents::GetAgentByID(lt_id);
    if (!lt_agent)
        return false;

    const auto lt_living = lt_agent->GetAsAgentLiving();
    if (!lt_living)
        return false;

    if (lt_living->primary == static_cast<uint8_t>(GW::Constants::Profession::Mesmer) &&
        lt_living->secondary == static_cast<uint8_t>(GW::Constants::Profession::Elementalist))
        return true;

    return false;
}

bool TargetIsReaper()
{
    if (!GW::Agents::GetTarget())
        return false;

    const auto *living_target = GW::Agents::GetTarget()->GetAsAgentLiving();

    if (!living_target)
        return false;

    return living_target->player_number == static_cast<uint32_t>(GW::Constants::ModelID::UW::Reapers);
}

bool TargetReaper(const std::vector<GW::AgentLiving *> &npcs)
{
    return TargetClosestNpcById(npcs, GW::Constants::ModelID::UW::Reapers) != 0U;
}

bool TalkReaper(const std::vector<GW::AgentLiving *> &npcs)
{
    const auto npc_id = TargetClosestNpcById(npcs, GW::Constants::ModelID::UW::Reapers);
    if (!npc_id)
        return true;

    const auto agent = GW::Agents::GetAgentByID(npc_id);
    if (!agent)
        return true;

    GW::Agents::InteractAgent(agent, 0U);

    return true;
}

bool TargetClosestKeeper(const std::vector<GW::AgentLiving *> enemies)
{
    return TargetClosestEnemyById(enemies, GW::Constants::ModelID::UW::KeeperOfSouls) != 0;
}

bool TakeChamber()
{
    const auto dialog = QuestAcceptDialog(GW::Constants::QuestID::UW_Chamber);
    return GW::Agents::SendDialog(dialog);
}

bool AcceptChamber()
{
    const auto dialog = QuestRewardDialog(GW::Constants::QuestID::UW_Chamber);
    return GW::Agents::SendDialog(dialog);
}

bool TakeRestore()
{
    const auto dialog = QuestAcceptDialog(GW::Constants::QuestID::UW_Restore);
    GW::Agents::SendDialog(dialog);
    return true;
}

bool TakeEscort()
{
    const auto dialog = QuestAcceptDialog(GW::Constants::QuestID::UW_Escort);
    GW::Agents::SendDialog(dialog);
    return true;
}

bool TakeUWG()
{
    const auto dialog = QuestAcceptDialog(GW::Constants::QuestID::UW_UWG);
    GW::Agents::SendDialog(dialog);
    return true;
}

bool TakePits()
{
    const auto dialog = QuestAcceptDialog(GW::Constants::QuestID::UW_Pits);
    GW::Agents::SendDialog(dialog);
    return true;
}

bool TakePlanes()
{
    const auto dialog = QuestAcceptDialog(GW::Constants::QuestID::UW_Planes);
    GW::Agents::SendDialog(dialog);
    return true;
}

bool FoundKeeperAtPos(const std::vector<GW::AgentLiving *> &keeper_livings, const GW::GamePos &keeper_pos)
{
    auto found_keeper = false;

    for (const auto keeper : keeper_livings)
    {
        if (GW::GetDistance(keeper->pos, keeper_pos) < GW::Constants::Range::Earshot)
        {
            found_keeper = true;
            break;
        }
    }

    return found_keeper;
}

bool DhuumIsCastingJudgement(const GW::Agent *dhuum_agent)
{
    if (!dhuum_agent)
        return false;

    const auto *dhuum_living = dhuum_agent->GetAsAgentLiving();
    if (!dhuum_living)
        return false;

    if (dhuum_living->GetIsCasting() && dhuum_living->skill == static_cast<uint32_t>(3085))
        return true;

    return false;
}

bool CheckForAggroFree(const AgentLivingData *livings_data, const GW::GamePos &next_pos)
{
    if (!livings_data)
        return true;

    const auto filter_ids = std::set<uint32_t>{GW::Constants::ModelID::UW::SkeletonOfDhuum1,
                                               GW::Constants::ModelID::UW::SkeletonOfDhuum2,
                                               GW::Constants::ModelID::UW::TerrorwebDryder,
                                               GW::Constants::ModelID::UW::GraspingDarkness,
                                               GW::Constants::ModelID::UW::DyingNightmare};

    const auto livings = FilterAgentsByRange(livings_data->enemies, GW::Constants::Range::Spellcast);
    const auto result_ids_aggro = FilterAgentIDS(livings, filter_ids);

    const auto player_pos = GetPlayerPos();
    if (player_pos.x == next_pos.x && player_pos.y == next_pos.y)
        return result_ids_aggro.size() == 0;

    if (result_ids_aggro.size() > 0)
        return false;

    const auto rect = GameRectangle(player_pos, next_pos, GW::Constants::Range::Spellcast + 50.0F);
    const auto filtered_livings = GetEnemiesInGameRectangle(rect, livings_data->enemies);

    const auto result_ids_rect = FilterAgentIDS(filtered_livings, filter_ids);
    return result_ids_rect.size() == 0;
}

float GetProgressValue()
{
    auto *game_context = GW::GetGameContext();
    const auto *char_context = game_context->character;

    if (!char_context || !char_context->progress_bar)
        return 0.0F;

    return char_context->progress_bar->progress;
}

bool DhuumFightDone(const uint32_t num_objectives)
{
    constexpr static auto num_uw_quests = 10U;

    if (num_objectives <= num_uw_quests)
        return false;

    const auto progress_perc = GetProgressValue();
    return num_objectives > num_uw_quests || progress_perc < 0.0F || progress_perc > 1.0F;
}

uint32_t GetUwTriggerRoleId(const TriggerRole role)
{
    uint32_t trigger_id = 0U;

    switch (role)
    {
    case TriggerRole::LT:
    {
        const auto [lt_id, has_lt] = GetTankId();
        trigger_id = lt_id;
        break;
    }
    case TriggerRole::EMO:
    {
        trigger_id = GetEmoId();
        break;
    }
    case TriggerRole::DB:
    {
        trigger_id = GetDhuumBitchId();
        break;
    }
    }

    return trigger_id;
}

bool TargetTrigger(const TriggerRole role)
{
    const auto trigger_id = GetUwTriggerRoleId(role);

    if (!trigger_id)
        return false;

    ChangeTarget(trigger_id);

    return true;
}

bool LtIsBonded()
{
    const auto [lt_id, has_lt] = GetTankId();

    if (!has_lt)
        return true;

    const auto has_prot = AgentHasBuff(GW::Constants::SkillID::Protective_Bond, lt_id);
    const auto has_life = AgentHasBuff(GW::Constants::SkillID::Life_Bond, lt_id);
    const auto has_ballth = AgentHasBuff(GW::Constants::SkillID::Balthazars_Spirit, lt_id);

    return (has_prot && has_life && has_ballth);
}

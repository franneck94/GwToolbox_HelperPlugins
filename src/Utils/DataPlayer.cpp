#include <utility>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Context/ItemContext.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Managers/EffectMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/MemoryMgr.h>
#include <GWCA/Managers/PartyMgr.h>

#include "ActionsBase.h"
#include "DataSkillbar.h"
#include "Helper.h"
#include "HelperAgents.h"

#include "DataPlayer.h"

namespace
{
DWORD GetTimeElapsed(const DWORD timestamp)
{
    return GW::MemoryMgr::GetSkillTimer() - timestamp;
}

float GetTimeRemaining(const float duration, const DWORD timestamp)
{
    return duration * 1000.0F - static_cast<float>(GetTimeElapsed(timestamp));
}
} // namespace

bool DataPlayer::ValidateData(std::function<bool(bool)> cb_fn, const bool need_party_loaded) const
{
    if (!cb_fn(need_party_loaded))
        return false;

    const auto *const me_agent = GW::Agents::GetPlayer();
    const auto *const me_living = GW::Agents::GetPlayerAsAgentLiving();

    return (me_agent != nullptr && me_living != nullptr);
}

void DataPlayer::Update()
{
    auto *me_agent = GW::Agents::GetPlayer();
    auto *me_living = GW::Agents::GetPlayerAsAgentLiving();

    id = me_agent->agent_id;
    pos = me_living->pos;

    me = me_agent;
    living = me_living;

    dead = living->GetIsDead() || living->hp == 0.0F || !living->GetIsAlive();

    target = GW::Agents::GetTarget();

    const auto energy_tpl = GetEnergy();
    energy = std::get<0>(energy_tpl);
    max_energy = std::get<1>(energy_tpl);
    energy_perc = std::get<2>(energy_tpl);

    const auto hp_tpl = GetHp();
    hp = std::get<0>(hp_tpl);
    max_hp = std::get<1>(hp_tpl);
    hp_perc = std::get<2>(hp_tpl);

    primary = static_cast<GW::Constants::Profession>(living->primary);
    secondary = static_cast<GW::Constants::Profession>(living->secondary);

    const auto equipped_items_bag = GW::Items::GetBag(GW::Constants::Bag::Equipped_Items);
    if (!equipped_items_bag)
        return;
    weapon_main_hand = GW::Items::GetItemBySlot(equipped_items_bag, 1);

    holds_melee_weapon = weapon_main_hand && (weapon_main_hand->type == GW::Constants::ItemType::Axe ||
                                              weapon_main_hand->type == GW::Constants::ItemType::Hammer ||
                                              weapon_main_hand->type == GW::Constants::ItemType::Sword ||
                                              weapon_main_hand->type == GW::Constants::ItemType::Daggers ||
                                              weapon_main_hand->type == GW::Constants::ItemType::Scythe ||
                                              weapon_main_hand->type == GW::Constants::ItemType::Spear);
    holds_caster_weapon = !holds_melee_weapon;
}

bool DataPlayer::CanCast() const
{
    if (living->GetIsDead() || living->GetIsKnockedDown() || living->GetIsCasting() || living->GetIsMoving())
        return false;

    return true;
}

bool DataPlayer::CanAttack() const
{
    if (living->GetIsDead() || living->GetIsKnockedDown() || living->GetIsCasting() || living->GetIsMoving())
        return false;

    return true;
}

bool DataPlayer::IsAttacking() const
{
    return living->GetIsAttacking();
}

bool DataPlayer::HasBuff(const GW::Constants::SkillID buff_skill_id) const
{
    const auto *const me_buffs = GW::Effects::GetPlayerBuffs();
    if (!me_buffs || !me_buffs->valid())
        return false;

    for (const auto &buff : *me_buffs)
    {
        const auto agent_id = buff.target_agent_id;
        const auto skill_id = buff.skill_id;

        if (agent_id == id)
        {
            if (skill_id == buff_skill_id)
            {
                return true;
            }
        }
    }

    return false;
}

bool DataPlayer::HasEffect(const GW::Constants::SkillID effect_skill_id) const
{
    const auto *const me_effects = GW::Effects::GetPlayerEffectsArray();
    if (!me_effects)
        return false;

    for (const auto &effect : me_effects->effects)
    {
        const auto agent_id = effect.agent_id;
        const auto skill_id = effect.skill_id;

        if (agent_id == id || agent_id == 0)
        {
            if (skill_id == effect_skill_id)
            {
                return true;
            }
        }
    }

    return false;
}

uint32_t DataPlayer::GetNumberOfPartyBonds() const
{
    const auto *effects = GW::Effects::GetPartyEffectsArray();
    if (!effects || !effects->valid())
        return false;

    const auto &buffs = (*effects)[0].buffs;
    if (!buffs.valid())
        return false;

    auto num_bonds = uint32_t{0};
    for (size_t i = 0; i < buffs.size(); ++i)
    {
        const auto agent_id = buffs[i].target_agent_id;

        if (agent_id != id)
            ++num_bonds;
    }

    return num_bonds;
}

float DataPlayer::GetRemainingEffectDuration(const GW::Constants::SkillID effect_skill_id) const
{
    const auto *me_effects = GW::Effects::GetPlayerEffectsArray();
    if (!me_effects)
        return false;

    for (const auto &effect : me_effects->effects)
    {
        const auto agent_id = effect.agent_id;
        const auto skill_id = effect.skill_id;

        if (agent_id == id || agent_id == 0)
        {
            if (skill_id == effect_skill_id)
            {
                return GetTimeRemaining(effect.duration, effect.timestamp);
            }
        }
    }

    return 0.0F;
}

bool DataPlayer::CastEffectIfNotAvailable(const DataSkill &skill_data) const
{
    const auto has_bond = HasEffect(static_cast<GW::Constants::SkillID>(skill_data.id));
    const auto bond_avail = skill_data.CanBeCasted(energy);

    if (!has_bond && bond_avail)
    {
        GW::GameThread::Enqueue([&]() { GW::SkillbarMgr::UseSkill(skill_data.idx, id); });
        return true;
    }

    return false;
}

bool DataPlayer::CastEffect(const DataSkill &skill_data) const
{
    if (skill_data.CanBeCasted(energy))
    {
        GW::GameThread::Enqueue([&]() { GW::SkillbarMgr::UseSkill(skill_data.idx, id); });
        return true;
    }

    return false;
}

void DataPlayer::ChangeTarget(const uint32_t target_id)
{
    if (!GW::Agents::GetAgentByID(target_id))
        return;

    GW::GameThread::Enqueue([&, target_id] {
        GW::Agents::ChangeTarget(target_id);
        target = GW::Agents::GetTarget();
    });
}


bool DataPlayer::SkillStoppedCallback(const GW::Packet::StoC::GenericValue *const packet) const noexcept
{
    const auto value_id = packet->value_id;
    const auto caster_id = packet->agent_id;

    if (caster_id != id)
        return false;

    if (value_id == GW::Packet::StoC::GenericValueID::skill_stopped)
        return true;

    return false;
}

bool DataPlayer::AnyTeamMemberHasEffect(const GW::Constants::SkillID effect_id) const
{
    const auto *effects = GetEffects(id);
    if (!effects)
        return false;

    for (const auto effect : *effects)
    {
        if (effect.skill_id == effect_id)
            return true;
    }

    return false;
}

bool DataPlayer::PlayerHasEffect(const GW::Constants::SkillID effect_id) const
{
    const auto *effects = GetEffects(id);
    if (!effects || effects->size() == 0)
        return false;

    for (const auto effect : *effects)
    {
        if (effect.skill_id == effect_id && (effect.agent_id == id || effect.agent_id == 0))
            return true;
    }

    return false;
}

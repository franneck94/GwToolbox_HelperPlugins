#include <cstdint>
#include <utility>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/ItemIDs.h>
#include <GWCA/Context/ItemContext.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Item.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/CtoSMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Packets/Opcodes.h>

#include <Helper.h>

#include "HelperItems.h"

bool IsWeapon(const GW::Item *item)
{
    if (!item)
        return false;

    return IsMeleeWeapon(item) || IsOffhandWeapon(item) || IsRangeWeapon(item);
}

bool IsMeleeWeapon(const GW::Item *item)
{
    if (!item)
        return false;

    switch (static_cast<GW::Constants::ItemType>(item->type))
    {
    case GW::Constants::ItemType::Axe:
    case GW::Constants::ItemType::Sword:
    case GW::Constants::ItemType::Scythe:
    case GW::Constants::ItemType::Daggers:
    case GW::Constants::ItemType::Hammer:
        return true;
    default:
        return false;
    }
}

bool IsOffhandWeapon(const GW::Item *item)
{
    if (!item)
        return false;

    switch (static_cast<GW::Constants::ItemType>(item->type))
    {
    case GW::Constants::ItemType::Shield:
    case GW::Constants::ItemType::Offhand:
        return true;
    default:
        return false;
    }
}

bool IsRangeWeapon(const GW::Item *item)
{
    if (!item)
        return false;

    switch (static_cast<GW::Constants::ItemType>(item->type))
    {
    case GW::Constants::ItemType::Bow:
    case GW::Constants::ItemType::Wand:
    case GW::Constants::ItemType::Staff:
    case GW::Constants::ItemType::Spear:
        return true;
    default:
        return false;
    }
}

bool IsArmor(const GW::Item *item)
{
    if (!item)
        return false;

    switch (static_cast<GW::Constants::ItemType>(item->type))
    {
    case GW::Constants::ItemType::Headpiece:
    case GW::Constants::ItemType::Chestpiece:
    case GW::Constants::ItemType::Leggings:
    case GW::Constants::ItemType::Boots:
    case GW::Constants::ItemType::Gloves:
        return true;
    default:
        return false;
    }
}

bool IsSalvagable(const GW::Item *item)
{
    if (IsWeapon(item) || IsArmor(item))
        return true;

    switch (static_cast<GW::Constants::ItemType>(item->type))
    {
    case GW::Constants::ItemType::Salvage:
        return true;
    default:
        break;
    }

    return false;
}

bool IsEquippable(const GW::Item *item)
{
    return (IsWeapon(item) || IsArmor(item));
}

GW::Item *GetBagItem(const uint32_t bag_idx, const uint32_t slot_idx)
{
    GW::Item *item = nullptr;

    if (bag_idx < 1 || bag_idx > 5 || slot_idx < 1 || slot_idx > 25)
        return nullptr;

    const auto bags = GW::Items::GetBagArray();
    if (!bags)
        return nullptr;
    const auto bag = bags[bag_idx];
    if (!bag)
        return nullptr;

    auto &items = bag->items;
    if (!items.valid() || slot_idx > items.size())
        return nullptr;
    item = items.at(slot_idx - 1);

    return item;
}

bool EquipItem(const uint32_t bag_idx, const uint32_t slot_idx)
{
    const auto item = GetBagItem(bag_idx, slot_idx);
    if (!item)
        return false;

    if (!IsEquippable(item))
        return false;

    if (!item || !item->item_id)
        return false;

    if (item->bag && item->bag->bag_type == 2)
        return false;

    const auto p = GW::Agents::GetCharacter();
    if (!p || p->GetIsDead() || p->GetIsKnockedDown() || p->GetIsCasting())
        return false;

    if (p->GetIsIdle() || p->GetIsMoving())
    {
        GW::Items::EquipItem(item);
        return true;
    }
    else
    {
        GW::Agents::Move(p->pos);
        return false;
    }
}

bool ArmorSwap(const uint32_t bag_idx, const uint32_t start_slot_idx, const bool low_armor)
{
    if (static_cast<uint32_t>(-1) == bag_idx || static_cast<uint32_t>(-1) == start_slot_idx)
        return true;

    const auto first_item = GetBagItem(bag_idx, start_slot_idx);
    if (!first_item)
        return true;

    if (first_item->mod_struct_size >= 10)
    {
        const auto armor_value = first_item->mod_struct[9].arg1();

        if (low_armor && armor_value >= 60U)
            return true;
        else if (!low_armor && armor_value < 60U)
            return true;
    }

    for (uint32_t offset = 0U; offset < 5U; offset++)
        EquipItem(bag_idx, start_slot_idx + offset);

    return true;
}

bool LowArmor(const uint32_t bag_idx, const uint32_t start_slot_idx)
{
    return ArmorSwap(bag_idx, start_slot_idx, true);
}

bool HighArmor(const uint32_t bag_idx, const uint32_t start_slot_idx)
{
    return ArmorSwap(bag_idx, start_slot_idx, false);
}

bool UseInventoryItem(const uint32_t item_id, const size_t from_bag, const size_t to_bag)
{
    auto bags = GW::Items::GetBagArray();
    if (!bags)
        return false;

    if (to_bag < from_bag || !to_bag || !from_bag)
        return false;

    for (size_t bagIndex = from_bag; bagIndex <= to_bag; bagIndex++)
    {
        auto bag = bags[bagIndex];
        if (!bag)
            continue;
        auto &items = bag->items;
        if (!items.valid())
            continue;

        for (const auto item : items)
        {
            if (!item)
                continue;

            if (item->model_id == item_id)
            {
                GW::Items::UseItem(item);
                return true;
            }
        }
    }

    return false;
}

GW::WeapondSet *GetWeaponSets()
{
    const auto *c = GW::ItemContext::instance();

    if (!c || !c->inventory)
        return nullptr;

    return c->inventory->weapon_sets;
}

GW::WeapondSet *GetActiveWeaponSet()
{
    const auto *c = GW::ItemContext::instance();

    if (!c || !c->inventory)
        return nullptr;

    return &c->inventory->weapon_sets[c->inventory->active_weapon_set];
}

std::pair<GW::WeapondSet *, uint32_t> GetFirstRangeWeaponSet()
{
    const auto sets = GetWeaponSets();

    for (size_t i = 0; i < 4; ++i)
    {
        if (WeaponSetIsRange(sets[i]))
            return std::make_pair(&sets[i], i);
    }

    return std::make_pair(nullptr, 0);
}

std::pair<GW::WeapondSet *, uint32_t> GetFirstMeleeWeaponSet()
{
    const auto sets = GetWeaponSets();

    for (size_t i = 0; i < 4; ++i)
    {
        if (WeaponSetIsMelee(sets[i]))
            return std::make_pair(&sets[i], i);
    }

    return std::make_pair(nullptr, 0);
}

bool WeaponSetIsMelee(const GW::WeapondSet &weapon_set)
{
    if (!weapon_set.weapond)
        return false;

    return IsMeleeWeapon(weapon_set.weapond);
}

bool WeaponSetIsRange(const GW::WeapondSet &weapon_set)
{
    if (!weapon_set.weapond)
        return false;

    return IsRangeWeapon(weapon_set.weapond);
}

bool UseWeaponSlot(const uint32_t slot_idx)
{
    auto action = GW::UI::ControlAction::ControlAction_ActivateWeaponSet1;

    switch (slot_idx)
    {
    case 0:
    {
        action = GW::UI::ControlAction::ControlAction_ActivateWeaponSet1;
        break;
    }
    case 1:
    {
        action = GW::UI::ControlAction::ControlAction_ActivateWeaponSet2;
        break;
    }
    case 2:
    {
        action = GW::UI::ControlAction::ControlAction_ActivateWeaponSet3;
        break;
    }
    case 3:
    {
        action = GW::UI::ControlAction::ControlAction_ActivateWeaponSet4;
        break;
    }
    default:
    {
        return false;
    }
    }

    GW::GameThread::Enqueue([&]() { GW::UI::Keypress(action); });
    return true;
}

bool SwapToMeleeSet()
{
    const auto active_set = GetActiveWeaponSet();
    if (WeaponSetIsMelee(*active_set))
        return true;

    const auto new_set = GetFirstMeleeWeaponSet();
    if (new_set.first && new_set.second)
    {
        UseWeaponSlot(new_set.second);
        return true;
    }

    return false;
}

bool SwapToRangeSet()
{
    const auto active_set = GetActiveWeaponSet();
    if (WeaponSetIsRange(*active_set))
        return true;

    const auto new_set = GetFirstRangeWeaponSet();
    if (new_set.first)
    {
        UseWeaponSlot(new_set.second);
        return true;
    }

    return false;
}

GW::ItemModifier *GetModifier(const GW::Item *const item, const uint32_t identifier)
{
    for (size_t i = 0; i < item->mod_struct_size; i++)
    {
        GW::ItemModifier *mod = &item->mod_struct[i];
        if (mod->identifier() == identifier)
            return mod;
    }
    return nullptr;
}

bool IsSparkly(const GW::Item *const item)
{
    return (item->interaction & 0x2000) == 0;
}

bool GetIsIdentified(const GW::Item *const item)
{
    return (item->interaction & 1) != 0;
}

bool IsStackable(const GW::Item *const item)
{
    return (item->interaction & 0x80000) != 0;
}

bool IsUsable(const GW::Item *const item)
{
    return (item->interaction & 0x1000000) != 0;
}

bool IsTradable(const GW::Item *const item)
{
    return (item->interaction & 0x100) == 0;
}

bool IsInscription(const GW::Item *const item)
{
    return (item->interaction & 0x25000000) == 0x25000000;
}

bool IsBlue(const GW::Item *const item)
{
    return item->single_item_name && item->single_item_name[0] == 0xA3F;
}

bool IsPurple(const GW::Item *const item)
{
    return (item->interaction & 0x400000) != 0;
}

bool IsGreen(const GW::Item *const item)
{
    return (item->interaction & 0x10) != 0;
}

bool IsGold(const GW::Item *const item)
{
    return (item->interaction & 0x20000) != 0;
}

uint32_t GetUses(const GW::Item *const item)
{
    GW::ItemModifier *mod = GetModifier(item, 0x2458);
    return mod ? mod->arg2() : 0;
}

bool IsIdentificationKit(const GW::Item *const item)
{
    GW::ItemModifier *mod = GetModifier(item, 0x25E8);
    return mod && mod->arg1() == 1;
}

bool IsLesserKit(const GW::Item *const item)
{
    GW::ItemModifier *mod = GetModifier(item, 0x25E8);
    return mod && mod->arg1() == 3;
}

bool IsExpertSalvageKit(const GW::Item *const item)
{
    GW::ItemModifier *mod = GetModifier(item, 0x25E8);
    return mod && mod->arg1() == 2;
}

bool IsPerfectSalvageKit(const GW::Item *const item)
{
    GW::ItemModifier *mod = GetModifier(item, 0x25E8);
    return mod && mod->arg1() == 6;
}

bool IsRareMaterial(const GW::Item *const item)
{
    GW::ItemModifier *mod = GetModifier(item, 0x2508);
    return mod && mod->arg1() > 11;
}

GW::Constants::Rarity GetRarity(const GW::Item *const item)
{
    if (IsGreen(item))
        return GW::Constants::Rarity::Green;
    if (IsGold(item))
        return GW::Constants::Rarity::Gold;
    if (IsPurple(item))
        return GW::Constants::Rarity::Purple;
    if (IsBlue(item))
        return GW::Constants::Rarity::Blue;

    return GW::Constants::Rarity::White;
}

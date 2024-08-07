#include <cstdint>
#include <utility>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Constants/ItemIDs.h>
#include <GWCA/Context/ItemContext.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Item.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/Managers/UIMgr.h>

#include "Helper.h"

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

    const auto item_type = static_cast<GW::Constants::ItemType>(item->type);

    return item_type == GW::Constants::ItemType::Axe || item_type == GW::Constants::ItemType::Sword ||
           item_type == GW::Constants::ItemType::Scythe || item_type == GW::Constants::ItemType::Daggers ||
           item_type == GW::Constants::ItemType::Hammer;
}

bool IsOffhandWeapon(const GW::Item *item)
{
    if (!item)
        return false;

    const auto item_type = static_cast<GW::Constants::ItemType>(item->type);

    return item_type == GW::Constants::ItemType::Shield || item_type == GW::Constants::ItemType::Offhand;
}

bool IsRangeWeapon(const GW::Item *item)
{
    if (!item)
        return false;

    const auto item_type = static_cast<GW::Constants::ItemType>(item->type);

    return item_type == GW::Constants::ItemType::Bow || item_type == GW::Constants::ItemType::Wand ||
           item_type == GW::Constants::ItemType::Staff || item_type == GW::Constants::ItemType::Spear;
}

bool IsArmor(const GW::Item *item)
{
    if (!item)
        return false;

    const auto item_type = static_cast<GW::Constants::ItemType>(item->type);

    return item_type == GW::Constants::ItemType::Headpiece || item_type == GW::Constants::ItemType::Chestpiece ||
           item_type == GW::Constants::ItemType::Leggings || item_type == GW::Constants::ItemType::Boots ||
           item_type == GW::Constants::ItemType::Gloves;
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

    const auto *bags = GW::Items::GetBagArray();
    if (!bags)
        return nullptr;
    const auto *bag = bags[bag_idx];
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
    const auto *item = GetBagItem(bag_idx, slot_idx);
    if (!item)
        return false;

    if (!IsEquippable(item))
        return false;

    if (!item || !item->item_id)
        return false;

    if (item->bag && item->bag->bag_type == GW::Constants::BagType::Equipped)
        return false;

    const auto *character = GW::Agents::GetCharacter();
    if (!character || character->GetIsDead() || character->GetIsKnockedDown() || character->GetIsCasting())
        return false;

    if (character->GetIsIdle() || character->GetIsMoving())
    {
        GW::Items::EquipItem(item);
        return true;
    }
    else
    {
        GW::Agents::Move(character->pos);
        return false;
    }
}

bool ArmorSwap(const uint32_t bag_idx, const uint32_t start_slot_idx, const bool low_armor)
{
    if (static_cast<uint32_t>(-1) == bag_idx || static_cast<uint32_t>(-1) == start_slot_idx)
        return true;

    const auto *first_item = GetBagItem(bag_idx, start_slot_idx);
    if (!first_item)
        return true;

    if (first_item->mod_struct_size >= 10)
    {
        const auto armor_value = first_item->mod_struct[9].arg1();

        if (low_armor && armor_value >= 60U)
            return true;

        if (!low_armor && armor_value < 60U)
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

GW::WeaponSet *GetWeaponSets()
{
    const auto *ctx = GW::GetItemContext();

    if (!ctx || !ctx->inventory)
        return nullptr;

    return ctx->inventory->weapon_sets;
}

GW::WeaponSet *GetActiveWeaponSet()
{
    const auto *ctx = GW::GetItemContext();

    if (!ctx || !ctx->inventory)
        return nullptr;

    return &ctx->inventory->weapon_sets[ctx->inventory->active_weapon_set];
}

std::pair<GW::WeaponSet *, uint32_t> GetFirstRangeWeaponSet()
{
    const auto sets = GetWeaponSets();

    for (size_t i = 0; i < 4; ++i)
    {
        if (WeaponSetIsRange(sets[i]))
            return std::make_pair(&sets[i], i);
    }

    return std::make_pair(nullptr, 0);
}

std::pair<GW::WeaponSet *, uint32_t> GetFirstMeleeWeaponSet()
{
    const auto sets = GetWeaponSets();

    for (size_t i = 0; i < 4; ++i)
    {
        if (WeaponSetIsMelee(sets[i]))
            return std::make_pair(&sets[i], i);
    }

    return std::make_pair(nullptr, 0);
}

bool WeaponSetIsMelee(const GW::WeaponSet &weapon_set)
{
    if (!weapon_set.weapon)
        return false;

    return IsMeleeWeapon(weapon_set.weapon);
}

bool WeaponSetIsRange(const GW::WeaponSet &weapon_set)
{
    if (!weapon_set.weapon)
        return false;

    return IsRangeWeapon(weapon_set.weapon);
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
    const auto *active_set = GetActiveWeaponSet();
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
    const auto *active_set = GetActiveWeaponSet();
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

#pragma once

#include <cstdint>

#include <GWCA/Constants/ItemIDs.h>
#include <GWCA/GameEntities/Item.h>

namespace GW
{
namespace Constants
{
enum class Rarity
{
    White,
    Blue,
    Purple,
    Gold,
    Green
};
}
} // namespace GW

bool IsWeapon(const GW::Item *item);

bool IsMeleeWeapon(const GW::Item *item);

bool IsOffhandWeapon(const GW::Item *item);

bool IsRangeWeapon(const GW::Item *item);

bool IsArmor(const GW::Item *item);

bool IsSalvagable(const GW::Item *item);

bool IsEquippable(const GW::Item *item);

bool EquipItem(const uint32_t bag_idx, const uint32_t slot_idx);

bool ArmorSwap(const uint32_t bag_idx, const uint32_t start_slot_idx, const uint32_t armor_threshold);

bool LowArmor(const uint32_t bag_idx, const uint32_t start_slot_idx);

bool HighArmor(const uint32_t bag_idx, const uint32_t start_slot_idx);

bool UseInventoryItem(const uint32_t item_id, const size_t from_bag = 1U, const size_t to_bag = 5U);

bool WeaponSetIsMelee(const GW::WeapondSet &weapon_set);

bool WeaponSetIsRange(const GW::WeapondSet &weapon_set);

GW::WeapondSet *GetActiveWeaponSet();

GW::WeapondSet *GetWeaponSets();

std::pair<GW::WeapondSet *, uint32_t> GetFirstRangeWeaponSet();

std::pair<GW::WeapondSet *, uint32_t> GetFirstMeleeWeaponSet();

bool UseWeaponSlot(const uint32_t slot_idx);

bool SwapToMeleeSet();

bool SwapToRangeSet();

GW::ItemModifier *GetModifier(const GW::Item *const item, const uint32_t identifier);

bool IsSparkly(const GW::Item *const item);

bool GetIsIdentified(const GW::Item *const item);

bool IsStackable(const GW::Item *const item);

bool IsUsable(const GW::Item *const item);

bool IsTradable(const GW::Item *const item);

bool IsInscription(const GW::Item *const item);

bool IsBlue(const GW::Item *const item);

bool IsPurple(const GW::Item *const item);

bool IsGreen(const GW::Item *const item);

bool IsGold(const GW::Item *const item);

uint32_t GetUses(const GW::Item *const item);

bool IsIdentificationKit(const GW::Item *const item);

bool IsLesserKit(const GW::Item *const item);

bool IsExpertSalvageKit(const GW::Item *const item);

bool IsPerfectSalvageKit(const GW::Item *const item);

bool IsRareMaterial(const GW::Item *const item);

GW::Constants::Rarity GetRarity(const GW::Item *const item);

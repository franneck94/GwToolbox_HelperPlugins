#pragma once

#include <cstdint>

#include <GWCA/Constants/Constants.h>

constexpr static const char *const questnames[] = {"UW - Chamber",
                                                   "UW - Wastes",
                                                   "UW - UWG",
                                                   "UW - Mnt",
                                                   "UW - Pits",
                                                   "UW - Planes",
                                                   "UW - Pools",
                                                   "UW - Escort",
                                                   "UW - Restore",
                                                   "UW - Vale",
                                                   "FoW - Defend",
                                                   "FoW - Army Of Darkness",
                                                   "FoW - WailingLord",
                                                   "FoW - Griffons",
                                                   "FoW - Slaves",
                                                   "FoW - Restore",
                                                   "FoW - Hunt",
                                                   "FoW - Forgemaster",
                                                   "FoW - Tos",
                                                   "FoW - Toc",
                                                   "FoW - Khobay",
                                                   "DoA - Gloom 1: Deathbringer Company",
                                                   "DoA - Gloom 2: The Rifts Between Us",
                                                   "DoA - Gloom 3: To The Rescue",
                                                   "DoA - City",
                                                   "DoA - Veil 1: Breaching Stygian Veil",
                                                   "DoA - Veil 2: Brood Wars",
                                                   "DoA - Foundry 1: Foundry Of Failed Creations",
                                                   "DoA - Foundry 2: Foundry Breakout"};

constexpr static const char *const dialognames[] = {
    "Craft fow armor",
    "Prof Change - Warrior",
    "Prof Change - Ranger",
    "Prof Change - Monk",
    "Prof Change - Necro",
    "Prof Change - Mesmer",
    "Prof Change - Elementalist",
    "Prof Change - Assassin",
    "Prof Change - Ritualist",
    "Prof Change - Paragon",
    "Prof Change - Dervish",
    "Kama -> Docks @ Hahnna",
    "Docks -> Kaineng @ Mhenlo",
    "Docks -> LA Gate @ Mhenlo",
    "LA Gate -> LA @ Neiro",
    "Faction mission outpost",
    "Nightfall mission outpost",
};

uint32_t QuestAcceptDialog(GW::Constants::QuestID quest);

uint32_t QuestRewardDialog(GW::Constants::QuestID quest);

GW::Constants::QuestID IndexToQuestID(const int index);

uint32_t IndexToDialogID(const int index);

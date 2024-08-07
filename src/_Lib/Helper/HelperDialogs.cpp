#include "HelperDialogs.h"

namespace
{
enum class QuestDialogType
{
    ENQUIRE = 0x800003,
    TAKE = 0x800001,
    ENQUIRE_NEXT = 0x800004,
    RECAP = 0x800005,
    ENQUIRE_REWARD = 0x800006,
    REWARD = 0x800007
};
}

bool IsQuest(const uint32_t dialog_id)
{
    return (dialog_id & 0x800000) != 0;
}

uint32_t GetQuestID(const uint32_t dialog_id)
{
    return (dialog_id ^ 0x800000) >> 8;
}

uint32_t QuestAcceptDialog(GW::Constants::QuestID quest)
{
    return static_cast<int>(quest) << 8 | static_cast<int>(QuestDialogType::TAKE);
}

uint32_t QuestRewardDialog(GW::Constants::QuestID quest)
{
    return static_cast<int>(quest) << 8 | static_cast<int>(QuestDialogType::REWARD);
}

GW::Constants::QuestID IndexToQuestID(const int index)
{
    switch (index)
    {
    case 0:
        return GW::Constants::QuestID::UW_Chamber;
    case 1:
        return GW::Constants::QuestID::UW_Wastes;
    case 2:
        return GW::Constants::QuestID::UW_UWG;
    case 3:
        return GW::Constants::QuestID::UW_Mnt;
    case 4:
        return GW::Constants::QuestID::UW_Pits;
    case 5:
        return GW::Constants::QuestID::UW_Planes;
    case 6:
        return GW::Constants::QuestID::UW_Pools;
    case 7:
        return GW::Constants::QuestID::UW_Escort;
    case 8:
        return GW::Constants::QuestID::UW_Restore;
    case 9:
        return GW::Constants::QuestID::UW_Vale;
    case 10:
        return GW::Constants::QuestID::Fow_Defend;
    case 11:
        return GW::Constants::QuestID::Fow_ArmyOfDarknesses;
    case 12:
        return GW::Constants::QuestID::Fow_WailingLord;
    case 13:
        return GW::Constants::QuestID::Fow_Griffons;
    case 14:
        return GW::Constants::QuestID::Fow_Slaves;
    case 15:
        return GW::Constants::QuestID::Fow_Restore;
    case 16:
        return GW::Constants::QuestID::Fow_Hunt;
    case 17:
        return GW::Constants::QuestID::Fow_Forgemaster;
    case 18:
        return GW::Constants::QuestID::Fow_Tos;
    case 19:
        return GW::Constants::QuestID::Fow_Toc;
    case 20:
        return GW::Constants::QuestID::Fow_Khobay;
    case 21:
        return GW::Constants::QuestID::Doa_DeathbringerCompany;
    case 22:
        return GW::Constants::QuestID::Doa_RiftBetweenUs;
    case 23:
        return GW::Constants::QuestID::Doa_ToTheRescue;
    case 24:
        return GW::Constants::QuestID::Doa_City;
    case 25:
        return GW::Constants::QuestID::Doa_BreachingStygianVeil;
    case 26:
        return GW::Constants::QuestID::Doa_BroodWars;
    case 27:
        return GW::Constants::QuestID::Doa_FoundryOfFailedCreations;
    case 28:
        return GW::Constants::QuestID::Doa_FoundryBreakout;
    default:
        return GW::Constants::QuestID::None;
    }
}

uint32_t IndexToDialogID(const int index)
{
    switch (index)
    {
    case 0:
        return GW::Constants::DialogID::FowCraftArmor;
    case 1:
        return GW::Constants::DialogID::ProfChangeWarrior;
    case 2:
        return GW::Constants::DialogID::ProfChangeRanger;
    case 3:
        return GW::Constants::DialogID::ProfChangeMonk;
    case 4:
        return GW::Constants::DialogID::ProfChangeNecro;
    case 5:
        return GW::Constants::DialogID::ProfChangeMesmer;
    case 6:
        return GW::Constants::DialogID::ProfChangeEle;
    case 7:
        return GW::Constants::DialogID::ProfChangeAssassin;
    case 8:
        return GW::Constants::DialogID::ProfChangeRitualist;
    case 9:
        return GW::Constants::DialogID::ProfChangeParagon;
    case 10:
        return GW::Constants::DialogID::ProfChangeDervish;
    case 11:
        return GW::Constants::DialogID::FerryKamadanToDocks;
    case 12:
        return GW::Constants::DialogID::FerryDocksToKaineng;
    case 13:
        return GW::Constants::DialogID::FerryDocksToLA;
    case 14:
        return GW::Constants::DialogID::FerryGateToLA;
    case 15:
        return GW::Constants::DialogID::FactionMissionOutpost;
    case 16:
        return GW::Constants::DialogID::NightfallMissionOutpost;
    default:
        return 0;
    }
}

bool IsUWTele(const uint32_t dialog_id)
{
    switch (dialog_id)
    {
    case GW::Constants::DialogID::UwTeleLab:
    case GW::Constants::DialogID::UwTeleVale:
    case GW::Constants::DialogID::UwTelePits:
    case GW::Constants::DialogID::UwTelePools:
    case GW::Constants::DialogID::UwTelePlanes:
    case GW::Constants::DialogID::UwTeleWastes:
    case GW::Constants::DialogID::UwTeleMnt:
        return true;
    default:
        return false;
    }
}

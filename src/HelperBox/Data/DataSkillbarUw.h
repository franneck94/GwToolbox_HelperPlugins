#pragma once

#include <cstdint>

#include <GWCA/Constants/Skills.h>

#include "DataSkillbar.h"

static constexpr auto NUM_SKILLS_EMO = size_t{10U};
class EmoSkillbarData : public SkillbarDataABC<NUM_SKILLS_EMO>
{
public:
    DataSkill burning = DataSkill{GW::Constants::SkillID::Burning_Speed, static_cast<uint32_t>(-1)};
    DataSkill sb = DataSkill{GW::Constants::SkillID::Spirit_Bond, static_cast<uint32_t>(-1)};
    DataSkill fuse = DataSkill{GW::Constants::SkillID::Infuse_Health, static_cast<uint32_t>(-1)};
    DataSkill ether = DataSkill{GW::Constants::SkillID::Ether_Renewal, static_cast<uint32_t>(-1)};
    DataSkill prot = DataSkill{GW::Constants::SkillID::Protective_Bond, static_cast<uint32_t>(-1)};
    DataSkill life = DataSkill{GW::Constants::SkillID::Life_Bond, static_cast<uint32_t>(-1)};
    DataSkill balth = DataSkill{GW::Constants::SkillID::Balthazars_Spirit, static_cast<uint32_t>(-1)};
    DataSkill gdw = DataSkill{GW::Constants::SkillID::Great_Dwarf_Weapon, static_cast<uint32_t>(-1)};
    DataSkill wisdom = DataSkill{GW::Constants::SkillID::Ebon_Battle_Standard_of_Wisdom, static_cast<uint32_t>(-1)};
    DataSkill pi = DataSkill{GW::Constants::SkillID::Pain_Inverter, static_cast<uint32_t>(-1)};

    EmoSkillbarData() : SkillbarDataABC()
    {
        skills = {&burning, &sb, &fuse, &ether, &prot, &life, &balth, &gdw, &wisdom, &pi};
    }
};

static constexpr auto NUM_SKILLS_MESMER = size_t{10U};
class MesmerSkillbarData : public SkillbarDataABC<NUM_SKILLS_MESMER>
{
public:
    // General Mesmer Skills
    DataSkill stoneflesh = DataSkill{GW::Constants::SkillID::Stoneflesh_Aura, static_cast<uint32_t>(-1)};
    DataSkill obsi = DataSkill{GW::Constants::SkillID::Obsidian_Flesh, static_cast<uint32_t>(-1)};
    DataSkill demise = DataSkill{GW::Constants::SkillID::Wastrels_Demise, static_cast<uint32_t>(-1)};
    DataSkill worry = DataSkill{GW::Constants::SkillID::Wastrels_Worry, static_cast<uint32_t>(-1)};
    DataSkill ebon = DataSkill{GW::Constants::SkillID::Ebon_Escape, static_cast<uint32_t>(-1)};
    DataSkill empathy = DataSkill{GW::Constants::SkillID::Empathy, static_cast<uint32_t>(-1)};

    // Solo LT Skills
    DataSkill mantra_of_resolve = DataSkill{GW::Constants::SkillID::Mantra_of_Resolve, static_cast<uint32_t>(-1)};
    DataSkill visage = DataSkill{GW::Constants::SkillID::Sympathetic_Visage, static_cast<uint32_t>(-1)};

    // T4 Skills
    DataSkill mantra_of_earth = DataSkill{GW::Constants::SkillID::Mantra_of_Earth, static_cast<uint32_t>(-1)};
    DataSkill stonestriker = DataSkill{GW::Constants::SkillID::Stone_Striker, static_cast<uint32_t>(-1)};

    MesmerSkillbarData() : SkillbarDataABC()
    {
        skills = {&stoneflesh,
                  &obsi,
                  &demise,
                  &worry,
                  &ebon,
                  &empathy,
                  &mantra_of_resolve,
                  &visage,
                  &mantra_of_earth,
                  &stonestriker};
    }

public:
};

static constexpr auto NUM_SKILLS_DB = size_t{8U};
class DbSkillbarData : public SkillbarDataABC<NUM_SKILLS_DB>
{
public:
    // General DB Skills
    DataSkill honor = DataSkill{GW::Constants::SkillID::Ebon_Battle_Standard_of_Honor, static_cast<uint32_t>(-1)};
    DataSkill eoe = DataSkill{GW::Constants::SkillID::Edge_of_Extinction, static_cast<uint32_t>(-1)};
    DataSkill qz = DataSkill{GW::Constants::SkillID::Quickening_Zephyr, static_cast<uint32_t>(-1)};
    DataSkill winnow = DataSkill{GW::Constants::SkillID::Winnowing, static_cast<uint32_t>(-1)};
    DataSkill pi = DataSkill{GW::Constants::SkillID::Pain_Inverter, static_cast<uint32_t>(-1)};

    // Only Rit Skills
    DataSkill sos = DataSkill{GW::Constants::SkillID::Signet_of_Spirits, static_cast<uint32_t>(-1)};
    DataSkill sq = DataSkill{GW::Constants::SkillID::Serpents_Quickness, static_cast<uint32_t>(-1)};
    DataSkill vamp = DataSkill{GW::Constants::SkillID::Vampirism, static_cast<uint32_t>(-1)};

    DbSkillbarData() : SkillbarDataABC()
    {
        skills = {&sos, &honor, &eoe, &qz, &winnow, &pi, &sq, &vamp};
    }
};

static constexpr auto NUM_SKILLS_RANGER = size_t{12U};
class RangerSkillbarData : public SkillbarDataABC<NUM_SKILLS_RANGER>
{
public:
    // General Ranger Skills
    DataSkill shroud = DataSkill{GW::Constants::SkillID::Shroud_of_Distress, static_cast<uint32_t>(-1)};
    DataSkill sf = DataSkill{GW::Constants::SkillID::Shadow_Form, static_cast<uint32_t>(-1)};
    DataSkill dc = DataSkill{GW::Constants::SkillID::Deaths_Charge, static_cast<uint32_t>(-1)};
    DataSkill dwarfen = DataSkill{GW::Constants::SkillID::Dwarven_Stability, static_cast<uint32_t>(-1)};
    DataSkill whirl = DataSkill{GW::Constants::SkillID::Whirling_Defense, static_cast<uint32_t>(-1)};

    // T1 + T2 Skills
    DataSkill winnow = DataSkill{GW::Constants::SkillID::Winnowing, static_cast<uint32_t>(-1)};

    // T1
    DataSkill finish_him = DataSkill{GW::Constants::SkillID::Finish_Him, static_cast<uint32_t>(-1)};

    // T2 Skills
    DataSkill radfield = DataSkill{GW::Constants::SkillID::Radiation_Field, static_cast<uint32_t>(-1)};
    DataSkill viper = DataSkill{GW::Constants::SkillID::Vipers_Defense, static_cast<uint32_t>(-1)};

    // T1 + T3 Skills
    DataSkill hos = DataSkill{GW::Constants::SkillID::Heart_of_Shadow, static_cast<uint32_t>(-1)};

    // T3 Skills
    DataSkill honor = DataSkill{GW::Constants::SkillID::Ebon_Battle_Standard_of_Honor, static_cast<uint32_t>(-1)};
    DataSkill soh = DataSkill{GW::Constants::SkillID::Shadow_of_Haste, static_cast<uint32_t>(-1)};

    RangerSkillbarData() : SkillbarDataABC()
    {
        skills = {&shroud, &sf, &dc, &dwarfen, &whirl, &winnow, &finish_him, &radfield, &viper, &hos, &honor, &soh};
    }
};

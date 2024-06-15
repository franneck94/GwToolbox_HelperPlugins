# General Features

## Hero Window

![alt](../imgs/HeroWindow.png)

## Behviour Button

Toggle **all** heroes between:

- Guard
- Avoid Combat
- Attack

## Follow

All heroes will be flagged to your current position every ~800ms.  
Hence, its much easier to run through enemies with your heroes.  
While running, your heroes with fall back will cast the skill after each other.  
When you ping a far away target, this gets activated.  
When you ping a close target or attack an close target, the following will stop.

## Attack

All heroes target will be set to the players target.
Also, for all mesmer heroes, they will cast ESurge on that target.

## General

Smart features are:

- BiP the player regardless of the weapon when he is on low energy (below 30% or below 15 energy)
- ST: Use Shelter and Union when in fight if not already present
- Use SoS when in fight if not all three spirits are already present
- Use splinter weapon and honor of strength on the player if he is melee attacker
  - For honor of strength: Disable the skill in the hero window
- Immediatly shatter important hexes on the player
- Immediatly remove important conditions on the player

### List of Hexes to Remove

```cpp
// to_remove_hexes_melee
// Mesmer
GW::Constants::SkillID::Ineptitude
GW::Constants::SkillID::Empathy
GW::Constants::SkillID::Crippling_Anguish
// Necro
GW::Constants::SkillID::Spiteful_Spirit
// Ele
GW::Constants::SkillID::Blurred_Vision

// to_remove_hexes_caster
// Mesmer
GW::Constants::SkillID::Panic
GW::Constants::SkillID::Backfire
GW::Constants::SkillID::Mistrust
GW::Constants::SkillID::Power_Leech
// Necro
GW::Constants::SkillID::Spiteful_Spirit
GW::Constants::SkillID::Soul_Leech

// to_remove_hexes_all
// Mesmer
GW::Constants::SkillID::Diversion
GW::Constants::SkillID::Visions_of_Regret
// Ele
GW::Constants::SkillID::Deep_Freeze
GW::Constants::SkillID::Mind_Freeze

// to_remove_hexes_paragon
// Necro
GW::Constants::SkillID::Vocal_Minority
```

Feel free to  create an feature request (issue in Github) for more to add!

### List of Conditions to Remove

```cpp
to_remove_conditions_melee
        GW::Constants::SkillID::Blind

to_remove_conditions_caster
        GW::Constants::SkillID::Dazed

to_remove_conditions_all
        GW::Constants::SkillID::Crippled

```

Feel free to  create an feature request (issue in Github) for more to add!

# Hero Smart Skills

- Smart BiP
  - The player regardless of the weapon when low energy (see the doc for the logic)
- Smart ST
  - Use Shelter and Union when in fight if not already present
- Smart SoS:
  - Use SoS whe in fight if not all three spirits are already present
- Smart melee buffs
  - Use splinter weapon, vigorous spirit on the player if melee attacker
- Smart honor
  - IMPORTANT: Disable the skill in the hero window
- Smart condition and hex removal
  - Immediatly shatter important hexes on the player
  - Immediatly remove important conditions on the player
- Smart hero rupting
  - Mesmer heros will rupt important enemy skills

Note: For a full list of hexes, conditions and skills to rupt look into the documentation

## List of Hexes to Remove

```cpp
// to_remove_hexes_melee
// Mesmer
GW::Constants::SkillID::Ineptitude
GW::Constants::SkillID::Empathy
GW::Constants::SkillID::Crippling_Anguish
GW::Constants::SkillID::Clumsiness
GW::Constants::SkillID::Faintheartedness
// Ele
GW::Constants::SkillID::Blurred_Vision
// Monk
GW::Constants::SkillID::Amity

// to_remove_hexes_caster
// Mesmer
GW::Constants::SkillID::Panic
GW::Constants::SkillID::Backfire
GW::Constants::SkillID::Mistrust
GW::Constants::SkillID::Power_Leech
// Necro
GW::Constants::SkillID::Soul_Leech

// to_remove_hexes_all
// Mesmer
GW::Constants::SkillID::Diversion
GW::Constants::SkillID::Visions_of_Regret
// Ele
GW::Constants::SkillID::Deep_Freeze
GW::Constants::SkillID::Mind_Freeze
// Necro
GW::Constants::SkillID::Spiteful_Spirit

// to_remove_hexes_paragon
// Necro
GW::Constants::SkillID::Vocal_Minority
```

Feel free to  create an feature request (issue in Github) for more to add!

## List of Conditions to Remove

```cpp
// to_remove_conditions_melee
GW::Constants::SkillID::Blind
GW::Constants::SkillID::Weakness

// to_remove_conditions_caster
GW::Constants::SkillID::Dazed

// to_remove_conditions_all
GW::Constants::SkillID::Crippled
```

## List of Skills to Rupt

```cpp
// Mesmer
GW::Constants::SkillID::Panic
GW::Constants::SkillID::Energy_Surge
// Necro
GW::Constants::SkillID::Chilblains
// Ele
GW::Constants::SkillID::Meteor
GW::Constants::SkillID::Meteor_Shower
GW::Constants::SkillID::Searing_Flames
// All
GW::Constants::SkillID::Resurrection_Signet
```

Feel free to  create an feature request (issue in Github) for more to add!

## The BiP Logic

The table defines what absolute and percentage enrgy needs to be surpassed to get a bip.

```cpp
{GW::Constants::Profession::Warrior, {25U, 0.70F}},
{GW::Constants::Profession::Ranger, {25U, 0.60F}},
{GW::Constants::Profession::Monk, {30U, 0.50F}},
{GW::Constants::Profession::Necromancer, {30U, 0.50F}},
{GW::Constants::Profession::Mesmer, {30U, 0.50F}},
{GW::Constants::Profession::Elementalist, {40U, 0.40F}},
{GW::Constants::Profession::Assassin, {25U, 0.60F}},
{GW::Constants::Profession::Ritualist, {30U, 0.50F}},
{GW::Constants::Profession::Paragon, {25U, 0.60F}},
{GW::Constants::Profession::Dervish, {25U, 0.50F}},
```

So for example the hero will bip you as a paragon, if your energy is below a total of 25 or below a percentage of 60%.

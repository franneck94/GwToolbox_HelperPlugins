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
  - For melee: Empathy etc.
  - For caster: Backfire etc.
- Immediatly remove important conditions on the player
  - For melee: blindness, cripple
  - For caster: Dazed, cripple

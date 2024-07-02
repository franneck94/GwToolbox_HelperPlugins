# [TAS] Clown Plugins for GWToolbox

Refer to the [documentation](https://franneck94.github.io/GwToolbox_HelperPlugins/).

## Features are

- Smart Chat Commands (better useskill e.g. for dhuum fight)
- UW Helper: Ranger, Mesmer, Emo and DB
- Hero Window, Hero Smart Skills

## Smart Hero Skills (This is really helpful for GWAMM'ing)

- Hero Window:
  - Heros will follow the player with smart flagging and a maximal fall-back uptime, when following activated or player pings distant target
  - Set all heros behaviour with one click
  - Spike players target with one click
- Smart BiP
  - The player regardless of the weapon when low energy (see the doc for the logic)
- Smart ST
  - Use Shelter and Union when in fight if not already present
- Smart SoS:
  - Use SoS whe in fight if not all three spirits are already present
- Smart splinter
  - Use splinter weapon on the player if melee attacker and is attacking multiple enemies
- Smart vigorous spirit
  - Use vigorous spirit on the player if melee attacker and is attacking
- Smart honor
  - IMPORTANT: Disable the skill in the hero window
- Smart condition and hex removal
  - Immediatly shatter important hexes on the player
  - Immediatly remove important conditions on the player
- Smart hero rupting
  - Mesmer heros will rupt important enemy skills

Note: For a full list of hexes, conditions and skills to rupt look into the documentation

## Development

- CMake 3.21+, Git
- MSVC 2022 with C++23
- Optional: Python 3.8+ for the Documentation

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A Win32 -B .
```

Then open it in VSCode or VS2022.

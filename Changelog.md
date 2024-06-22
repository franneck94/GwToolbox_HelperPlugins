# [TAS] Clown Plugins for GWToolbox

Refer to the [documentation](https://franneck94.github.io/GwToolbox_HelperPlugins/).

## Features are

- Smart Chat Commands (better useskill e.g.  for dhuum fight)
- UW Helper: Ranger, Mesmer, Emo and DB

## Smart Hero Commands (This is really helpful for GWAMM'ing)

- Heros will follow the player with smart flagging and a maximal fall-back uptime, when following activated or player pings distant target
- BiP the player regardless of the weapon when low energy (see the doc for the logic)
- ST: Use Shelter and Union when in fight if not already present
- Use SoS when in fight if not all three spirits are already present
- Use splinter weapon, vigorous spirit and honor of strength on the player if melee attacker
  - For honor of strength: Disable the skill in the hero window
- Immediatly shatter important hexes on the player
- Immediatly remove important conditions on the player
- Mesmer heros will rupt important enemy skills
- For a full list of hexes,conditions and skills to rupt look into the documentation

## New feature compared to last release (0.13.0)

- Added energy mapping table for each class for the bip feature
- Fixed crash on plugin onload while target ping
- Added plotting funcs for debugging purposes

## Known Issues

- /tb close - does not work with the plugins...

# General Features

## Hero Window

![alt](../imgs/HeroWindow.png)

## Behviour Button

Toggle **all** heroes between:

- Guard
- Avoid Combat
- Attack

## Hero Following

While the hero following is active, your heroes with cast fall back after each other.  
The hero behaviour will be set to guard, afterwards it will be set back to the previous behaviour.  
There are two different kinds of activation the hero following:

### Manual Follow

All heroes will be flagged to your current position every ~800ms.  
Hence, its much easier to run through enemies with your heroes.  

### Automatic Follow

Note: With pinging a target i mean pressing **ctrl+space**.
When you ping a far away target, the hero following gets activated.  
When you ping a close target or attack an close target, the hero following will stop.

## Attack

All heroes target will be set to the players target.
Also, for all mesmer heroes, they will cast ESurge on that target.

For smart hero skills see [here](./HeroSkills.md)

# Chat Commands

## Use Skill

This command is the replacement of GWToolbox's useskill since this will count into the current energy and has no artificial skill casting delay.

The command to start useskill for a certain skill (number) is:

```
/use [1-8]
```

The command to stop useskill is one of the following:

```
/use 0
/use
```

## Dhuum

This is a **smart** useskill for the Dhuum fight.
While the progress bar is not finished, *Rest* is cast.
If the progress bar is finished but Dhuum is not below 25%, the damage skill is cast.
If the player is not attacking Dhuum, auto-attacking will be started.

The command to start Dhuum useskil is:

```
/dhuum start
```

The command to stop useskill is one of the following:

```
/dhuum stop
/dhuum
```

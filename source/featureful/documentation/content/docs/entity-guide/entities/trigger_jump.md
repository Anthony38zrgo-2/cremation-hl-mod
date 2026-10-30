---
bookHidden: true
bookToC: false
entityCategory: solid
---

# trigger_jump

A trigger entity that can be used to create jump pads - to launch the toucher to the designated point. Similar to the one from Ricochet but with several changes:

* The jump evaluation is a bit different from Ricochet, especially in terms of how the height parameter is handled: in Ricochet the height is relative to the midpoint between the toucher origin and the target destination; in Featureful the height is relative to the toucher origin.
* The jump evaluation takes into consideration the player's gravity (e.g. if set from `trigger_gravity`).
* By default it can be touched by players only, but has filters for other entities, like `trigger_multiple`.

### Parameters

* `Jump Target [LP]` - the target point to launch the toucher to. Being the Locus Position parameter it allows to create dynamic target points. In simple cases this is usually the name of `info_target` entity.
* `Height` - custom jump height (the distance between the initial position and the apex point). If not set or set to 0, the height will be calculated automatically depending on the toucher position and the jump target.
* `Trigger on jump` - fire when something is launched. The activator is toucher.

### Soundscripts

* **TriggerJump.Self** - the sound to play on `trigger_jump` itself, when something is launched.
* **TriggerJump.Other** - the sound to play on the toucher entity, when it's launched.

Example:

```json
{
    "TriggerJump.Self": {
        "waves": ["triggerjump.wav"]
    },
    "TriggerJump.Other": {
        "waves": ["player/plyrjmp8.wav"]
    }
}

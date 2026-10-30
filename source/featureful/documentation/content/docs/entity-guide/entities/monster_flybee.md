---
bookHidden: true
bookToC: false
---

# monster_flybee

A fast moving flying creature with range attacks. Ported from Half-Life Invasion.

### Skill variables

* **sk_flybee_health** - monster's health.
* **sk_flybee_dmg_kick** - melee damage.
* **sk_flybee_maxspeed** - the maximum flight speed a flybee can reach.
* [flyball skill variables]({{< ref "flyball/#skill-variables" >}}).
* [flybee_zapbomb skill variables]({{< ref "flybee_zapbomb/#skill-variables" >}}).

### Default classification

`Alien Monster`

### Default display name

`Flybee`

### Soundscripts

* **Flybee.Idle** - idle sounds.
* **Flybee.Alert** - alert sounds.
* **Flybee.Pain** - pain sounds.
* **Flybee.Die** - death sounds.
* **Flybee.Attack** - starting chasing the enemy.
* **Flybee.Bite** - melee attack.
* **Flybee.Beam** - played at the end of the beam.
* [flyball soundscripts]({{< ref "flyball/#soundscripts" >}}).
* [flybee_zapbomb soundscripts]({{< ref "flybee_zapbomb/#soundscripts" >}}).

{{% hint info %}}
Flybees don't have their own sounds and use ichthyosaur sounds by default.
{{% /hint %}}

### Visuals

* **Flybee.ZapBeam** - zap attack beam.
* **Flybee.ZapBeamAlt** - zap attack alternative beam. Same as **Flybee.ZapBeam** but different default color.
* [flyball visuals]({{< ref "flyball/#visuals" >}}).
* [flybee_zapbomb visuals]({{< ref "flybee_zapbomb/#visuals" >}}).

### Attacks

* *Melee Attack 1* - bite.
* *Range Attack 1* - projectiles.
* *Range Attack 2* - beam and zap bomb.

### Animation events

* `1` - bite trace hull attack. Plays **Flybee.Bite** soundscript.
* `2` - fire 4 electric [balls]({{< ref flyball >}}).
* `3` - beam attack that creates a [zapbomb]({{< ref flybee_zapbomb >}}) at the end of the beam.

### Entity template examples

{{% tabs %}}

{{% tab "Melee attack settings" %}}
The [check melee]({{< ref "entity-templates/#check_melee_attack1" >}}) rules and [trace hull attacks]({{< ref "entity-templates/#trace_hull_attacks" >}}) properties that emulate monster's native ones. Could be used as a starting point for further changes.

```json
{
    "monster_flybee": {
        "trace_hull_attacks": {
            "1": {
                "distance": 70,
                "punchangle": {
                    "roll": 25
                },
                "damage_info": {
                    "type": ["club"],
                }
            }
        }
    }
}
```
{{% /tab %}}

{{% /tabs %}}

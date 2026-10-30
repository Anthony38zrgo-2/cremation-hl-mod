---
bookHidden: true
bookToC: false
entityCategory: projectile
---

# squidtoxicspit

A poisonous projectile fired by [monster_bullchicken]({{< ref monster_bullchicken >}})

### Skill variables

* **sk_bullsquid_dmg_toxic_poison** - the poison damage of the toxic spit. By default it's **sk_bullsquid_dmg_spit** multiplied by 0.25.
* **sk_bullsquid_dmg_toxic_impact** - the impact damage of the toxic spit. By default it's **sk_bullsquid_dmg_spit** multiplied by 1.5.

### Projectile properties

Speed: 600

#### Direct damage

* Damage: **sk_bullsquid_dmg_toxic_impact**
* Type: acid

#### Aura damage

* Damage: **sk_bullsquid_dmg_toxic_poison**
* Type: poison
* Radius: 32
* Traits: timed damage is non lethal; ignores armor

### Soundscripts

* **Bullsquid.ToxicSpitTouch** - one of the sounds played when the toxic spit projectile hits something.
* **Bullsquid.ToxicSpitHit** - one of the sounds played when the toxic spit projectile hits something.

### Visuals

* **Bullsquid.ToxicSpit** - the toxic spit projectile sprite.
* **Bullsquid.ToxicFleck** - sprites sprayed on the toxic spit projectile impact with the wall.
* **Bullsquid.ToxicParticle** - sprites emitted from the toxic spit projectile as it flies. Note: currently can't be animated.

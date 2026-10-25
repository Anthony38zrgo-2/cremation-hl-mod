# Gib routing and Link Gun corpses

UT organs are compiled at twice their previous linear dimensions. Human pieces retain their prior scale. Native hgibs has six body variants; agibs has four.

Humanoid death hooks wrap the existing Win32 TakeDamage (slot 11) and Killed (slot 13), preserving the Blood cultist voice hook. During zombie/HECU native death, the engine SetModel interception routes agibs to hgibs. AI classification remains unchanged. Hooks are reinstalled on spawn/restore.

Link Gun secondary leaves the existing skeleton effect only for zombie variants and monster_human_grunt. Other targets leave a temporary copy of their own model, body and skin with flat green translucent rendering and green glow shell for five seconds. Native death sequence and position are followed, with continuous animation interpolation; the normal textured corpse remains hidden. The temporary corpse disappears after five seconds.

Compiled and installed. Interactive combat and visual validation remain pending.

## Fixed humanoid gibbing set

After native zombie/HECU whole-body gibbing, collected native random fragments are retired and replaced once by six rigid pieces: head, torso, left arm, right arm, left leg, right leg. The mirrored limbs use original UT textures. Each leg combines the supplied UT thigh and foot geometry into one rigid fragment. No localized dismemberment is introduced. Normal deaths and Link Gun death effects do not trigger this replacement. Pieces bounce with varied impulses/rotation, play selected fragment impact sounds, cannot take damage, and expire after 25 seconds. Transient pieces are discarded on save restore.

## Gib physics correction

Rigid fragments now use one explicit world-collision simulation with 10 ms substeps instead of native MOVETYPE_BOUNCE. They are non-solid decorative entities, preventing mutual/player collision impulses. Six radial spawn offsets and separated launch directions avoid overlapping piles; inherited velocity is capped. Impacts damp tangent velocity to 55 percent and normal velocity to 25 percent. Floor contacts settle after three contacts, low speed, or two seconds of flight; this threshold never freezes airborne pieces. Resting geometry is laid down and its transformed mesh minimum Z is placed on the floor. Resting pieces stop translating and rotating. Impact audio retains the selected UT samples. Compiled/installed; visual in-game validation remains pending.

## Blood restoration

Anatomical gib collision emits red blood decals on world/brush surfaces, up to five per piece with a 120 ms cooldown, respecting violence_hblood. BloodTrailsForGibs archive flags are applied to the UT human (36) and organ (16) models; original archive reference models are retained in assets-source/bloodtrailsforgibs. Converter preserves flags on rebuild. Existing UT geometry, fixed count, double organ scale and settling physics are retained. DLL compiled and installed; in-game appearance is pending.

## Weapon impact particles

Custom weapons often invoke TakeDamage directly and skip the native TraceAttack visual blood path. weapon_blood.inc chains the existing monster TakeDamage wrappers, including humanoid gib routing and Blood cultist voices. On actual health loss from a player-attributed attack, it emits GibBlood to the affected PVS. Client invokes the same R_RocketTrail primitive used by the installed gib model flags: type2 for red human blood, type3 for the organ model green trail. This restores visible particle emission for custom pellet, energy, projectile and blast damage without adding damage or changing weapon behavior. DONT_BLEED targets are excluded, violence_hblood/ablood are respected, and emissions are throttled to40 ms per victim. Multiple short randomized segments produce the same flying/falling particles as the gibs. Models are unchanged. Client/server rebuilt and installed; visual acceptance pending in game.

Blood arc update: server doubles the previous emission group budget (4-10 groups), and records actual TraceLine hit points/outward directions for weapon impacts; non-trace damage falls back to the attacker-facing body surface. Client allocates16 gib-palette particles per group (64-160) with an explicit analytical ballistic path: outward impulse40-85, upward70-115, gravity650 units/s squared, .8-1.25 s lifetime. Random sideways spread is limited to8 units/s. This replaces tracer-driven sideways motion while retaining particle rendering. Positions are assigned absolutely in the callback to avoid engine-dependent double integration. Original Half-Life bloodspray/blood sprites are also emitted via R_BloodSprite. Particle state resets on HUD reset/video init and removes itself on particle death. Client/server compiled and installed; in-game visual validation pending.

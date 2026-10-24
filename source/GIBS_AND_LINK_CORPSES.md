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

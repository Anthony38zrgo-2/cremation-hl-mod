# Necromancer

Slot 1 (internal slot 0, position 1), hornetgun carrier ID 11; HUD charge index 23.
New Rapidcore games provide the staff beside the current startup weapons.

Primary, once per press: launches a red gravity-affected orb, 450 units/s plus 120 vertical. Only ground surfaces with normal Z >= .65 summon a minion; walls reflect it. Lifetime 6 seconds. A human hull checks clearance before native zombie construction.

One pending orb or living minion consumes the single charge. Natural death restores availability. Secondary, once per press, remotely detonates the owner's minion: 45 blast damage in 180 units, then 12 burn damage/second in 160 units for 5 seconds. Line-of-sight and height checks constrain damage. Player, minions and friendly/passive human classes are spared.

Minion uses native zombie AI and the supplied skeleton model. A conditional Classify hook returns PLAYER_ALLY only for monster_minion; ordinary zombies retain their original class. Native ownership survives saves. Effects are transient and removed on restore; live minions are rediscovered by owner.

Burn kills use copies of installed monster models with texture palettes multiplied by .73. Skeletons and animation data remain intact. Models with external texture companions are excluded from the palette substitution.

Assets: Temp/v_crowbar (3).mdl, p_crowbar.mdl, w_crowbar.mdl; Downloads/skeleton_zombie_code_24433ds.zip (zombie.mdl). Flame sprite from installed valve/sprites/fire.spr. Red orb derived from Cremation's Link Gun plasma body; HUD generated locally. No archive instructions executed.

Build: compile ricochet_proxy.cpp and bounce_shotgun.cpp with MSVC Win32, finalize_proxy_exports.py, copy DLL. Do not run the full asset preparation pipeline. Compiled and installed; interactive gameplay validation remains pending.

Hand grenades: native timed grenade damage is halved once per entity. On detonation they call the same necroIgnite function as the minion: 160-unit radius, 5 seconds, 12 burn damage/second, identical flame sprites and corpse darkening. Underwater detonations do not create fire. Save writes the original blast damage so loading cannot halve it twice.

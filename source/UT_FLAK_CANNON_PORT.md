# UT GOTY Flak Cannon port

Source C:/UnrealTournament/System/Botpack.u; original UT_FlakCannon.uc identifies flakm, FlakHand and Flak2Pick and their texture assignments. Extracted models/textures are in assets-source/ut99_flakcannon/export. Dedicated converter: tools/convert_ut99_flakcannon.py. Do not use the full asset preparation pipeline.

Installed models retain current resource names: v_bounce_shotgun.mdl, p_bounce_shotgun.mdl, w_bounce_shotgun.mdl. View rig approximates all 100 exported poses with 120 bones, mean vertex pose deviation .248 units, max 2.670; third-person has 100 bones, world one rigid bone. Original indexed textures are retained; the procedural ammo screen is a static orange 30, while the HUD continues to show actual ammunition.

View reference pose 45, scale .085, Y mirrored with winding/normal correction, translation (-12,-24,-13). Visible projected geometry was inspected offline to choose a muzzle point on the barrel front. Actual in-game camera alignment is unverified.

Animation slots match existing Flak gameplay: idle 0, secondary 1 (source 57-66), primary 2 (46-55), mechanism 3 (30-44), draw 6 (0-29), holster 7 (82-91), sway 8, all source poses 9. Primary/secondary schedule the mechanism motion after their shot animation without consuming extra ammunition or adding reload input. New firing cancels/replaces the queued mechanism timing.

The client render-bone/projection muzzle capture now supports this viewmodel as well as the Pulse Gun. FlakMuz messages trigger a local muzzle sprite at that rendered attachment; world projectile collision, orange trails/particles, damage, shared ammo and CannonExplode impact recording retain their current implementations.

All three models and both DLLs compiled and installed. Full game restart required; interactive visibility, animation, grip and muzzle alignment remain pending.

HUD availability fix: Flak occupies bucket3 position3 internally; disabled Winchester previously occupied that same slot, overwriting the HUD registration even without inventory ownership. Winchester now registers position4. Flak retains its own WeaponID6/name weapon_bounce_shotgun (Plasma Rifle is weapon_nailgun/ID4) and sets SELECTONEMPTY so reserve timing cannot hide it from selection. Rapidcore startup loadout regenerated with Flak and buckshot ammo. Compiled/installed; in-game availability acceptance pending.

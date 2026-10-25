# UT99 Shock Rifle / ASMD

Source: C:/UnrealTournament/System/Botpack.u and UnrealShare.u. Original class references: assets-source/ut99_sounds/classes/ShockRifle.uc and ShockProj.uc. Dedicated model conversion: tools/convert_ut99_shockrifle.py; raw meshes/textures and pose-error report: assets-source/ut99_shockrifle/. View model uses 120 bones for 52 poses (mean error .063 units, maximum .858); third-person 100, pickup 1. Original textures retained. View axes Y-negative, offset (-10,-28,-10); barrel attachment follows the converted animated mesh. Offline projection preview inspected; in-game view remains unverified.

Inventory: weapon_shockrifle aliases native weapon_gauss, ID9, group4 position1. Native uranium pool provides 20 rounds on pickup and persists through native saves; normalized cap50, no magazine/reload. Optional ammo_shockcore export aliases native ammo_gaussclip. Full ammo blocks pickup. Startup Rapidcore adds the Shock Rifle to the seven previously enabled weapons; shotgun and Winchester remain disabled.

Primary: 40 energy damage, hitscan8192, .71 seconds, one charge, precise ray with push. Secondary: constant500 units/s violet core, 55 radial damage/70 radius, .85 seconds, one charge. Lifetime10 seconds, side traces sweep6-unit radius; impacts explode without bouncing. Combo: the primary ray searches all active spheres within12 units of its path and before its nearest native hit. Hitting a sphere produces up to165 radial damage in250 units with falloff and line-of-sight checks, then consumes it. As in ShockRifle.uc, hitting a core consumes up to two extra available charges in addition to the primary shot (up to four total including the sphere). Maximum64 live cores. Damage/push/locking is server authoritative.

Primary beam, core, trail, glow, blast rings and light are violet. UT original fire/alternate/equip/impact/combo sounds converted to mono PCM16/22050Hz. Client ShockFx messages render the beam/launch glow from the actual render-bone/projection muzzle capture used by the other UT ports. Native trajectory/impact stays on server. HUD has a dedicated SHOCK icon and uses the actual ammo pool. Temporary cores are discarded on restore, without affecting saved ammo.

Sequences: idle0, primary1 (30-39), secondary2 (40-49), draw3 (0-14), holster4 (17-23), idle2=5 (28-29), all source52 poses=6.

Narrow model/client/server compilation completed and installed. No gameplay tests were run. Restart the game completely and start a new Rapidcore game for the revised inventory. Interactive combo, visual origin, third-person grip and damage acceptance remain pending.

View shifted forward8 units and secondary core speed reduced from1000 to500 units/s at user request. Firing cadence is unchanged. Model muzzle remains render-attached. Recompiled/installed; interactive appearance pending.

World/third-person scale correction: a one-line Python conditional accidentally
limited scale and offset application to ASMD2M. Scale/offset now apply to all
three meshes; ASMD2pick uses 0.04 (51.76 units long), ASMD2hand uses 0.035.
Both installed models were regenerated. Initial weapons are delivered directly
to native player inventory, with no map weapon pickups or weapon drops.

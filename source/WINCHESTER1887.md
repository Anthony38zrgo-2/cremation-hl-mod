# Winchester 1887

Source: C:/Users/bill/Downloads/winchester_1887.rar, Default + Textures model variant. Added as weapon_winchester1887, using the unused native .357 inventory carrier (ID 3). Group 3, position 3 internally (slot 2). Startup Rapidcore loadout includes it alongside the existing shotgun.

No magazine or reload: primary consumes one buckshot directly from sharedShells, the same reserve as shotgun/Flak. Native carrier firing/reloading is masked. HUD clip is hidden. Eight hitscan pellets, 8 damage each, .07 spread, 1.04 s shot interval; original shoot1 (sequence 1), draw (6), idle (0). Secondary has no attack. Uses supplied view/player/world models and isolated sound paths. Model audio events are redirected to weapons/m1887/ so other shotguns retain their own recordings.

Narrow Win32 DLL build succeeded and was installed. Interactive gameplay verification remains pending. Existing saves can obtain the weapon using give weapon_winchester1887; new Rapidcore starts include it.

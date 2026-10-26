# UT GOTY Pulse Gun port

Damage balance update: both Link Gun modes deal 75% more damage. Primary
plasma increases from 9 to 15.75 per impact. Secondary beam scales from
21 to 84 DPS (previously 12 to 48), with ramp gain 10.5 DPS per connected
second. Cadence and ammo costs are unchanged. The native client validation
fires a projectile into a zombie and measures 15.75 damage, then measures
2.1 and 8.4 damage over 0.1-second beam ticks at minimum and maximum ramp.

Source: C:/UnrealTournament/System/Botpack.u, exported by UModel into assets-source/ut99_pulsegun/export. Converter tools/convert_ut99_pulsegun.py compiles installed models/cremation/v_nailgun.mdl, p_nailgun.mdl and w_nailgun.mdl with original UT hand and gun textures. The existing weapon_nailgun inventory identity and Link Gun combat behavior remain.

UE1 vertex-animation poses are approximated by 120 translation bones for the viewmodel, within GoldSrc's 128-bone limit. Conversion covers all 126 frames; mean pose error is 0.071 GoldSrc units, maximum 3.372 units in outlying poses. The 100-bone third-person conversion reproduces its source poses. Pickup uses one rigid bone. Details in assets-source/ut99_pulsegun/conversion-report.json.

Sequences: idle=0, idle2=1, beamloop=2, holster=3, draw=4, shoot=5, shootloop=6, beamstart=7, beamend=8, spindown=9. Gameplay uses draw, shoot, beamloop and beamend. Original gun/hand texture images are preserved via indexed BMP conversion. UT's procedural ammo screen is replaced with a static green 60 readout; live HUD ammo remains accurate.

View orientation/scale are converted to GoldSrc and muzzle effects use forward 40, right 16, down 5.4 offsets. Pickup now explicitly receives the converted world model. Native combat, sounds, damage, ammo and kill effects are retained.

Models and narrow Win32 DLL compiled and installed. No interactive visual/gameplay validation performed; view positioning, attachment alignment and third-person grip need in-game review. Full legacy asset installers can overwrite the converted models; rerun the dedicated converter to regenerate this port.

Visibility fix: preserve source negative Y as the view-forward direction, mirror X, and use view offset (-4,-12,-10). The previous Y mirror with positive Y offset placed the mesh behind the view camera. Models and DLL regenerated and installed; in-game visibility remains to be confirmed.

View adjustment: shifted forward another 8 units, final offset (-4,-20,-10). Primary plasma and secondary beam origins shifted forward 8 units accordingly (forward 48/right 16/down 5.4). Compiled and installed; visual alignment pending in-game.

## Client-rendered muzzle attachment

Pulse Gun converter places attachment 0 at the centroid of the muzzle tip and attaches it to the corresponding animated vertex-cluster bone. The server emits LinkMuzzle messages (shooter, impact flag, authoritative endpoint) to clients instead of server-origin beam segments. A Win32 client proxy preserves the Ravenholm client in cl_dlls/client_ravenholm.dll and forwards its exports. Initialize, HUD callbacks and the F function table are intercepted. After the native studio renderer draws the viewmodel, beam source points are refreshed from viewmodel attachment[0], including all four beam layers. Beam objects remain allocated and update every rendered frame, avoiding accumulation of temporary server beam origins. Endpoints expire after 180 ms without updates; HUD map/video reset clears beam references. Native server damage/locking/ammo remain authoritative.

Client/server DLLs and models compiled and installed. Full game restart is required. Interactive muzzle alignment and client compatibility remain pending.

Primary launch visual correction: attachment bind position was inspected in the compiled model and lies at (-16.361,-47.900,-5.389), consistent with the front of the rendered reference mesh. Primary previously still rendered only the server projectile. LinkShot now sends its three sprite entity IDs and velocity to the owner client. After drawing the viewmodel, the client emits all three plasma layers directly from attachment[0] for 65 ms and suppresses those authoritative sprite entities during that interval. Subsequent flight uses server entities; collision and damage remain unchanged. Client and server rebuilt and installed; interactive visual acceptance remains pending.

Render-space muzzle correction: client now wraps engine StudioDrawPoints through the studio API passed to the donor. The muzzle is transformed with the actual render bone matrix immediately before drawing the model, instead of relying on cl_entity attachment values computed only in STUDIO_EVENTS passes. OpenGL model/projection matrices project this point into normalized screen coordinates, then the normal world projection reconstructs a beam-space position at the same depth. The result is used by all beam layers and primary launch sprites. World projection is captured in HUD_DrawNormalTriangles; non-OpenGL paths fall back to render-bone coordinates. Muzzle sampling is logged at most every two seconds in link-muzzle-diagnostics.txt to confirm actual render matrices in-game. Compiled and installed; interactive acceptance pending. Link opengl32.lib when building the client proxy.

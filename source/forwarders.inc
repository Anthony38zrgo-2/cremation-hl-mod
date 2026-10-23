// Generated Win32 tail jumps preserve all original calling conventions.
static FARPROC forwardTargets[521];
static const char* forwardNames[]={
    "?AccelerateThink@CApacheHVR@@AAEXXZ",
    "?ActiveThink@CBaseTurret@@QAEXXZ",
    "?AngularMoveDone@CBaseToggle@@QAEXXZ",
    "?Animate@CBMortar@@QAEXXZ",
    "?Animate@CSquidSpit@@QAEXXZ",
    "?AnimateThink@CControllerZapBall@@AAEXXZ",
    "?AnimateThink@CSprite@@QAEXXZ",
    "?AnimateUntilDead@CSprite@@QAEXXZ",
    "?AttemptToMaterialize@CBasePlayerItem@@QAEXXZ",
    "?AutoSearchThink@CBaseTurret@@QAEXXZ",
    "?BallDetonate@CGrenade@@QAEXXZ",
    "?BallThink@CGrenade@@QAEXXZ",
    "?BallTouch@CGrenade@@QAEXPAVCBaseEntity@@@Z",
    "?BarnacleThink@CBarnacle@@QAEXXZ",
    "?BeamBreakThink@CTripmineGrenade@@AAEXXZ",
    "?BiteTouch@CIchthyosaur@@QAEXPAVCBaseEntity@@@Z",
    "?BloodGush@CCineBlood@@QAEXXZ",
    "?BloodStart@CCineBlood@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?BoltTouch@CCrossbowBolt@@AAEXPAVCBaseEntity@@@Z",
    "?BounceGibTouch@CGib@@QAEXPAVCBaseEntity@@@Z",
    "?BounceTouch@CControllerHeadBall@@AAEXPAVCBaseEntity@@@Z",
    "?BounceTouch@CGrenade@@QAEXPAVCBaseEntity@@@Z",
    "?BounceTouch@CNihilanthHVR@@QAEXPAVCBaseEntity@@@Z",
    "?BreakTouch@CBreakable@@QAEXPAVCBaseEntity@@@Z",
    "?BubbleThink@CCrossbowBolt@@AAEXXZ",
    "?ButtonBackHome@CBaseButton@@QAEXXZ",
    "?ButtonReturn@CBaseButton@@QAEXXZ",
    "?ButtonSpark@CBaseButton@@QAEXXZ",
    "?ButtonTouch@CBaseButton@@QAEXPAVCBaseEntity@@@Z",
    "?ButtonUse@CBaseButton@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?CallBuildNodeGraph@CTestHull@@QAEXXZ",
    "?CallGoDown@CFuncPlat@@QAEXXZ",
    "?CallHitBottom@CFuncPlat@@QAEXXZ",
    "?CallHitTop@CFuncPlat@@QAEXXZ",
    "?CallMonsterThink@CBaseMonster@@QAEXXZ",
    "?CanThink@CItemSoda@@QAEXXZ",
    "?CanTouch@CItemSoda@@QAEXPAVCBaseEntity@@@Z",
    "?ChangeFriction@CFrictionModifier@@QAEXPAVCBaseEntity@@@Z",
    "?CineThink@CCineMonster@@QAEXXZ",
    "?CineThink@CLegacyCineMonster@@QAEXXZ",
    "?CombatUse@CIchthyosaur@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?CommandUse@CNihilanth@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?CommandUse@COsprey@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?CommandUse@CTentacle@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?CorpseFallThink@CBaseMonster@@QAEXXZ",
    "?CounterUse@CBaseTrigger@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?CrashTouch@CApache@@AAEXPAVCBaseEntity@@@Z",
    "?CrashTouch@CNihilanth@@QAEXPAVCBaseEntity@@@Z",
    "?CrashTouch@COsprey@@QAEXPAVCBaseEntity@@@Z",
    "?Cycle@CTentacle@@QAEXXZ",
    "?CyclicUse@CMonsterMaker@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?DamageThink@CLightning@@QAEXXZ",
    "?DangerSoundThink@CGrenade@@QAEXXZ",
    "?DartTouch@CHornet@@QAEXPAVCBaseEntity@@@Z",
    "?DeadEnd@CFuncTrackTrain@@QAEXXZ",
    "?DeadEnd@CFuncVehicle@@QAEXXZ",
    "?DeadThink@CLeech@@QAEXXZ",
    "?DefaultTouch@CBasePlayerAmmo@@QAEXPAVCBaseEntity@@@Z",
    "?DefaultTouch@CBasePlayerItem@@QAEXPAVCBaseEntity@@@Z",
    "?DelayDeathThink@CTripmineGrenade@@AAEXXZ",
    "?DelayThink@CBaseDelay@@QAEXXZ",
    "?DelayThink@CScriptedSentence@@QAEXXZ",
    "?Deploy@CBaseTurret@@QAEXXZ",
    "?DeployThink@COsprey@@QAEXXZ",
    "?DestroyItem@CBasePlayerItem@@QAEXXZ",
    "?Detonate@CGrenade@@QAEXXZ",
    "?DetonateUse@CGrenade@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?Die@CBreakable@@QAEXXZ",
    "?DieThink@CControllerHeadBall@@AAEXXZ",
    "?DieThink@CTentacle@@QAEXXZ",
    "?DieTouch@CHornet@@QAEXPAVCBaseEntity@@@Z",
    "?DissipateThink@CNihilanthHVR@@QAEXXZ",
    "?DoorGoDown@CBaseDoor@@QAEXXZ",
    "?DoorGoUp@CBaseDoor@@QAEXXZ",
    "?DoorHitBottom@CBaseDoor@@QAEXXZ",
    "?DoorHitTop@CBaseDoor@@QAEXXZ",
    "?DoorMoveDone@CMomentaryDoor@@QAEXXZ",
    "?DoorTouch@CBaseDoor@@QAEXPAVCBaseEntity@@@Z",
    "?DrawThink@CNodeViewer@@QAEXXZ",
    "?DropDelay@CTestHull@@QAEXXZ",
    "?DyingThink@CApache@@AAEXXZ",
    "?DyingThink@CNihilanth@@QAEXXZ",
    "?DyingThink@COsprey@@QAEXXZ",
    "?EndSectionTouch@CTriggerEndSection@@QAEXPAVCBaseEntity@@@Z",
    "?EndSectionUse@CTriggerEndSection@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?ExecuteChangeLevel@CChangeLevel@@QAEXXZ",
    "?ExpandThink@CSprite@@QAEXXZ",
    "?ExplodeThink@CCrossbowBolt@@AAEXXZ",
    "?ExplodeTouch@CControllerZapBall@@AAEXPAVCBaseEntity@@@Z",
    "?ExplodeTouch@CGrenade@@QAEXPAVCBaseEntity@@@Z",
    "?FallHack@CFlockingFlyer@@QAEXXZ",
    "?FallThink@CBasePlayerItem@@QAEXXZ",
    "?FieldUse@CFuncMortarField@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?Find@CFuncTrackChange@@QAEXXZ",
    "?Find@CFuncTrackTrain@@QAEXXZ",
    "?Find@CFuncTrainControls@@QAEXXZ",
    "?Find@CFuncVehicle@@QAEXXZ",
    "?Find@CFuncVehicleControls@@QAEXXZ",
    "?FindAllThink@COsprey@@QAEXXZ",
    "?FindThink@CScriptedSentence@@QAEXXZ",
    "?FizzThink@CBubbling@@QAEXXZ",
    "?FlockFollowerThink@CFlockingFlyer@@QAEXXZ",
    "?FlockLeaderThink@CFlockingFlyer@@QAEXXZ",
    "?FlyThink@COsprey@@QAEXXZ",
    "?FlyTouch@CApache@@AAEXPAVCBaseEntity@@@Z",
    "?FollowTarget@CTriggerCamera@@QAEXXZ",
    "?FollowThink@CRpgRocket@@QAEXXZ",
    "?FollowerUse@CTalkMonster@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?FormFlock@CFlockingFlyer@@QAEXXZ",
    "?GoDown@CFuncTrackChange@@UAEXXZ",
    "?GoUp@CFuncTrackChange@@UAEXXZ",
    "?GravityTouch@CTriggerGravity@@QAEXPAVCBaseEntity@@@Z",
    "?HitTouch@COsprey@@QAEXPAVCBaseEntity@@@Z",
    "?HitTouch@CTentacle@@QAEXPAVCBaseEntity@@@Z",
    "?HoverThink@CNihilanthHVR@@QAEXXZ",
    "?HoverThink@COsprey@@QAEXXZ",
    "?HuntThink@CApache@@AAEXXZ",
    "?HuntThink@CControllerHeadBall@@AAEXXZ",
    "?HuntThink@CNihilanth@@QAEXXZ",
    "?HuntThink@CSqueakGrenade@@AAEXXZ",
    "?HurtTouch@CBaseTrigger@@QAEXPAVCBaseEntity@@@Z",
    "?HurtTouch@CFuncRotating@@QAEXPAVCBaseEntity@@@Z",
    "?IdleThink@CFlockingFlyer@@QAEXXZ",
    "?IgniteThink@CApacheHVR@@AAEXXZ",
    "?IgniteThink@CRpgRocket@@QAEXXZ",
    "?Initialize@CBaseTurret@@QAEXXZ",
    "?ItemTouch@CItem@@QAEXPAVCBaseEntity@@@Z",
    "?Kill@CWeaponBox@@QAEXXZ",
    "?LeapTouch@CHeadCrab@@QAEXPAVCBaseEntity@@@Z",
    "?LeapTouch@CPHeadCrab@@QAEXPAVCBaseEntity@@@Z",
    "?LeapTouch@Cfastheadcrab@@QAEXPAVCBaseEntity@@@Z",
    "?LinearMoveDone@CBaseToggle@@QAEXXZ",
    "?LoadThink@CRevertSaved@@QAEXXZ",
    "?MakerThink@CMonsterMaker@@QAEXXZ",
    "?ManagerThink@CMultiManager@@QAEXXZ",
    "?ManagerUse@CMultiManager@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?Materialize@CBasePlayerAmmo@@QAEXXZ",
    "?Materialize@CBasePlayerItem@@QAEXXZ",
    "?Materialize@CItem@@QAEXXZ",
    "?MessageThink@CRevertSaved@@QAEXXZ",
    "?MonsterInitThink@CBaseMonster@@QAEXXZ",
    "?MonsterThink@CRoach@@UAEXXZ",
    "?MonsterUse@CBaseMonster@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?MortarExplode@CMortar@@QAEXXZ",
    "?MultiTouch@CBaseTrigger@@QAEXPAVCBaseEntity@@@Z",
    "?MultiWaitOver@CBaseTrigger@@QAEXXZ",
    "?NearestPath@CFuncTrackTrain@@QAEXXZ",
    "?NearestPath@CFuncVehicle@@QAEXXZ",
    "?Next@CFuncTrackTrain@@QAEXXZ",
    "?Next@CFuncTrain@@QAEXXZ",
    "?Next@CFuncVehicle@@QAEXXZ",
    "?Next@CGunTarget@@QAEXXZ",
    "?NullThink@CApache@@AAEXXZ",
    "?NullThink@CNihilanth@@QAEXXZ",
    "?Off@CMomentaryRotButton@@QAEXXZ",
    "?Off@CRecharge@@QAEXXZ",
    "?Off@CWallHealth@@QAEXXZ",
    "?PathFind@CTestHull@@QAEXXZ",
    "?PendulumUse@CPendulum@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?PlatUse@CFuncPlat@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?PlayerDeathThink@CBasePlayer@@QAEXXZ",
    "?PowerupThink@CTripmineGrenade@@AAEXXZ",
    "?PreDetonate@CGrenade@@QAEXXZ",
    "?RadiationThink@CTriggerHurt@@QAEXXZ",
    "?RampThink@CAmbientGeneric@@QAEXXZ",
    "?Recharge@CRecharge@@QAEXXZ",
    "?Recharge@CWallHealth@@QAEXXZ",
    "?Register@CMultiSource@@QAEXXZ",
    "?RemoveTouch@CNihilanthHVR@@QAEXPAVCBaseEntity@@@Z",
    "?RepelUse@CHGruntRepel@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?Retire@CBaseTurret@@QAEXXZ",
    "?Return@CMomentaryRotButton@@QAEXXZ",
    "?Revive@CLaserSpot@@QAEXXZ",
    "?RocketTouch@CRpgRocket@@QAEXPAVCBaseEntity@@@Z",
    "?RopeTouch@CPendulum@@QAEXPAVCBaseEntity@@@Z",
    "?Rotate@CFuncRotating@@QAEXXZ",
    "?RotatingUse@CFuncRotating@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?SUB_CallUseToggle@CBaseEntity@@QAEXXZ",
    "?SUB_DoNothing@CBaseEntity@@QAEXXZ",
    "?SUB_FadeOut@CBaseEntity@@QAEXXZ",
    "?SUB_Remove@CBaseEntity@@QAEXXZ",
    "?SUB_StartFadeOut@CBaseEntity@@QAEXXZ",
    "?SatchelSlide@CSatchelCharge@@AAEXPAVCBaseEntity@@@Z",
    "?SatchelThink@CSatchelCharge@@AAEXXZ",
    "?SaveTouch@CTriggerSave@@QAEXPAVCBaseEntity@@@Z",
    "?SearchThink@CBaseTurret@@QAEXXZ",
    "?SentryDeath@CSentry@@QAEXXZ",
    "?SentryTouch@CSentry@@QAEXPAVCBaseEntity@@@Z",
    "?ShootThink@CGibShooter@@QAEXXZ",
    "?ShowBadNode@CTestHull@@QAEXXZ",
    "?SittingThink@CSittingScientist@@QAEXXZ",
    "?SittingThink@CSittingcitizen@@QAEXXZ",
    "?SlideTouch@CGrenade@@QAEXPAVCBaseEntity@@@Z",
    "?Smack@CCrowbar@@QAEXXZ",
    "?Smoke@CEnvExplosion@@QAEXXZ",
    "?Smoke@CGrenade@@QAEXXZ",
    "?SparkStart@CEnvSpark@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?SparkStop@CEnvSpark@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?SparkThink@CEnvSpark@@QAEXXZ",
    "?SpeakerThink@CSpeaker@@QAEXXZ",
    "?SpinDown@CFuncRotating@@QAEXXZ",
    "?SpinDownCall@CBaseTurret@@UAEXXZ",
    "?SpinUp@CFuncRotating@@QAEXXZ",
    "?SpinUpCall@CBaseTurret@@UAEXXZ",
    "?Start@CFlockingFlyer@@QAEXXZ",
    "?Start@CGunTarget@@QAEXXZ",
    "?Start@CTentacle@@QAEXXZ",
    "?StartDart@CHornet@@QAEXXZ",
    "?StartTrack@CHornet@@QAEXXZ",
    "?StartupThink@CNihilanth@@QAEXXZ",
    "?StartupUse@CApache@@AAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?StartupUse@CNihilanth@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?StaticDecal@CDecal@@QAEXXZ",
    "?StickyGibTouch@CGib@@QAEXPAVCBaseEntity@@@Z",
    "?Stop@CPendulum@@QAEXXZ",
    "?StrikeThink@CLaser@@QAEXXZ",
    "?StrikeThink@CLightning@@QAEXXZ",
    "?StrikeUse@CLightning@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?SuperBounceTouch@CSqueakGrenade@@AAEXPAVCBaseEntity@@@Z",
    "?SwimThink@CLeech@@QAEXXZ",
    "?Swing@CPendulum@@QAEXXZ",
    "?SwingAgain@CCrowbar@@QAEXXZ",
    "?TankThink@CAirtank@@AAEXXZ",
    "?TankTouch@CAirtank@@AAEXPAVCBaseEntity@@@Z",
    "?TeleportThink@CNihilanthHVR@@QAEXXZ",
    "?TeleportTouch@CBaseTrigger@@QAEXPAVCBaseEntity@@@Z",
    "?TeleportTouch@CNihilanthHVR@@QAEXPAVCBaseEntity@@@Z",
    "?Test@CTentacle@@QAEXXZ",
    "?TestThink@CTestEffect@@QAEXXZ",
    "?ToggleUse@CAmbientGeneric@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?ToggleUse@CBaseTrigger@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?ToggleUse@CLightning@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?ToggleUse@CMonsterMaker@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?ToggleUse@CSpeaker@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?Touch@CRoach@@UAEXPAVCBaseEntity@@@Z",
    "?TouchChangeLevel@CChangeLevel@@QAEXPAVCBaseEntity@@@Z",
    "?TrackTarget@CHornet@@QAEXXZ",
    "?TrackTouch@CHornet@@QAEXPAVCBaseEntity@@@Z",
    "?TriggerAndWait@CBaseButton@@QAEXXZ",
    "?TriggerDecal@CDecal@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?TriggerTouch@CBeam@@QAEXPAVCBaseEntity@@@Z",
    "?TumbleThink@CGrenade@@QAEXXZ",
    "?TurretDeath@CBaseTurret@@QAEXXZ",
    "?TurretUse@CBaseTurret@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?UseChangeLevel@CChangeLevel@@QAEXPAVCBaseEntity@@0W4USE_TYPE@@M@Z",
    "?Wait@CFuncTrain@@QAEXXZ",
    "?Wait@CGunTarget@@QAEXXZ",
    "?WaitTillDead@CBarnacle@@QAEXXZ",
    "?WaitTillLand@CGib@@QAEXXZ",
    "?WarningThink@CTripmineGrenade@@AAEXXZ",
    "?ZapThink@CNihilanthHVR@@QAEXXZ",
    "?ZapTouch@CNihilanthHVR@@QAEXPAVCBaseEntity@@@Z",
    "?think@CPushable@@QAEXXZ",
    "DelayedUse",
    "aiscripted_sequence",
    "ambient_generic",
    "ammo_357",
    "ammo_9mmAR",
    "ammo_9mmbox",
    "ammo_9mmclip",
    "ammo_ARgrenades",
    "ammo_PRbox",
    "ammo_PRclip",
    "ammo_PRgrenades",
    "ammo_buckshot",
    "ammo_crossbow",
    "ammo_egonclip",
    "ammo_gaussclip",
    "ammo_glockclip",
    "ammo_mp5clip",
    "ammo_mp5grenades",
    "ammo_rpgclip",
    "beam",
    "bmortar",
    "bodyque",
    "button_target",
    "cine_blood",
    "controller_energy_ball",
    "controller_head_ball",
    "crossbow_bolt",
    "cycler",
    "cycler_prdroid",
    "cycler_sprite",
    "cycler_weapon",
    "cycler_wreckage",
    "env_beam",
    "env_beverage",
    "env_blood",
    "env_bubbles",
    "env_debris",
    "env_explosion",
    "env_fade",
    "env_funnel",
    "env_global",
    "env_glow",
    "env_laser",
    "env_lightning",
    "env_message",
    "env_render",
    "env_shake",
    "env_shooter",
    "env_smoker",
    "env_sound",
    "env_spark",
    "env_sprite",
    "fireanddie",
    "func_breakable",
    "func_button",
    "func_conveyor",
    "func_door",
    "func_door_rotating",
    "func_friction",
    "func_guntarget",
    "func_healthcharger",
    "func_illusionary",
    "func_ladder",
    "func_monsterclip",
    "func_mortar_field",
    "func_pendulum",
    "func_plat",
    "func_platrot",
    "func_pushable",
    "func_recharge",
    "func_rot_button",
    "func_rotating",
    "func_tank",
    "func_tankcontrols",
    "func_tanklaser",
    "func_tankmortar",
    "func_tankrocket",
    "func_trackautochange",
    "func_trackchange",
    "func_tracktrain",
    "func_train",
    "func_traincontrols",
    "func_vehicle",
    "func_vehiclecontrols",
    "func_wall",
    "func_wall_toggle",
    "func_water",
    "game_counter",
    "game_counter_set",
    "game_end",
    "game_player_equip",
    "game_player_hurt",
    "game_player_team",
    "game_score",
    "game_team_master",
    "game_team_set",
    "game_text",
    "game_zone_player",
    "garg_stomp",
    "gibshooter",
    "grenade",
    "hornet",
    "hvr_rocket",
    "info_bigmomma",
    "info_intermission",
    "info_landmark",
    "info_node",
    "info_node_air",
    "info_null",
    "info_player_deathmatch",
    "info_player_start",
    "info_target",
    "info_teleport_destination",
    "infodecal",
    "item_airtank",
    "item_antidote",
    "item_battery",
    "item_healthkit",
    "item_longjump",
    "item_security",
    "item_sodacan",
    "item_suit",
    "laser_spot",
    "light",
    "light_environment",
    "light_spot",
    "momentary_door",
    "momentary_rot_button",
    "monster_alien_controller",
    "monster_alien_grunt",
    "monster_alien_slave",
    "monster_alyx",
    "monster_alyx_dead",
    "monster_apache",
    "monster_babycrab",
    "monster_barnacle",
    "monster_barney",
    "monster_barney_dead",
    "monster_bigmomma",
    "monster_bloater",
    "monster_bullchicken",
    "monster_cine2_hvyweapons",
    "monster_cine2_scientist",
    "monster_cine2_slave",
    "monster_cine3_barney",
    "monster_cine3_scientist",
    "monster_cine_barney",
    "monster_cine_panther",
    "monster_cine_scientist",
    "monster_citizen",
    "monster_citizen_dead",
    "monster_cockroach",
    "monster_cscanner",
    "monster_fast_headcrab",
    "monster_fastzombie",
    "monster_flyer",
    "monster_flyer_flock",
    "monster_furniture",
    "monster_gargantua",
    "monster_generic",
    "monster_gman",
    "monster_grunt_repel",
    "monster_headcrab",
    "monster_hevsuit_dead",
    "monster_hgrunt_dead",
    "monster_houndeye",
    "monster_human_assassin",
    "monster_human_grunt",
    "monster_ichthyosaur",
    "monster_leech",
    "monster_metrocop",
    "monster_metrocop_dead",
    "monster_miniturret",
    "monster_monk",
    "monster_monk_dead",
    "monster_mortar",
    "monster_nihilanth",
    "monster_osprey",
    "monster_poisionzombie",
    "monster_poison_headcrab",
    "monster_rat",
    "monster_satchel",
    "monster_scientist",
    "monster_scientist_dead",
    "monster_sentry",
    "monster_sitting_citizen",
    "monster_sitting_scientist",
    "monster_snark",
    "monster_tentacle",
    "monster_tentaclemaw",
    "monster_tripmine",
    "monster_turret",
    "monster_vortigaunt",
    "monster_zombie",
    "monster_zombie_dead",
    "monstermaker",
    "multi_manager",
    "multisource",
    "nihilanth_energy_ball",
    "node_viewer",
    "node_viewer_fly",
    "node_viewer_human",
    "node_viewer_large",
    "path_corner",
    "path_track",
    "player",
    "player_loadsaved",
    "player_weaponstrip",
    "rpg_rocket",
    "scripted_sentence",
    "scripted_sequence",
    "soundent",
    "spark_shower",
    "speaker",
    "squidspit",
    "streak_spiral",
    "target_cdaudio",
    "test_effect",
    "testhull",
    "trigger",
    "trigger_auto",
    "trigger_autosave",
    "trigger_camera",
    "trigger_cdaudio",
    "trigger_changelevel",
    "trigger_changetarget",
    "trigger_counter",
    "trigger_endsection",
    "trigger_gravity",
    "trigger_hurt",
    "trigger_monsterjump",
    "trigger_multiple",
    "trigger_once",
    "trigger_push",
    "trigger_relay",
    "trigger_teleport",
    "trigger_transition",
    "weapon_357",
    "weapon_9mmAR",
    "weapon_9mmhandgun",
    "weapon_PR",
    "weapon_crossbow",
    "weapon_crowbar",
    "weapon_egon",
    "weapon_gauss",
    "weapon_ggun",
    "weapon_glock",
    "weapon_handgrenade",
    "weapon_hornetgun",
    "weapon_mp5",
    "weapon_pr",
    "weapon_python",
    "weapon_rpg",
    "weapon_satchel",
    "weapon_shotgun",
    "weapon_snark",
    "weapon_tripmine",
    "weaponbox",
    "world_items",
    "worldspawn",
    "xen_hair",
    "xen_hull",
    "xen_plantlight",
    "xen_spore_large",
    "xen_spore_medium",
    "xen_spore_small",
    "xen_tree",
    "xen_ttrigger",
};
extern "C" __declspec(naked) void forward_0() { __asm jmp dword ptr [forwardTargets+0] }
extern "C" __declspec(naked) void forward_1() { __asm jmp dword ptr [forwardTargets+4] }
extern "C" __declspec(naked) void forward_2() { __asm jmp dword ptr [forwardTargets+8] }
extern "C" __declspec(naked) void forward_3() { __asm jmp dword ptr [forwardTargets+12] }
extern "C" __declspec(naked) void forward_4() { __asm jmp dword ptr [forwardTargets+16] }
extern "C" __declspec(naked) void forward_5() { __asm jmp dword ptr [forwardTargets+20] }
extern "C" __declspec(naked) void forward_6() { __asm jmp dword ptr [forwardTargets+24] }
extern "C" __declspec(naked) void forward_7() { __asm jmp dword ptr [forwardTargets+28] }
extern "C" __declspec(naked) void forward_8() { __asm jmp dword ptr [forwardTargets+32] }
extern "C" __declspec(naked) void forward_9() { __asm jmp dword ptr [forwardTargets+36] }
extern "C" __declspec(naked) void forward_10() { __asm jmp dword ptr [forwardTargets+40] }
extern "C" __declspec(naked) void forward_11() { __asm jmp dword ptr [forwardTargets+44] }
extern "C" __declspec(naked) void forward_12() { __asm jmp dword ptr [forwardTargets+48] }
extern "C" __declspec(naked) void forward_13() { __asm jmp dword ptr [forwardTargets+52] }
extern "C" __declspec(naked) void forward_14() { __asm jmp dword ptr [forwardTargets+56] }
extern "C" __declspec(naked) void forward_15() { __asm jmp dword ptr [forwardTargets+60] }
extern "C" __declspec(naked) void forward_16() { __asm jmp dword ptr [forwardTargets+64] }
extern "C" __declspec(naked) void forward_17() { __asm jmp dword ptr [forwardTargets+68] }
extern "C" __declspec(naked) void forward_18() { __asm jmp dword ptr [forwardTargets+72] }
extern "C" __declspec(naked) void forward_19() { __asm jmp dword ptr [forwardTargets+76] }
extern "C" __declspec(naked) void forward_20() { __asm jmp dword ptr [forwardTargets+80] }
extern "C" __declspec(naked) void forward_21() { __asm jmp dword ptr [forwardTargets+84] }
extern "C" __declspec(naked) void forward_22() { __asm jmp dword ptr [forwardTargets+88] }
extern "C" __declspec(naked) void forward_23() { __asm jmp dword ptr [forwardTargets+92] }
extern "C" __declspec(naked) void forward_24() { __asm jmp dword ptr [forwardTargets+96] }
extern "C" __declspec(naked) void forward_25() { __asm jmp dword ptr [forwardTargets+100] }
extern "C" __declspec(naked) void forward_26() { __asm jmp dword ptr [forwardTargets+104] }
extern "C" __declspec(naked) void forward_27() { __asm jmp dword ptr [forwardTargets+108] }
extern "C" __declspec(naked) void forward_28() { __asm jmp dword ptr [forwardTargets+112] }
extern "C" __declspec(naked) void forward_29() { __asm jmp dword ptr [forwardTargets+116] }
extern "C" __declspec(naked) void forward_30() { __asm jmp dword ptr [forwardTargets+120] }
extern "C" __declspec(naked) void forward_31() { __asm jmp dword ptr [forwardTargets+124] }
extern "C" __declspec(naked) void forward_32() { __asm jmp dword ptr [forwardTargets+128] }
extern "C" __declspec(naked) void forward_33() { __asm jmp dword ptr [forwardTargets+132] }
extern "C" __declspec(naked) void forward_34() { __asm jmp dword ptr [forwardTargets+136] }
extern "C" __declspec(naked) void forward_35() { __asm jmp dword ptr [forwardTargets+140] }
extern "C" __declspec(naked) void forward_36() { __asm jmp dword ptr [forwardTargets+144] }
extern "C" __declspec(naked) void forward_37() { __asm jmp dword ptr [forwardTargets+148] }
extern "C" __declspec(naked) void forward_38() { __asm jmp dword ptr [forwardTargets+152] }
extern "C" __declspec(naked) void forward_39() { __asm jmp dword ptr [forwardTargets+156] }
extern "C" __declspec(naked) void forward_40() { __asm jmp dword ptr [forwardTargets+160] }
extern "C" __declspec(naked) void forward_41() { __asm jmp dword ptr [forwardTargets+164] }
extern "C" __declspec(naked) void forward_42() { __asm jmp dword ptr [forwardTargets+168] }
extern "C" __declspec(naked) void forward_43() { __asm jmp dword ptr [forwardTargets+172] }
extern "C" __declspec(naked) void forward_44() { __asm jmp dword ptr [forwardTargets+176] }
extern "C" __declspec(naked) void forward_45() { __asm jmp dword ptr [forwardTargets+180] }
extern "C" __declspec(naked) void forward_46() { __asm jmp dword ptr [forwardTargets+184] }
extern "C" __declspec(naked) void forward_47() { __asm jmp dword ptr [forwardTargets+188] }
extern "C" __declspec(naked) void forward_48() { __asm jmp dword ptr [forwardTargets+192] }
extern "C" __declspec(naked) void forward_49() { __asm jmp dword ptr [forwardTargets+196] }
extern "C" __declspec(naked) void forward_50() { __asm jmp dword ptr [forwardTargets+200] }
extern "C" __declspec(naked) void forward_51() { __asm jmp dword ptr [forwardTargets+204] }
extern "C" __declspec(naked) void forward_52() { __asm jmp dword ptr [forwardTargets+208] }
extern "C" __declspec(naked) void forward_53() { __asm jmp dword ptr [forwardTargets+212] }
extern "C" __declspec(naked) void forward_54() { __asm jmp dword ptr [forwardTargets+216] }
extern "C" __declspec(naked) void forward_55() { __asm jmp dword ptr [forwardTargets+220] }
extern "C" __declspec(naked) void forward_56() { __asm jmp dword ptr [forwardTargets+224] }
extern "C" __declspec(naked) void forward_57() { __asm jmp dword ptr [forwardTargets+228] }
extern "C" __declspec(naked) void forward_58() { __asm jmp dword ptr [forwardTargets+232] }
extern "C" __declspec(naked) void forward_59() { __asm jmp dword ptr [forwardTargets+236] }
extern "C" __declspec(naked) void forward_60() { __asm jmp dword ptr [forwardTargets+240] }
extern "C" __declspec(naked) void forward_61() { __asm jmp dword ptr [forwardTargets+244] }
extern "C" __declspec(naked) void forward_62() { __asm jmp dword ptr [forwardTargets+248] }
extern "C" __declspec(naked) void forward_63() { __asm jmp dword ptr [forwardTargets+252] }
extern "C" __declspec(naked) void forward_64() { __asm jmp dword ptr [forwardTargets+256] }
extern "C" __declspec(naked) void forward_65() { __asm jmp dword ptr [forwardTargets+260] }
extern "C" __declspec(naked) void forward_66() { __asm jmp dword ptr [forwardTargets+264] }
extern "C" __declspec(naked) void forward_67() { __asm jmp dword ptr [forwardTargets+268] }
extern "C" __declspec(naked) void forward_68() { __asm jmp dword ptr [forwardTargets+272] }
extern "C" __declspec(naked) void forward_69() { __asm jmp dword ptr [forwardTargets+276] }
extern "C" __declspec(naked) void forward_70() { __asm jmp dword ptr [forwardTargets+280] }
extern "C" __declspec(naked) void forward_71() { __asm jmp dword ptr [forwardTargets+284] }
extern "C" __declspec(naked) void forward_72() { __asm jmp dword ptr [forwardTargets+288] }
extern "C" __declspec(naked) void forward_73() { __asm jmp dword ptr [forwardTargets+292] }
extern "C" __declspec(naked) void forward_74() { __asm jmp dword ptr [forwardTargets+296] }
extern "C" __declspec(naked) void forward_75() { __asm jmp dword ptr [forwardTargets+300] }
extern "C" __declspec(naked) void forward_76() { __asm jmp dword ptr [forwardTargets+304] }
extern "C" __declspec(naked) void forward_77() { __asm jmp dword ptr [forwardTargets+308] }
extern "C" __declspec(naked) void forward_78() { __asm jmp dword ptr [forwardTargets+312] }
extern "C" __declspec(naked) void forward_79() { __asm jmp dword ptr [forwardTargets+316] }
extern "C" __declspec(naked) void forward_80() { __asm jmp dword ptr [forwardTargets+320] }
extern "C" __declspec(naked) void forward_81() { __asm jmp dword ptr [forwardTargets+324] }
extern "C" __declspec(naked) void forward_82() { __asm jmp dword ptr [forwardTargets+328] }
extern "C" __declspec(naked) void forward_83() { __asm jmp dword ptr [forwardTargets+332] }
extern "C" __declspec(naked) void forward_84() { __asm jmp dword ptr [forwardTargets+336] }
extern "C" __declspec(naked) void forward_85() { __asm jmp dword ptr [forwardTargets+340] }
extern "C" __declspec(naked) void forward_86() { __asm jmp dword ptr [forwardTargets+344] }
extern "C" __declspec(naked) void forward_87() { __asm jmp dword ptr [forwardTargets+348] }
extern "C" __declspec(naked) void forward_88() { __asm jmp dword ptr [forwardTargets+352] }
extern "C" __declspec(naked) void forward_89() { __asm jmp dword ptr [forwardTargets+356] }
extern "C" __declspec(naked) void forward_90() { __asm jmp dword ptr [forwardTargets+360] }
extern "C" __declspec(naked) void forward_91() { __asm jmp dword ptr [forwardTargets+364] }
extern "C" __declspec(naked) void forward_92() { __asm jmp dword ptr [forwardTargets+368] }
extern "C" __declspec(naked) void forward_93() { __asm jmp dword ptr [forwardTargets+372] }
extern "C" __declspec(naked) void forward_94() { __asm jmp dword ptr [forwardTargets+376] }
extern "C" __declspec(naked) void forward_95() { __asm jmp dword ptr [forwardTargets+380] }
extern "C" __declspec(naked) void forward_96() { __asm jmp dword ptr [forwardTargets+384] }
extern "C" __declspec(naked) void forward_97() { __asm jmp dword ptr [forwardTargets+388] }
extern "C" __declspec(naked) void forward_98() { __asm jmp dword ptr [forwardTargets+392] }
extern "C" __declspec(naked) void forward_99() { __asm jmp dword ptr [forwardTargets+396] }
extern "C" __declspec(naked) void forward_100() { __asm jmp dword ptr [forwardTargets+400] }
extern "C" __declspec(naked) void forward_101() { __asm jmp dword ptr [forwardTargets+404] }
extern "C" __declspec(naked) void forward_102() { __asm jmp dword ptr [forwardTargets+408] }
extern "C" __declspec(naked) void forward_103() { __asm jmp dword ptr [forwardTargets+412] }
extern "C" __declspec(naked) void forward_104() { __asm jmp dword ptr [forwardTargets+416] }
extern "C" __declspec(naked) void forward_105() { __asm jmp dword ptr [forwardTargets+420] }
extern "C" __declspec(naked) void forward_106() { __asm jmp dword ptr [forwardTargets+424] }
extern "C" __declspec(naked) void forward_107() { __asm jmp dword ptr [forwardTargets+428] }
extern "C" __declspec(naked) void forward_108() { __asm jmp dword ptr [forwardTargets+432] }
extern "C" __declspec(naked) void forward_109() { __asm jmp dword ptr [forwardTargets+436] }
extern "C" __declspec(naked) void forward_110() { __asm jmp dword ptr [forwardTargets+440] }
extern "C" __declspec(naked) void forward_111() { __asm jmp dword ptr [forwardTargets+444] }
extern "C" __declspec(naked) void forward_112() { __asm jmp dword ptr [forwardTargets+448] }
extern "C" __declspec(naked) void forward_113() { __asm jmp dword ptr [forwardTargets+452] }
extern "C" __declspec(naked) void forward_114() { __asm jmp dword ptr [forwardTargets+456] }
extern "C" __declspec(naked) void forward_115() { __asm jmp dword ptr [forwardTargets+460] }
extern "C" __declspec(naked) void forward_116() { __asm jmp dword ptr [forwardTargets+464] }
extern "C" __declspec(naked) void forward_117() { __asm jmp dword ptr [forwardTargets+468] }
extern "C" __declspec(naked) void forward_118() { __asm jmp dword ptr [forwardTargets+472] }
extern "C" __declspec(naked) void forward_119() { __asm jmp dword ptr [forwardTargets+476] }
extern "C" __declspec(naked) void forward_120() { __asm jmp dword ptr [forwardTargets+480] }
extern "C" __declspec(naked) void forward_121() { __asm jmp dword ptr [forwardTargets+484] }
extern "C" __declspec(naked) void forward_122() { __asm jmp dword ptr [forwardTargets+488] }
extern "C" __declspec(naked) void forward_123() { __asm jmp dword ptr [forwardTargets+492] }
extern "C" __declspec(naked) void forward_124() { __asm jmp dword ptr [forwardTargets+496] }
extern "C" __declspec(naked) void forward_125() { __asm jmp dword ptr [forwardTargets+500] }
extern "C" __declspec(naked) void forward_126() { __asm jmp dword ptr [forwardTargets+504] }
extern "C" __declspec(naked) void forward_127() { __asm jmp dword ptr [forwardTargets+508] }
extern "C" __declspec(naked) void forward_128() { __asm jmp dword ptr [forwardTargets+512] }
extern "C" __declspec(naked) void forward_129() { __asm jmp dword ptr [forwardTargets+516] }
extern "C" __declspec(naked) void forward_130() { __asm jmp dword ptr [forwardTargets+520] }
extern "C" __declspec(naked) void forward_131() { __asm jmp dword ptr [forwardTargets+524] }
extern "C" __declspec(naked) void forward_132() { __asm jmp dword ptr [forwardTargets+528] }
extern "C" __declspec(naked) void forward_133() { __asm jmp dword ptr [forwardTargets+532] }
extern "C" __declspec(naked) void forward_134() { __asm jmp dword ptr [forwardTargets+536] }
extern "C" __declspec(naked) void forward_135() { __asm jmp dword ptr [forwardTargets+540] }
extern "C" __declspec(naked) void forward_136() { __asm jmp dword ptr [forwardTargets+544] }
extern "C" __declspec(naked) void forward_137() { __asm jmp dword ptr [forwardTargets+548] }
extern "C" __declspec(naked) void forward_138() { __asm jmp dword ptr [forwardTargets+552] }
extern "C" __declspec(naked) void forward_139() { __asm jmp dword ptr [forwardTargets+556] }
extern "C" __declspec(naked) void forward_140() { __asm jmp dword ptr [forwardTargets+560] }
extern "C" __declspec(naked) void forward_141() { __asm jmp dword ptr [forwardTargets+564] }
extern "C" __declspec(naked) void forward_142() { __asm jmp dword ptr [forwardTargets+568] }
extern "C" __declspec(naked) void forward_143() { __asm jmp dword ptr [forwardTargets+572] }
extern "C" __declspec(naked) void forward_144() { __asm jmp dword ptr [forwardTargets+576] }
extern "C" __declspec(naked) void forward_145() { __asm jmp dword ptr [forwardTargets+580] }
extern "C" __declspec(naked) void forward_146() { __asm jmp dword ptr [forwardTargets+584] }
extern "C" __declspec(naked) void forward_147() { __asm jmp dword ptr [forwardTargets+588] }
extern "C" __declspec(naked) void forward_148() { __asm jmp dword ptr [forwardTargets+592] }
extern "C" __declspec(naked) void forward_149() { __asm jmp dword ptr [forwardTargets+596] }
extern "C" __declspec(naked) void forward_150() { __asm jmp dword ptr [forwardTargets+600] }
extern "C" __declspec(naked) void forward_151() { __asm jmp dword ptr [forwardTargets+604] }
extern "C" __declspec(naked) void forward_152() { __asm jmp dword ptr [forwardTargets+608] }
extern "C" __declspec(naked) void forward_153() { __asm jmp dword ptr [forwardTargets+612] }
extern "C" __declspec(naked) void forward_154() { __asm jmp dword ptr [forwardTargets+616] }
extern "C" __declspec(naked) void forward_155() { __asm jmp dword ptr [forwardTargets+620] }
extern "C" __declspec(naked) void forward_156() { __asm jmp dword ptr [forwardTargets+624] }
extern "C" __declspec(naked) void forward_157() { __asm jmp dword ptr [forwardTargets+628] }
extern "C" __declspec(naked) void forward_158() { __asm jmp dword ptr [forwardTargets+632] }
extern "C" __declspec(naked) void forward_159() { __asm jmp dword ptr [forwardTargets+636] }
extern "C" __declspec(naked) void forward_160() { __asm jmp dword ptr [forwardTargets+640] }
extern "C" __declspec(naked) void forward_161() { __asm jmp dword ptr [forwardTargets+644] }
extern "C" __declspec(naked) void forward_162() { __asm jmp dword ptr [forwardTargets+648] }
extern "C" __declspec(naked) void forward_163() { __asm jmp dword ptr [forwardTargets+652] }
extern "C" __declspec(naked) void forward_164() { __asm jmp dword ptr [forwardTargets+656] }
extern "C" __declspec(naked) void forward_165() { __asm jmp dword ptr [forwardTargets+660] }
extern "C" __declspec(naked) void forward_166() { __asm jmp dword ptr [forwardTargets+664] }
extern "C" __declspec(naked) void forward_167() { __asm jmp dword ptr [forwardTargets+668] }
extern "C" __declspec(naked) void forward_168() { __asm jmp dword ptr [forwardTargets+672] }
extern "C" __declspec(naked) void forward_169() { __asm jmp dword ptr [forwardTargets+676] }
extern "C" __declspec(naked) void forward_170() { __asm jmp dword ptr [forwardTargets+680] }
extern "C" __declspec(naked) void forward_171() { __asm jmp dword ptr [forwardTargets+684] }
extern "C" __declspec(naked) void forward_172() { __asm jmp dword ptr [forwardTargets+688] }
extern "C" __declspec(naked) void forward_173() { __asm jmp dword ptr [forwardTargets+692] }
extern "C" __declspec(naked) void forward_174() { __asm jmp dword ptr [forwardTargets+696] }
extern "C" __declspec(naked) void forward_175() { __asm jmp dword ptr [forwardTargets+700] }
extern "C" __declspec(naked) void forward_176() { __asm jmp dword ptr [forwardTargets+704] }
extern "C" __declspec(naked) void forward_177() { __asm jmp dword ptr [forwardTargets+708] }
extern "C" __declspec(naked) void forward_178() { __asm jmp dword ptr [forwardTargets+712] }
extern "C" __declspec(naked) void forward_179() { __asm jmp dword ptr [forwardTargets+716] }
extern "C" __declspec(naked) void forward_180() { __asm jmp dword ptr [forwardTargets+720] }
extern "C" __declspec(naked) void forward_181() { __asm jmp dword ptr [forwardTargets+724] }
extern "C" __declspec(naked) void forward_182() { __asm jmp dword ptr [forwardTargets+728] }
extern "C" __declspec(naked) void forward_183() { __asm jmp dword ptr [forwardTargets+732] }
extern "C" __declspec(naked) void forward_184() { __asm jmp dword ptr [forwardTargets+736] }
extern "C" __declspec(naked) void forward_185() { __asm jmp dword ptr [forwardTargets+740] }
extern "C" __declspec(naked) void forward_186() { __asm jmp dword ptr [forwardTargets+744] }
extern "C" __declspec(naked) void forward_187() { __asm jmp dword ptr [forwardTargets+748] }
extern "C" __declspec(naked) void forward_188() { __asm jmp dword ptr [forwardTargets+752] }
extern "C" __declspec(naked) void forward_189() { __asm jmp dword ptr [forwardTargets+756] }
extern "C" __declspec(naked) void forward_190() { __asm jmp dword ptr [forwardTargets+760] }
extern "C" __declspec(naked) void forward_191() { __asm jmp dword ptr [forwardTargets+764] }
extern "C" __declspec(naked) void forward_192() { __asm jmp dword ptr [forwardTargets+768] }
extern "C" __declspec(naked) void forward_193() { __asm jmp dword ptr [forwardTargets+772] }
extern "C" __declspec(naked) void forward_194() { __asm jmp dword ptr [forwardTargets+776] }
extern "C" __declspec(naked) void forward_195() { __asm jmp dword ptr [forwardTargets+780] }
extern "C" __declspec(naked) void forward_196() { __asm jmp dword ptr [forwardTargets+784] }
extern "C" __declspec(naked) void forward_197() { __asm jmp dword ptr [forwardTargets+788] }
extern "C" __declspec(naked) void forward_198() { __asm jmp dword ptr [forwardTargets+792] }
extern "C" __declspec(naked) void forward_199() { __asm jmp dword ptr [forwardTargets+796] }
extern "C" __declspec(naked) void forward_200() { __asm jmp dword ptr [forwardTargets+800] }
extern "C" __declspec(naked) void forward_201() { __asm jmp dword ptr [forwardTargets+804] }
extern "C" __declspec(naked) void forward_202() { __asm jmp dword ptr [forwardTargets+808] }
extern "C" __declspec(naked) void forward_203() { __asm jmp dword ptr [forwardTargets+812] }
extern "C" __declspec(naked) void forward_204() { __asm jmp dword ptr [forwardTargets+816] }
extern "C" __declspec(naked) void forward_205() { __asm jmp dword ptr [forwardTargets+820] }
extern "C" __declspec(naked) void forward_206() { __asm jmp dword ptr [forwardTargets+824] }
extern "C" __declspec(naked) void forward_207() { __asm jmp dword ptr [forwardTargets+828] }
extern "C" __declspec(naked) void forward_208() { __asm jmp dword ptr [forwardTargets+832] }
extern "C" __declspec(naked) void forward_209() { __asm jmp dword ptr [forwardTargets+836] }
extern "C" __declspec(naked) void forward_210() { __asm jmp dword ptr [forwardTargets+840] }
extern "C" __declspec(naked) void forward_211() { __asm jmp dword ptr [forwardTargets+844] }
extern "C" __declspec(naked) void forward_212() { __asm jmp dword ptr [forwardTargets+848] }
extern "C" __declspec(naked) void forward_213() { __asm jmp dword ptr [forwardTargets+852] }
extern "C" __declspec(naked) void forward_214() { __asm jmp dword ptr [forwardTargets+856] }
extern "C" __declspec(naked) void forward_215() { __asm jmp dword ptr [forwardTargets+860] }
extern "C" __declspec(naked) void forward_216() { __asm jmp dword ptr [forwardTargets+864] }
extern "C" __declspec(naked) void forward_217() { __asm jmp dword ptr [forwardTargets+868] }
extern "C" __declspec(naked) void forward_218() { __asm jmp dword ptr [forwardTargets+872] }
extern "C" __declspec(naked) void forward_219() { __asm jmp dword ptr [forwardTargets+876] }
extern "C" __declspec(naked) void forward_220() { __asm jmp dword ptr [forwardTargets+880] }
extern "C" __declspec(naked) void forward_221() { __asm jmp dword ptr [forwardTargets+884] }
extern "C" __declspec(naked) void forward_222() { __asm jmp dword ptr [forwardTargets+888] }
extern "C" __declspec(naked) void forward_223() { __asm jmp dword ptr [forwardTargets+892] }
extern "C" __declspec(naked) void forward_224() { __asm jmp dword ptr [forwardTargets+896] }
extern "C" __declspec(naked) void forward_225() { __asm jmp dword ptr [forwardTargets+900] }
extern "C" __declspec(naked) void forward_226() { __asm jmp dword ptr [forwardTargets+904] }
extern "C" __declspec(naked) void forward_227() { __asm jmp dword ptr [forwardTargets+908] }
extern "C" __declspec(naked) void forward_228() { __asm jmp dword ptr [forwardTargets+912] }
extern "C" __declspec(naked) void forward_229() { __asm jmp dword ptr [forwardTargets+916] }
extern "C" __declspec(naked) void forward_230() { __asm jmp dword ptr [forwardTargets+920] }
extern "C" __declspec(naked) void forward_231() { __asm jmp dword ptr [forwardTargets+924] }
extern "C" __declspec(naked) void forward_232() { __asm jmp dword ptr [forwardTargets+928] }
extern "C" __declspec(naked) void forward_233() { __asm jmp dword ptr [forwardTargets+932] }
extern "C" __declspec(naked) void forward_234() { __asm jmp dword ptr [forwardTargets+936] }
extern "C" __declspec(naked) void forward_235() { __asm jmp dword ptr [forwardTargets+940] }
extern "C" __declspec(naked) void forward_236() { __asm jmp dword ptr [forwardTargets+944] }
extern "C" __declspec(naked) void forward_237() { __asm jmp dword ptr [forwardTargets+948] }
extern "C" __declspec(naked) void forward_238() { __asm jmp dword ptr [forwardTargets+952] }
extern "C" __declspec(naked) void forward_239() { __asm jmp dword ptr [forwardTargets+956] }
extern "C" __declspec(naked) void forward_240() { __asm jmp dword ptr [forwardTargets+960] }
extern "C" __declspec(naked) void forward_241() { __asm jmp dword ptr [forwardTargets+964] }
extern "C" __declspec(naked) void forward_242() { __asm jmp dword ptr [forwardTargets+968] }
extern "C" __declspec(naked) void forward_243() { __asm jmp dword ptr [forwardTargets+972] }
extern "C" __declspec(naked) void forward_244() { __asm jmp dword ptr [forwardTargets+976] }
extern "C" __declspec(naked) void forward_245() { __asm jmp dword ptr [forwardTargets+980] }
extern "C" __declspec(naked) void forward_246() { __asm jmp dword ptr [forwardTargets+984] }
extern "C" __declspec(naked) void forward_247() { __asm jmp dword ptr [forwardTargets+988] }
extern "C" __declspec(naked) void forward_248() { __asm jmp dword ptr [forwardTargets+992] }
extern "C" __declspec(naked) void forward_249() { __asm jmp dword ptr [forwardTargets+996] }
extern "C" __declspec(naked) void forward_250() { __asm jmp dword ptr [forwardTargets+1000] }
extern "C" __declspec(naked) void forward_251() { __asm jmp dword ptr [forwardTargets+1004] }
extern "C" __declspec(naked) void forward_252() { __asm jmp dword ptr [forwardTargets+1008] }
extern "C" __declspec(naked) void forward_253() { __asm jmp dword ptr [forwardTargets+1012] }
extern "C" __declspec(naked) void forward_254() { __asm jmp dword ptr [forwardTargets+1016] }
extern "C" __declspec(naked) void forward_255() { __asm jmp dword ptr [forwardTargets+1020] }
extern "C" __declspec(naked) void forward_256() { __asm jmp dword ptr [forwardTargets+1024] }
extern "C" __declspec(naked) void forward_257() { __asm jmp dword ptr [forwardTargets+1028] }
extern "C" __declspec(naked) void forward_258() { __asm jmp dword ptr [forwardTargets+1032] }
extern "C" __declspec(naked) void forward_259() { __asm jmp dword ptr [forwardTargets+1036] }
extern "C" __declspec(naked) void forward_260() { __asm jmp dword ptr [forwardTargets+1040] }
extern "C" __declspec(naked) void forward_261() { __asm jmp dword ptr [forwardTargets+1044] }
extern "C" __declspec(naked) void forward_262() { __asm jmp dword ptr [forwardTargets+1048] }
extern "C" __declspec(naked) void forward_263() { __asm jmp dword ptr [forwardTargets+1052] }
extern "C" __declspec(naked) void forward_264() { __asm jmp dword ptr [forwardTargets+1056] }
extern "C" __declspec(naked) void forward_265() { __asm jmp dword ptr [forwardTargets+1060] }
extern "C" __declspec(naked) void forward_266() { __asm jmp dword ptr [forwardTargets+1064] }
extern "C" __declspec(naked) void forward_267() { __asm jmp dword ptr [forwardTargets+1068] }
extern "C" __declspec(naked) void forward_268() { __asm jmp dword ptr [forwardTargets+1072] }
extern "C" __declspec(naked) void forward_269() { __asm jmp dword ptr [forwardTargets+1076] }
extern "C" __declspec(naked) void forward_270() { __asm jmp dword ptr [forwardTargets+1080] }
extern "C" __declspec(naked) void forward_271() { __asm jmp dword ptr [forwardTargets+1084] }
extern "C" __declspec(naked) void forward_272() { __asm jmp dword ptr [forwardTargets+1088] }
extern "C" __declspec(naked) void forward_273() { __asm jmp dword ptr [forwardTargets+1092] }
extern "C" __declspec(naked) void forward_274() { __asm jmp dword ptr [forwardTargets+1096] }
extern "C" __declspec(naked) void forward_275() { __asm jmp dword ptr [forwardTargets+1100] }
extern "C" __declspec(naked) void forward_276() { __asm jmp dword ptr [forwardTargets+1104] }
extern "C" __declspec(naked) void forward_277() { __asm jmp dword ptr [forwardTargets+1108] }
extern "C" __declspec(naked) void forward_278() { __asm jmp dword ptr [forwardTargets+1112] }
extern "C" __declspec(naked) void forward_279() { __asm jmp dword ptr [forwardTargets+1116] }
extern "C" __declspec(naked) void forward_280() { __asm jmp dword ptr [forwardTargets+1120] }
extern "C" __declspec(naked) void forward_281() { __asm jmp dword ptr [forwardTargets+1124] }
extern "C" __declspec(naked) void forward_282() { __asm jmp dword ptr [forwardTargets+1128] }
extern "C" __declspec(naked) void forward_283() { __asm jmp dword ptr [forwardTargets+1132] }
extern "C" __declspec(naked) void forward_284() { __asm jmp dword ptr [forwardTargets+1136] }
extern "C" __declspec(naked) void forward_285() { __asm jmp dword ptr [forwardTargets+1140] }
extern "C" __declspec(naked) void forward_286() { __asm jmp dword ptr [forwardTargets+1144] }
extern "C" __declspec(naked) void forward_287() { __asm jmp dword ptr [forwardTargets+1148] }
extern "C" __declspec(naked) void forward_288() { __asm jmp dword ptr [forwardTargets+1152] }
extern "C" __declspec(naked) void forward_289() { __asm jmp dword ptr [forwardTargets+1156] }
extern "C" __declspec(naked) void forward_290() { __asm jmp dword ptr [forwardTargets+1160] }
extern "C" __declspec(naked) void forward_291() { __asm jmp dword ptr [forwardTargets+1164] }
extern "C" __declspec(naked) void forward_292() { __asm jmp dword ptr [forwardTargets+1168] }
extern "C" __declspec(naked) void forward_293() { __asm jmp dword ptr [forwardTargets+1172] }
extern "C" __declspec(naked) void forward_294() { __asm jmp dword ptr [forwardTargets+1176] }
extern "C" __declspec(naked) void forward_295() { __asm jmp dword ptr [forwardTargets+1180] }
extern "C" __declspec(naked) void forward_296() { __asm jmp dword ptr [forwardTargets+1184] }
extern "C" __declspec(naked) void forward_297() { __asm jmp dword ptr [forwardTargets+1188] }
extern "C" __declspec(naked) void forward_298() { __asm jmp dword ptr [forwardTargets+1192] }
extern "C" __declspec(naked) void forward_299() { __asm jmp dword ptr [forwardTargets+1196] }
extern "C" __declspec(naked) void forward_300() { __asm jmp dword ptr [forwardTargets+1200] }
extern "C" __declspec(naked) void forward_301() { __asm jmp dword ptr [forwardTargets+1204] }
extern "C" __declspec(naked) void forward_302() { __asm jmp dword ptr [forwardTargets+1208] }
extern "C" __declspec(naked) void forward_303() { __asm jmp dword ptr [forwardTargets+1212] }
extern "C" __declspec(naked) void forward_304() { __asm jmp dword ptr [forwardTargets+1216] }
extern "C" __declspec(naked) void forward_305() { __asm jmp dword ptr [forwardTargets+1220] }
extern "C" __declspec(naked) void forward_306() { __asm jmp dword ptr [forwardTargets+1224] }
extern "C" __declspec(naked) void forward_307() { __asm jmp dword ptr [forwardTargets+1228] }
extern "C" __declspec(naked) void forward_308() { __asm jmp dword ptr [forwardTargets+1232] }
extern "C" __declspec(naked) void forward_309() { __asm jmp dword ptr [forwardTargets+1236] }
extern "C" __declspec(naked) void forward_310() { __asm jmp dword ptr [forwardTargets+1240] }
extern "C" __declspec(naked) void forward_311() { __asm jmp dword ptr [forwardTargets+1244] }
extern "C" __declspec(naked) void forward_312() { __asm jmp dword ptr [forwardTargets+1248] }
extern "C" __declspec(naked) void forward_313() { __asm jmp dword ptr [forwardTargets+1252] }
extern "C" __declspec(naked) void forward_314() { __asm jmp dword ptr [forwardTargets+1256] }
extern "C" __declspec(naked) void forward_315() { __asm jmp dword ptr [forwardTargets+1260] }
extern "C" __declspec(naked) void forward_316() { __asm jmp dword ptr [forwardTargets+1264] }
extern "C" __declspec(naked) void forward_317() { __asm jmp dword ptr [forwardTargets+1268] }
extern "C" __declspec(naked) void forward_318() { __asm jmp dword ptr [forwardTargets+1272] }
extern "C" __declspec(naked) void forward_319() { __asm jmp dword ptr [forwardTargets+1276] }
extern "C" __declspec(naked) void forward_320() { __asm jmp dword ptr [forwardTargets+1280] }
extern "C" __declspec(naked) void forward_321() { __asm jmp dword ptr [forwardTargets+1284] }
extern "C" __declspec(naked) void forward_322() { __asm jmp dword ptr [forwardTargets+1288] }
extern "C" __declspec(naked) void forward_323() { __asm jmp dword ptr [forwardTargets+1292] }
extern "C" __declspec(naked) void forward_324() { __asm jmp dword ptr [forwardTargets+1296] }
extern "C" __declspec(naked) void forward_325() { __asm jmp dword ptr [forwardTargets+1300] }
extern "C" __declspec(naked) void forward_326() { __asm jmp dword ptr [forwardTargets+1304] }
extern "C" __declspec(naked) void forward_327() { __asm jmp dword ptr [forwardTargets+1308] }
extern "C" __declspec(naked) void forward_328() { __asm jmp dword ptr [forwardTargets+1312] }
extern "C" __declspec(naked) void forward_329() { __asm jmp dword ptr [forwardTargets+1316] }
extern "C" __declspec(naked) void forward_330() { __asm jmp dword ptr [forwardTargets+1320] }
extern "C" __declspec(naked) void forward_331() { __asm jmp dword ptr [forwardTargets+1324] }
extern "C" __declspec(naked) void forward_332() { __asm jmp dword ptr [forwardTargets+1328] }
extern "C" __declspec(naked) void forward_333() { __asm jmp dword ptr [forwardTargets+1332] }
extern "C" __declspec(naked) void forward_334() { __asm jmp dword ptr [forwardTargets+1336] }
extern "C" __declspec(naked) void forward_335() { __asm jmp dword ptr [forwardTargets+1340] }
extern "C" __declspec(naked) void forward_336() { __asm jmp dword ptr [forwardTargets+1344] }
extern "C" __declspec(naked) void forward_337() { __asm jmp dword ptr [forwardTargets+1348] }
extern "C" __declspec(naked) void forward_338() { __asm jmp dword ptr [forwardTargets+1352] }
extern "C" __declspec(naked) void forward_339() { __asm jmp dword ptr [forwardTargets+1356] }
extern "C" __declspec(naked) void forward_340() { __asm jmp dword ptr [forwardTargets+1360] }
extern "C" __declspec(naked) void forward_341() { __asm jmp dword ptr [forwardTargets+1364] }
extern "C" __declspec(naked) void forward_342() { __asm jmp dword ptr [forwardTargets+1368] }
extern "C" __declspec(naked) void forward_343() { __asm jmp dword ptr [forwardTargets+1372] }
extern "C" __declspec(naked) void forward_344() { __asm jmp dword ptr [forwardTargets+1376] }
extern "C" __declspec(naked) void forward_345() { __asm jmp dword ptr [forwardTargets+1380] }
extern "C" __declspec(naked) void forward_346() { __asm jmp dword ptr [forwardTargets+1384] }
extern "C" __declspec(naked) void forward_347() { __asm jmp dword ptr [forwardTargets+1388] }
extern "C" __declspec(naked) void forward_348() { __asm jmp dword ptr [forwardTargets+1392] }
extern "C" __declspec(naked) void forward_349() { __asm jmp dword ptr [forwardTargets+1396] }
extern "C" __declspec(naked) void forward_350() { __asm jmp dword ptr [forwardTargets+1400] }
extern "C" __declspec(naked) void forward_351() { __asm jmp dword ptr [forwardTargets+1404] }
extern "C" __declspec(naked) void forward_352() { __asm jmp dword ptr [forwardTargets+1408] }
extern "C" __declspec(naked) void forward_353() { __asm jmp dword ptr [forwardTargets+1412] }
extern "C" __declspec(naked) void forward_354() { __asm jmp dword ptr [forwardTargets+1416] }
extern "C" __declspec(naked) void forward_355() { __asm jmp dword ptr [forwardTargets+1420] }
extern "C" __declspec(naked) void forward_356() { __asm jmp dword ptr [forwardTargets+1424] }
extern "C" __declspec(naked) void forward_357() { __asm jmp dword ptr [forwardTargets+1428] }
extern "C" __declspec(naked) void forward_358() { __asm jmp dword ptr [forwardTargets+1432] }
extern "C" __declspec(naked) void forward_359() { __asm jmp dword ptr [forwardTargets+1436] }
extern "C" __declspec(naked) void forward_360() { __asm jmp dword ptr [forwardTargets+1440] }
extern "C" __declspec(naked) void forward_361() { __asm jmp dword ptr [forwardTargets+1444] }
extern "C" __declspec(naked) void forward_362() { __asm jmp dword ptr [forwardTargets+1448] }
extern "C" __declspec(naked) void forward_363() { __asm jmp dword ptr [forwardTargets+1452] }
extern "C" __declspec(naked) void forward_364() { __asm jmp dword ptr [forwardTargets+1456] }
extern "C" __declspec(naked) void forward_365() { __asm jmp dword ptr [forwardTargets+1460] }
extern "C" __declspec(naked) void forward_366() { __asm jmp dword ptr [forwardTargets+1464] }
extern "C" __declspec(naked) void forward_367() { __asm jmp dword ptr [forwardTargets+1468] }
extern "C" __declspec(naked) void forward_368() { __asm jmp dword ptr [forwardTargets+1472] }
extern "C" __declspec(naked) void forward_369() { __asm jmp dword ptr [forwardTargets+1476] }
extern "C" __declspec(naked) void forward_370() { __asm jmp dword ptr [forwardTargets+1480] }
extern "C" __declspec(naked) void forward_371() { __asm jmp dword ptr [forwardTargets+1484] }
extern "C" __declspec(naked) void forward_372() { __asm jmp dword ptr [forwardTargets+1488] }
extern "C" __declspec(naked) void forward_373() { __asm jmp dword ptr [forwardTargets+1492] }
extern "C" __declspec(naked) void forward_374() { __asm jmp dword ptr [forwardTargets+1496] }
extern "C" __declspec(naked) void forward_375() { __asm jmp dword ptr [forwardTargets+1500] }
extern "C" __declspec(naked) void forward_376() { __asm jmp dword ptr [forwardTargets+1504] }
extern "C" __declspec(naked) void forward_377() { __asm jmp dword ptr [forwardTargets+1508] }
extern "C" __declspec(naked) void forward_378() { __asm jmp dword ptr [forwardTargets+1512] }
extern "C" __declspec(naked) void forward_379() { __asm jmp dword ptr [forwardTargets+1516] }
extern "C" __declspec(naked) void forward_380() { __asm jmp dword ptr [forwardTargets+1520] }
extern "C" __declspec(naked) void forward_381() { __asm jmp dword ptr [forwardTargets+1524] }
extern "C" __declspec(naked) void forward_382() { __asm jmp dword ptr [forwardTargets+1528] }
extern "C" __declspec(naked) void forward_383() { __asm jmp dword ptr [forwardTargets+1532] }
extern "C" __declspec(naked) void forward_384() { __asm jmp dword ptr [forwardTargets+1536] }
extern "C" __declspec(naked) void forward_385() { __asm jmp dword ptr [forwardTargets+1540] }
extern "C" __declspec(naked) void forward_386() { __asm jmp dword ptr [forwardTargets+1544] }
extern "C" __declspec(naked) void forward_387() { __asm jmp dword ptr [forwardTargets+1548] }
extern "C" __declspec(naked) void forward_388() { __asm jmp dword ptr [forwardTargets+1552] }
extern "C" __declspec(naked) void forward_389() { __asm jmp dword ptr [forwardTargets+1556] }
extern "C" __declspec(naked) void forward_390() { __asm jmp dword ptr [forwardTargets+1560] }
extern "C" __declspec(naked) void forward_391() { __asm jmp dword ptr [forwardTargets+1564] }
extern "C" __declspec(naked) void forward_392() { __asm jmp dword ptr [forwardTargets+1568] }
extern "C" __declspec(naked) void forward_393() { __asm jmp dword ptr [forwardTargets+1572] }
extern "C" __declspec(naked) void forward_394() { __asm jmp dword ptr [forwardTargets+1576] }
extern "C" __declspec(naked) void forward_395() { __asm jmp dword ptr [forwardTargets+1580] }
extern "C" __declspec(naked) void forward_396() { __asm jmp dword ptr [forwardTargets+1584] }
extern "C" __declspec(naked) void forward_397() { __asm jmp dword ptr [forwardTargets+1588] }
extern "C" __declspec(naked) void forward_398() { __asm jmp dword ptr [forwardTargets+1592] }
extern "C" __declspec(naked) void forward_399() { __asm jmp dword ptr [forwardTargets+1596] }
extern "C" __declspec(naked) void forward_400() { __asm jmp dword ptr [forwardTargets+1600] }
extern "C" __declspec(naked) void forward_401() { __asm jmp dword ptr [forwardTargets+1604] }
extern "C" __declspec(naked) void forward_402() { __asm jmp dword ptr [forwardTargets+1608] }
extern "C" __declspec(naked) void forward_403() { __asm jmp dword ptr [forwardTargets+1612] }
extern "C" __declspec(naked) void forward_404() { __asm jmp dword ptr [forwardTargets+1616] }
extern "C" __declspec(naked) void forward_405() { __asm jmp dword ptr [forwardTargets+1620] }
extern "C" __declspec(naked) void forward_406() { __asm jmp dword ptr [forwardTargets+1624] }
extern "C" __declspec(naked) void forward_407() { __asm jmp dword ptr [forwardTargets+1628] }
extern "C" __declspec(naked) void forward_408() { __asm jmp dword ptr [forwardTargets+1632] }
extern "C" __declspec(naked) void forward_409() { __asm jmp dword ptr [forwardTargets+1636] }
extern "C" __declspec(naked) void forward_410() { __asm jmp dword ptr [forwardTargets+1640] }
extern "C" __declspec(naked) void forward_411() { __asm jmp dword ptr [forwardTargets+1644] }
extern "C" __declspec(naked) void forward_412() { __asm jmp dword ptr [forwardTargets+1648] }
extern "C" __declspec(naked) void forward_413() { __asm jmp dword ptr [forwardTargets+1652] }
extern "C" __declspec(naked) void forward_414() { __asm jmp dword ptr [forwardTargets+1656] }
extern "C" __declspec(naked) void forward_415() { __asm jmp dword ptr [forwardTargets+1660] }
extern "C" __declspec(naked) void forward_416() { __asm jmp dword ptr [forwardTargets+1664] }
extern "C" __declspec(naked) void forward_417() { __asm jmp dword ptr [forwardTargets+1668] }
extern "C" __declspec(naked) void forward_418() { __asm jmp dword ptr [forwardTargets+1672] }
extern "C" __declspec(naked) void forward_419() { __asm jmp dword ptr [forwardTargets+1676] }
extern "C" __declspec(naked) void forward_420() { __asm jmp dword ptr [forwardTargets+1680] }
extern "C" __declspec(naked) void forward_421() { __asm jmp dword ptr [forwardTargets+1684] }
extern "C" __declspec(naked) void forward_422() { __asm jmp dword ptr [forwardTargets+1688] }
extern "C" __declspec(naked) void forward_423() { __asm jmp dword ptr [forwardTargets+1692] }
extern "C" __declspec(naked) void forward_424() { __asm jmp dword ptr [forwardTargets+1696] }
extern "C" __declspec(naked) void forward_425() { __asm jmp dword ptr [forwardTargets+1700] }
extern "C" __declspec(naked) void forward_426() { __asm jmp dword ptr [forwardTargets+1704] }
extern "C" __declspec(naked) void forward_427() { __asm jmp dword ptr [forwardTargets+1708] }
extern "C" __declspec(naked) void forward_428() { __asm jmp dword ptr [forwardTargets+1712] }
extern "C" __declspec(naked) void forward_429() { __asm jmp dword ptr [forwardTargets+1716] }
extern "C" __declspec(naked) void forward_430() { __asm jmp dword ptr [forwardTargets+1720] }
extern "C" __declspec(naked) void forward_431() { __asm jmp dword ptr [forwardTargets+1724] }
extern "C" __declspec(naked) void forward_432() { __asm jmp dword ptr [forwardTargets+1728] }
extern "C" __declspec(naked) void forward_433() { __asm jmp dword ptr [forwardTargets+1732] }
extern "C" __declspec(naked) void forward_434() { __asm jmp dword ptr [forwardTargets+1736] }
extern "C" __declspec(naked) void forward_435() { __asm jmp dword ptr [forwardTargets+1740] }
extern "C" __declspec(naked) void forward_436() { __asm jmp dword ptr [forwardTargets+1744] }
extern "C" __declspec(naked) void forward_437() { __asm jmp dword ptr [forwardTargets+1748] }
extern "C" __declspec(naked) void forward_438() { __asm jmp dword ptr [forwardTargets+1752] }
extern "C" __declspec(naked) void forward_439() { __asm jmp dword ptr [forwardTargets+1756] }
extern "C" __declspec(naked) void forward_440() { __asm jmp dword ptr [forwardTargets+1760] }
extern "C" __declspec(naked) void forward_441() { __asm jmp dword ptr [forwardTargets+1764] }
extern "C" __declspec(naked) void forward_442() { __asm jmp dword ptr [forwardTargets+1768] }
extern "C" __declspec(naked) void forward_443() { __asm jmp dword ptr [forwardTargets+1772] }
extern "C" __declspec(naked) void forward_444() { __asm jmp dword ptr [forwardTargets+1776] }
extern "C" __declspec(naked) void forward_445() { __asm jmp dword ptr [forwardTargets+1780] }
extern "C" __declspec(naked) void forward_446() { __asm jmp dword ptr [forwardTargets+1784] }
extern "C" __declspec(naked) void forward_447() { __asm jmp dword ptr [forwardTargets+1788] }
extern "C" __declspec(naked) void forward_448() { __asm jmp dword ptr [forwardTargets+1792] }
extern "C" __declspec(naked) void forward_449() { __asm jmp dword ptr [forwardTargets+1796] }
extern "C" __declspec(naked) void forward_450() { __asm jmp dword ptr [forwardTargets+1800] }
extern "C" __declspec(naked) void forward_451() { __asm jmp dword ptr [forwardTargets+1804] }
extern "C" __declspec(naked) void forward_452() { __asm jmp dword ptr [forwardTargets+1808] }
extern "C" __declspec(naked) void forward_453() { __asm jmp dword ptr [forwardTargets+1812] }
extern "C" __declspec(naked) void forward_454() { __asm jmp dword ptr [forwardTargets+1816] }
extern "C" __declspec(naked) void forward_455() { __asm jmp dword ptr [forwardTargets+1820] }
extern "C" __declspec(naked) void forward_456() { __asm jmp dword ptr [forwardTargets+1824] }
extern "C" __declspec(naked) void forward_457() { __asm jmp dword ptr [forwardTargets+1828] }
extern "C" __declspec(naked) void forward_458() { __asm jmp dword ptr [forwardTargets+1832] }
extern "C" __declspec(naked) void forward_459() { __asm jmp dword ptr [forwardTargets+1836] }
extern "C" __declspec(naked) void forward_460() { __asm jmp dword ptr [forwardTargets+1840] }
extern "C" __declspec(naked) void forward_461() { __asm jmp dword ptr [forwardTargets+1844] }
extern "C" __declspec(naked) void forward_462() { __asm jmp dword ptr [forwardTargets+1848] }
extern "C" __declspec(naked) void forward_463() { __asm jmp dword ptr [forwardTargets+1852] }
extern "C" __declspec(naked) void forward_464() { __asm jmp dword ptr [forwardTargets+1856] }
extern "C" __declspec(naked) void forward_465() { __asm jmp dword ptr [forwardTargets+1860] }
extern "C" __declspec(naked) void forward_466() { __asm jmp dword ptr [forwardTargets+1864] }
extern "C" __declspec(naked) void forward_467() { __asm jmp dword ptr [forwardTargets+1868] }
extern "C" __declspec(naked) void forward_468() { __asm jmp dword ptr [forwardTargets+1872] }
extern "C" __declspec(naked) void forward_469() { __asm jmp dword ptr [forwardTargets+1876] }
extern "C" __declspec(naked) void forward_470() { __asm jmp dword ptr [forwardTargets+1880] }
extern "C" __declspec(naked) void forward_471() { __asm jmp dword ptr [forwardTargets+1884] }
extern "C" __declspec(naked) void forward_472() { __asm jmp dword ptr [forwardTargets+1888] }
extern "C" __declspec(naked) void forward_473() { __asm jmp dword ptr [forwardTargets+1892] }
extern "C" __declspec(naked) void forward_474() { __asm jmp dword ptr [forwardTargets+1896] }
extern "C" __declspec(naked) void forward_475() { __asm jmp dword ptr [forwardTargets+1900] }
extern "C" __declspec(naked) void forward_476() { __asm jmp dword ptr [forwardTargets+1904] }
extern "C" __declspec(naked) void forward_477() { __asm jmp dword ptr [forwardTargets+1908] }
extern "C" __declspec(naked) void forward_478() { __asm jmp dword ptr [forwardTargets+1912] }
extern "C" __declspec(naked) void forward_479() { __asm jmp dword ptr [forwardTargets+1916] }
extern "C" __declspec(naked) void forward_480() { __asm jmp dword ptr [forwardTargets+1920] }
extern "C" __declspec(naked) void forward_481() { __asm jmp dword ptr [forwardTargets+1924] }
extern "C" __declspec(naked) void forward_482() { __asm jmp dword ptr [forwardTargets+1928] }
extern "C" __declspec(naked) void forward_483() { __asm jmp dword ptr [forwardTargets+1932] }
extern "C" __declspec(naked) void forward_484() { __asm jmp dword ptr [forwardTargets+1936] }
extern "C" __declspec(naked) void forward_485() { __asm jmp dword ptr [forwardTargets+1940] }
extern "C" __declspec(naked) void forward_486() { __asm jmp dword ptr [forwardTargets+1944] }
extern "C" __declspec(naked) void forward_487() { __asm jmp dword ptr [forwardTargets+1948] }
extern "C" __declspec(naked) void forward_488() { __asm jmp dword ptr [forwardTargets+1952] }
extern "C" __declspec(naked) void forward_489() { __asm jmp dword ptr [forwardTargets+1956] }
extern "C" __declspec(naked) void forward_490() { __asm jmp dword ptr [forwardTargets+1960] }
extern "C" __declspec(naked) void forward_491() { __asm jmp dword ptr [forwardTargets+1964] }
extern "C" __declspec(naked) void forward_492() { __asm jmp dword ptr [forwardTargets+1968] }
extern "C" __declspec(naked) void forward_493() { __asm jmp dword ptr [forwardTargets+1972] }
extern "C" __declspec(naked) void forward_494() { __asm jmp dword ptr [forwardTargets+1976] }
extern "C" __declspec(naked) void forward_495() { __asm jmp dword ptr [forwardTargets+1980] }
extern "C" __declspec(naked) void forward_496() { __asm jmp dword ptr [forwardTargets+1984] }
extern "C" __declspec(naked) void forward_497() { __asm jmp dword ptr [forwardTargets+1988] }
extern "C" __declspec(naked) void forward_498() { __asm jmp dword ptr [forwardTargets+1992] }
extern "C" __declspec(naked) void forward_499() { __asm jmp dword ptr [forwardTargets+1996] }
extern "C" __declspec(naked) void forward_500() { __asm jmp dword ptr [forwardTargets+2000] }
extern "C" __declspec(naked) void forward_501() { __asm jmp dword ptr [forwardTargets+2004] }
extern "C" __declspec(naked) void forward_502() { __asm jmp dword ptr [forwardTargets+2008] }
extern "C" __declspec(naked) void forward_503() { __asm jmp dword ptr [forwardTargets+2012] }
extern "C" __declspec(naked) void forward_504() { __asm jmp dword ptr [forwardTargets+2016] }
extern "C" __declspec(naked) void forward_505() { __asm jmp dword ptr [forwardTargets+2020] }
extern "C" __declspec(naked) void forward_506() { __asm jmp dword ptr [forwardTargets+2024] }
extern "C" __declspec(naked) void forward_507() { __asm jmp dword ptr [forwardTargets+2028] }
extern "C" __declspec(naked) void forward_508() { __asm jmp dword ptr [forwardTargets+2032] }
extern "C" __declspec(naked) void forward_509() { __asm jmp dword ptr [forwardTargets+2036] }
extern "C" __declspec(naked) void forward_510() { __asm jmp dword ptr [forwardTargets+2040] }
extern "C" __declspec(naked) void forward_511() { __asm jmp dword ptr [forwardTargets+2044] }
extern "C" __declspec(naked) void forward_512() { __asm jmp dword ptr [forwardTargets+2048] }
extern "C" __declspec(naked) void forward_513() { __asm jmp dword ptr [forwardTargets+2052] }
extern "C" __declspec(naked) void forward_514() { __asm jmp dword ptr [forwardTargets+2056] }
extern "C" __declspec(naked) void forward_515() { __asm jmp dword ptr [forwardTargets+2060] }
extern "C" __declspec(naked) void forward_516() { __asm jmp dword ptr [forwardTargets+2064] }
extern "C" __declspec(naked) void forward_517() { __asm jmp dword ptr [forwardTargets+2068] }
extern "C" __declspec(naked) void forward_518() { __asm jmp dword ptr [forwardTargets+2072] }
extern "C" __declspec(naked) void forward_519() { __asm jmp dword ptr [forwardTargets+2076] }
extern "C" __declspec(naked) void forward_520() { __asm jmp dword ptr [forwardTargets+2080] }

#pragma once

// Tunables for weapon_bounce_shotgun.
//
// Kept apart from ricochet_balance.h on purpose: the disc weapon and this
// shotgun must be adjustable without touching each other. Section numbers refer
// to the implementation specification.
namespace bounce_balance {

// ---------------------------------------------------------------------------
// Inventory and HUD identity (section 2)
// ---------------------------------------------------------------------------
// The weapon rides the native crossbow carrier: it is an unused inventory slot
// in Cremation (no crossbow exists in rapidcore) and gives us a real weapon bit,
// slot bookkeeping and save/restore support. Nothing visible stays a crossbow:
// classname, HUD name, HUD id bytes, icon, models and ammo are ours.
constexpr int CarrierWeaponId = 6;   // WEAPON_CROSSBOW.
constexpr int HudSlot = 2;           // 0-based bucket 3, the shotgun bucket.
constexpr int HudSlotPos = 3;        // mp5=0, shotgun=1, crossbow=2; 3 is free.
// Client-only ammo indices for this weapon. They are never used by the native
// CBasePlayer::m_rgAmmo array, so the shotgun's real reserve is untouched.
constexpr int HudAmmoShells = 20;
constexpr int HudAmmoGrenades = 21;
constexpr int HudMaxShells = 100;
constexpr int HudMaxGrenades = 10;

// ---------------------------------------------------------------------------
// Primary: bouncing buckshot (section 3 and 4)
// ---------------------------------------------------------------------------
constexpr float ReferenceCone = 0.08716f; // Original single-player double shot.
constexpr float SpreadMultiplier = 1.15f; // Slightly wider: +15% in both axes.
constexpr float Cone = ReferenceCone * SpreadMultiplier;
constexpr float PelletDamageMultiplier = 1.5f;
constexpr int Pellets = 12;           // CShotgun::SecondaryAttack, single player.
constexpr float PelletSpeed = 1000.f; // Balance starting point (same as a disc).
constexpr int MaxSurfaceContacts = 3; // 3rd contact removes; two bounces at most.
constexpr float PelletLifetime = 4.f;
constexpr int MaxPellets = 96;        // This weapon only; Ricochet keeps its 24.
constexpr int MaxStepsPerFrame = 8;   // Corner/overlap guard, per traced step.
constexpr float PrimaryCooldown = 1.5f;
constexpr int PrimaryAmmoCost = 2;
constexpr float PelletScale = .35f;   // Reuses the Ricochet disc, much smaller.
constexpr float PelletRadius = 1.5f;  // Presentation only; motion is continuous.
constexpr float PelletMuzzleOffset = 6.f;
constexpr float SurfaceOffset = 1.f;  // Push out of the surface after a bounce.

// ---------------------------------------------------------------------------
// Magazines (section 6)
// ---------------------------------------------------------------------------
// The legacy runtime offers no supported way to write CBasePlayer::m_rgAmmo, so
// this weapon keeps its own magazine and reserve and shows them through AmmoX /
// CurWeapon messages addressed to the client-only indices above.
constexpr int ClipSize = 8;
constexpr int StartShells = 40;
constexpr int StartGrenades = 6;
constexpr int SecondaryAmmoCost = 1;
constexpr float ReloadTime = 1.2f;
constexpr float DeployTime = .6f;

// ---------------------------------------------------------------------------
// Secondary: contact grenade whose damage follows the aim elevation (section 5)
// ---------------------------------------------------------------------------
constexpr float GrenadeSpeed = 800.f;    // CMP5::SecondaryAttack.
constexpr float GrenadeGravity = .5f;    // CGrenade::ShootContact.
constexpr float GrenadeLifetime = 6.f;   // Safety fuse for a grenade in the void.
constexpr float SecondaryCooldown = 1.f;
constexpr int MaxGrenades = 8;           // Concurrent grenades for this weapon.
constexpr float ExplosionSurfaceOffset = 8.f; // Pull the blast out of the wall.
constexpr float GrenadeMuzzleOffset = 16.f;
constexpr float ElevationMinFraction = .25f; // Dmin = 25% of the M203 damage.
constexpr float M203RadiusFactor = 2.5f;     // RadiusDamage: damage * 2.5.
constexpr float GrenadePushHorizontal = 140.f;
constexpr float GrenadePushVertical = 35.f;
constexpr float GrenadePushDuration = .5f;
constexpr float ExplosionScaleMin = 12.f;    // TE_EXPLOSION scale * 10.
constexpr float ExplosionScaleMax = 30.f;

// ---------------------------------------------------------------------------
// Damage configuration keys (section 4 and 5). Values live in skill.cfg:
// sk_plr_buckshot1..3 = 5 and sk_plr_9mmAR_grenade1..3 = 100 in this install.
// ---------------------------------------------------------------------------
constexpr const char* BuckshotSkill = "sk_plr_buckshot";
constexpr const char* M203Skill = "sk_plr_9mmAR_grenade";
constexpr const char* MonsterHeadSkill = "sk_monster_head";
constexpr const char* MonsterChestSkill = "sk_monster_chest";
constexpr const char* MonsterStomachSkill = "sk_monster_stomach";
constexpr const char* MonsterArmSkill = "sk_monster_arm";
constexpr const char* MonsterLegSkill = "sk_monster_leg";
constexpr float FallbackBuckshot = 5.f;
constexpr float FallbackM203 = 100.f;
constexpr float FallbackMonsterHead = 3.f;

// ---------------------------------------------------------------------------
// Shotgun animation sequences. Verified against the donor model header
// (valve/models/v_shotgun.mdl shipped by Ravenholm, 10 sequences):
// 0 sm_idle, 1 shoot, 2 shoot_big, 3 reload, 4 pump, 5 start_reload,
// 6 draw, 7 reholster, 8 idle4, 9 deepidle.
// ---------------------------------------------------------------------------
constexpr int AnimIdle = 0;
constexpr int AnimFire = 1;
constexpr int AnimFire2 = 2;
constexpr int AnimReload = 3;
constexpr int AnimPump = 4;
constexpr int AnimStartReload = 5;
constexpr int AnimDraw = 6;
constexpr int AnimHolster = 7;

} // namespace bounce_balance

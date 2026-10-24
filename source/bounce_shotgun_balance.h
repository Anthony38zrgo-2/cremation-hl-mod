#pragma once

// Tunables for weapon_bounce_shotgun.
//
// Kept apart from ricochet_balance.h on purpose: the disc weapon and this
// shotgun must be adjustable without touching each other. Section numbers refer
// to the implementation specification.
namespace bounce_balance {
inline bool SecondaryShrapnel = true;
inline float SecondaryShrapnelDamage = 1.f;
inline float SecondaryDamageMultiplier = .75f;
inline bool OverkillGib = true;
inline float GrenadeRadius = 48.f;
inline float GrenadeUpwardBoost = 120.f;

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
// Shell index is replaced at runtime by the native shotgun WeaponList index.
// The fallback index is only used by the isolated module tests.
constexpr int HudAmmoShells = 20;
constexpr int HudMaxShells = 125;

// ---------------------------------------------------------------------------
// Primary: bouncing buckshot (section 3 and 4)
// ---------------------------------------------------------------------------
constexpr float ReferenceCone = 0.08716f; // Original single-player double shot.
inline float SpreadMultiplier = 4.f; // Wide circular cone, about 38 degrees across.
inline float Cone = ReferenceCone * SpreadMultiplier;
inline float PelletDamageMultiplier = 1.875f;
inline int Pellets = 12;           // CShotgun::SecondaryAttack, single player.
inline float PelletSpeed = 1000.f; // Balance starting point (same as a disc).
inline int MaxSurfaceContacts = 3; // 3rd contact removes; two bounces at most.
inline float PelletLifetime = 4.f;
constexpr int MaxPellets = 96;        // This weapon only; Ricochet keeps its 24.
constexpr int MaxStepsPerFrame = 8;   // Corner/overlap guard, per traced step.
inline float PrimaryCooldown = .75f; // Native shotgun primary cadence.
inline int PrimaryAmmoCost = 1;
constexpr float PelletScale = .35f;   // Reuses the Ricochet disc, much smaller.
constexpr float PelletRadius = 1.5f;  // Presentation only; motion is continuous.
constexpr float PelletMuzzleOffset = 6.f;
constexpr float SurfaceOffset = 1.f;  // Push out of the surface after a bounce.

// ---------------------------------------------------------------------------
// Flak Cannon uses the native shared buckshot reserve, without a magazine.
// Both attacks consume this same shell pool, with costs of 1 and 2.
// ---------------------------------------------------------------------------
constexpr int StartShells = 40;
inline int SecondaryAmmoCost = 2;
inline float DeployTime = .6f;

// ---------------------------------------------------------------------------
// Secondary: contact grenade whose damage follows the aim elevation (section 5)
// ---------------------------------------------------------------------------
inline float GrenadeSpeed = 450.f;    // CMP5::SecondaryAttack.
inline float GrenadeGravity = .8f;    // CGrenade::ShootContact.
inline float GrenadeLifetime = 6.f;   // Safety fuse for a grenade in the void.
inline float SecondaryCooldown = 1.f;
constexpr int MaxGrenades = 8;           // Concurrent grenades for this weapon.
constexpr float ExplosionSurfaceOffset = 8.f; // Pull the blast out of the wall.
constexpr float GrenadeMuzzleOffset = 16.f;
inline float ElevationMinFraction = .5f; // Dmin = 25% of the M203 damage.
constexpr float M203RadiusFactor = 2.5f;     // RadiusDamage: damage * 2.5.
inline float GrenadePushHorizontal = 140.f;
inline float GrenadePushVertical = 35.f;
inline float GrenadePushDuration = .5f;
constexpr float ExplosionScaleMin = 5.f;    // TE_EXPLOSION scale * 10.
constexpr float ExplosionScaleMax = 9.f;

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

#pragma once
namespace balance {
inline float GuidanceTurnRate = 6.f;
inline int MaxCharges = 3;
constexpr int HudAmmoCharges = 22;
inline float RechargeTime = 1.5f;
inline float DiscSpeed = 750.f;       // Valve Ricochet DISC_VELOCITY.
inline float SlowDuration = 7.f;       // Valve Ricochet FREEZE_TIME.
inline float SlowFactor = .25f;       // Adapted for Ravenholm monster locomotion.
inline float PrimaryCooldown = .5f;
inline float SecondaryCooldown = 1.25f;
inline float TripleSpread = 7.f;       // Valve Ricochet FireDisc: yaw +/-7 degrees.
inline float BlastRadius = 210.f;
inline float BlastDamage = 4.f;        // Per disc: at most 12 from the entire volley.
inline float BlastPush = 850.f;
inline float BlastLift = 190.f;
inline float PushDuration = .8f;
inline float MaxHorizontalPush = 1200.f;
inline float DiscLifetime = 6.f;
inline int MaxProjectiles = 24;
}

#pragma once
namespace balance {
constexpr float DiscSpeed = 1000.f;       // Valve Ricochet DISC_VELOCITY.
constexpr float SlowDuration = 7.f;       // Valve Ricochet FREEZE_TIME.
constexpr float SlowFactor = .25f;       // Adapted for Ravenholm monster locomotion.
constexpr float PrimaryCooldown = .5f;
constexpr float SecondaryCooldown = 1.25f;
constexpr float TripleSpread = 7.f;       // Valve Ricochet FireDisc: yaw +/-7 degrees.
constexpr float BlastRadius = 210.f;
constexpr float BlastDamage = 4.f;        // Per disc: at most 12 from the entire volley.
constexpr float BlastPush = 850.f;
constexpr float BlastLift = 190.f;
constexpr float PushDuration = .8f;
constexpr float MaxHorizontalPush = 1200.f;
constexpr float DiscLifetime = 6.f;
constexpr int MaxProjectiles = 24;
}

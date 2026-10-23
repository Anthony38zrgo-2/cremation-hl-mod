#pragma once

// weapon_bounce_shotgun: an independent shotgun with bouncing pellets and an
// elevation-scaled contact grenade.
//
// The module owns its projectiles, its magazine and its damage rules. It knows
// nothing about Ricochet, Ravenholm private classes or entity layouts: every
// host service it needs is a function pointer below, so the same translation
// unit can be exercised by the simulated engine in test_bounce.cpp.
#include "extdll.h"

namespace bounce {

extern const char* const WeaponName;  // "weapon_bounce_shotgun"
extern const char* const CarrierName; // "weapon_crossbow"
extern const char* const ViewModel;   // models/cremation/v_bounce_shotgun.mdl
extern const char* const PlayerModel; // models/cremation/p_bounce_shotgun.mdl
extern const char* const WorldModel;  // models/cremation/w_bounce_shotgun.mdl

struct Host {
    enginefuncs_t* engine = nullptr;
    globalvars_t* globals = nullptr;
    int (*spawn)(edict_t*) = nullptr;                    // Ravenholm pfnSpawn
    void (*sound)(edict_t*, const char*) = nullptr;
    void (*animation)(edict_t*, int sequence) = nullptr; // SVC_WEAPONANIM
    void (*push)(edict_t*, const Vector& direction, float horizontal, float vertical,
                 float duration) = nullptr;              // shared small impulse
};

// Wiring. attach() is called once the engine/DLL pointers are known; every
// other entry point is a safe no-op until then.
void attach(const Host& host);
void setMessageIds(int curWeapon, int ammoX);
int carrierWeaponId();
void precache();
void reset();
void frame(float dt);

// Inventory, HUD and lifecycle glue used by the proxy hooks.
void cmdStart(edict_t* player, unsigned int seed);
void playerPostThink(edict_t* player, int buttons);
bool isBouncePlayer(edict_t* player);
bool isCarrierClass(const char* classname);
void hudListBytes(int* out);
void weaponSpawned(edict_t* weapon);
void pickedUp(edict_t* player);
void sendWeaponState(edict_t* player);
int remapAnimation(int sequence);
bool removeRestored(edict_t* entity);

// Gameplay entry points, also used by the automated tests.
bool fire(edict_t* player, bool secondary);

struct Status {
    int activePellets = 0;
    int activeGrenades = 0;
    int clip = 0;
    int shells = 0;
    int grenades = 0;
    float lastGrenadeDamage = 0.f;   // Dshot of the most recent grenade.
    float lastGrenadeRadius = 0.f;
    float lastGrenadeElevation = 0.f;
};

Status status(edict_t* player);

struct PelletInfo {
    Vector origin{0, 0, 0};
    Vector velocity{0, 0, 0};
    int contacts = 0;
};

int pellets(PelletInfo* out, int max);

struct GrenadeInfo {
    Vector origin{0, 0, 0};
    Vector velocity{0, 0, 0};
    float damage = 0.f;
};

int grenades(GrenadeInfo* out, int max);

} // namespace bounce

// weapon_bounce_shotgun behaviour. See bounce_shotgun.h for the host interface.
#include "extdll.h"
#undef min
#undef max
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "bounce_shotgun_balance.h"
#include "bounce_shotgun.h"

namespace bounce {

using namespace bounce_balance;

const char* const WeaponName = "weapon_bounce_shotgun";
const char* const CarrierName = "weapon_crossbow";
const char* const CarrierViewModel = "models/v_crossbow.mdl";
const char* const ViewModel = "models/cremation/v_bounce_shotgun.mdl";
const char* const PlayerModel = "models/cremation/p_bounce_shotgun.mdl";
const char* const WorldModel = "models/cremation/w_bounce_shotgun.mdl";

namespace {

constexpr int ButtonAttack = 1;
constexpr int ButtonAttack2 = 1 << 11;
// const.h keeps the DmgType bits in the game DLL headers; these are the two the
// pellet and grenade paths need (DMG_BULLET and DMG_BLAST).
constexpr int DamageBullet = 1 << 1;
constexpr int DamageBlast = 1 << 6;
constexpr int SvcTempEntity = 23; // SVC_TEMPENTITY (not exported by the 140 API headers)
constexpr float Pi = 3.14159265358979323846f;

Host host{};
enginefuncs_t* eng = nullptr;
globalvars_t* globals = nullptr;
int msgCurWeapon = 0, msgAmmoX = 0;
int fireballSprite = 0, smokeSprite = 0, trailSprite = 0;
int viewModelString = -1, playerModelString = -1;

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------
const char* text(string_t value) { return eng->pfnSzFromIndex(value); }
bool valid(edict_t* e) { return e && !e->free && !(e->v.flags & FL_KILLME); }
float length(const Vector& a) { return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z); }
Vector normal(const Vector& a) {
    float n = length(a);
    return n > .0001f ? a * (1.f / n) : Vector(0, 0, 0);
}
float dot(const Vector& a, const Vector& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vector center(edict_t* e) { return (e->v.absmin + e->v.absmax) * .5f; }
bool aliveEnemy(edict_t* e) {
    return valid(e) && (e->v.flags & FL_MONSTER) && e->v.deadflag == DEAD_NO && e->v.health > 0;
}
bool ready() { return eng && globals; }
int playerIndex(edict_t* e) {
    if (!ready() || !e) return -1;
    int i = eng->pfnIndexOfEdict(e);
    return (i > 0 && i < 33) ? i : -1;
}

// Engine-compatible AngleVectors (roll is ignored: players never roll).
void angleVectors(const Vector& angles, Vector& forward, Vector& right, Vector& up) {
    float sy = std::sin(angles.y * (Pi * 2.f / 360.f)), cy = std::cos(angles.y * (Pi * 2.f / 360.f));
    float sp = std::sin(angles.x * (Pi * 2.f / 360.f)), cp = std::cos(angles.x * (Pi * 2.f / 360.f));
    forward = Vector(cp * cy, cp * sy, -sp);
    right = Vector(sy, -cy, 0);
    up = Vector(sp * cy, sp * sy, cp);
}
float vecToYaw(const Vector& dir) { return std::atan2(dir.y, dir.x) * 180.f / Pi; }

// ---------------------------------------------------------------------------
// Skill configuration, read from the live cvars (never hardcoded)
// ---------------------------------------------------------------------------
int skillLevel() {
    cvar_t* cv = eng->pfnCVarGetPointer ? eng->pfnCVarGetPointer("skill") : nullptr;
    int level = cv ? int(cv->value) : 1;
    return level < 0 ? 0 : (level > 2 ? 2 : level);
}
float skillValue(const char* base, float fallback) {
    char name[64];
    std::snprintf(name, sizeof name, "%s%d", base, skillLevel() + 1);
    cvar_t* cv = eng->pfnCVarGetPointer ? eng->pfnCVarGetPointer(name) : nullptr;
    return cv ? cv->value : fallback;
}

// ---------------------------------------------------------------------------
// UTIL_SharedRandomFloat, ported verbatim from hlsdk/dlls/util.cpp so the
// pellet offsets use the donor's generator and its seed order.
// ---------------------------------------------------------------------------
unsigned int seedState = 0;
const unsigned int seedTable[256] = {
    28985, 27138, 26457, 9451, 17764, 10909, 28790, 8716, 6361, 4853, 17798, 21977, 19643, 20662, 10834, 20103,
    27067, 28634, 18623, 25849, 8576, 26234, 23887, 18228, 32587, 4836, 3306, 1811, 3035, 24559, 18399, 315,
    26766, 907, 24102, 12370, 9674, 2972, 10472, 16492, 22683, 11529, 27968, 30406, 13213, 2319, 23620, 16823,
    10013, 23772, 21567, 1251, 19579, 20313, 18241, 30130, 8402, 20807, 27354, 7169, 21211, 17293, 5410, 19223,
    10255, 22480, 27388, 9946, 15628, 24389, 17308, 2370, 9530, 31683, 25927, 23567, 11694, 26397, 32602, 15031,
    18255, 17582, 1422, 28835, 23607, 12597, 20602, 10138, 5212, 1252, 10074, 23166, 19823, 31667, 5902, 24630,
    18948, 14330, 14950, 8939, 23540, 21311, 22428, 22391, 3583, 29004, 30498, 18714, 4278, 2437, 22430, 3439,
    28313, 23161, 25396, 13471, 19324, 15287, 2563, 18901, 13103, 16867, 9714, 14322, 15197, 26889, 19372, 26241,
    31925, 14640, 11497, 8941, 10056, 6451, 28656, 10737, 13874, 17356, 8281, 25937, 1661, 4850, 7448, 12744,
    21826, 5477, 10167, 16705, 26897, 8839, 30947, 27978, 27283, 24685, 32298, 3525, 12398, 28726, 9475, 10208,
    617, 13467, 22287, 2376, 6097, 26312, 2974, 9114, 21787, 28010, 4725, 15387, 3274, 10762, 31695, 17320,
    18324, 12441, 16801, 27376, 22464, 7500, 5666, 18144, 15314, 31914, 31627, 6495, 5226, 31203, 2331, 4668,
    12650, 18275, 351, 7268, 31319, 30119, 7600, 2905, 13826, 11343, 13053, 15583, 30055, 31093, 5067, 761,
    9685, 11070, 21369, 27155, 3663, 26542, 20169, 12161, 15411, 30401, 7580, 31784, 8985, 29367, 20989, 14203,
    29694, 21167, 10337, 1706, 28578, 887, 3373, 19477, 14382, 675, 7033, 15111, 26138, 12252, 30996, 21409,
    25678, 18555, 13256, 23316, 22407, 16727, 991, 9236, 5373, 29402, 6117, 15241, 27715, 19291, 19888, 19847};

unsigned int uRandom() {
    seedState *= 69069;
    seedState += seedTable[seedState & 0xff];
    return ++seedState & 0x0fffffff;
}
void uSrand(unsigned int seed) { seedState = seedTable[seed & 0xff]; }

float sharedRandomFloat(unsigned int seed, float low, float high) {
    int lowBits = 0, highBits = 0;
    std::memcpy(&lowBits, &low, sizeof lowBits);
    std::memcpy(&highBits, &high, sizeof highBits);
    uSrand((int)seed + lowBits + highBits);
    uRandom();
    uRandom();
    unsigned int range = (unsigned int)(high - low); // Implicit float->uint, as in util.cpp.
    if (!range) return low;
    int tensixrand = (int)(uRandom() & 65535);
    float offset = (float)tensixrand / 65536.0f;
    return low + offset * (float)range;
}

// ---------------------------------------------------------------------------
// Damage helpers. CBaseEntity::TraceAttack (slot 10) only accumulates into the
// DLL's multi-damage list, which this proxy cannot flush, so the hitgroup
// multiplier is applied here and TakeDamage (slot 11) is called directly.
// ---------------------------------------------------------------------------
using TakeDamageFn = int(__thiscall*)(void*, entvars_t*, entvars_t*, float, int);
void takeDamage(edict_t* victim, edict_t* inflictor, edict_t* attacker, float amount, int bits) {
    if (!valid(victim) || !valid(inflictor) || !victim->pvPrivateData) return;
    if (!victim->v.takedamage || amount <= 0.f) return;
    auto table = *reinterpret_cast<void***>(victim->pvPrivateData);
    auto fn = reinterpret_cast<TakeDamageFn>(table[11]);
    fn(victim->pvPrivateData, &inflictor->v, &attacker->v, amount, bits);
}
float hitgroupScale(int hitgroup) {
    switch (hitgroup) {
    case 1: return skillValue(MonsterHeadSkill, FallbackMonsterHead);
    case 2: return skillValue(MonsterChestSkill, 1.f);
    case 3: return skillValue(MonsterStomachSkill, 1.f);
    case 4:
    case 5: return skillValue(MonsterArmSkill, 1.f);
    case 6:
    case 7: return skillValue(MonsterLegSkill, 1.f);
    default: return 1.f;
    }
}

// ---------------------------------------------------------------------------
// Entities
// ---------------------------------------------------------------------------
edict_t* createRuntime(const char* classname, edict_t* owner) {
    if (!host.spawn || !eng->pfnCreateNamedEntity) return nullptr;
    edict_t* e = eng->pfnCreateNamedEntity(eng->pfnAllocString("info_target"));
    if (!e) return nullptr;
    host.spawn(e);
    e->v.classname = eng->pfnAllocString(classname);
    e->v.movetype = MOVETYPE_NONE;
    e->v.solid = SOLID_NOT;
    e->v.takedamage = 0;
    e->v.health = 0;
    e->v.owner = owner;
    return e;
}
void destroy(edict_t*& e, int serial) {
    if (valid(e) && e->serialnumber == serial) eng->pfnRemoveEntity(e);
    e = nullptr;
}
bool passThrough(edict_t* e) {
    // The world (null entity) is always a real surface; only entities that are
    // gone, non-solid or triggers are ignored by projectiles.
    if (!e) return false;
    if (e->free || (e->v.flags & FL_KILLME)) return true;
    return e->v.solid == SOLID_TRIGGER || e->v.solid == SOLID_NOT;
}
bool isWorld(edict_t* e) { return !e || eng->pfnIndexOfEdict(e) == 0; }

// ---------------------------------------------------------------------------
// Pellets
// ---------------------------------------------------------------------------
struct Pellet {
    edict_t* entity = nullptr;
    int serial = 0;
    edict_t* owner = nullptr;
    int ownerSerial = 0;
    Vector velocity{0, 0, 0};
    float born = 0;
    float expires = 0;
    float damage = 0;
    int contacts = 0;
    bool consumed = false;
};
std::vector<Pellet> pelletsStore;

void damagePelletTarget(Pellet& pellet, edict_t* victim, const TraceResult& tr) {
    float amount = pellet.damage;
    if (aliveEnemy(victim)) amount *= hitgroupScale(tr.iHitgroup);
    takeDamage(victim, pellet.entity, pellet.owner, amount, DamageBullet);
    pellet.consumed = true;
}

void advancePellets(float dt) {
    for (auto& pellet : pelletsStore) {
        if (!valid(pellet.entity) || pellet.entity->serialnumber != pellet.serial) {
            pellet.entity = nullptr;
            continue;
        }
        if (!valid(pellet.owner) || pellet.owner->serialnumber != pellet.ownerSerial ||
            pellet.owner->v.health <= 0) {
            destroy(pellet.entity, pellet.serial);
            continue;
        }
        if (globals->time >= pellet.expires) {
            destroy(pellet.entity, pellet.serial);
            continue;
        }
        float remaining = dt;
        int steps = 0;
        bool exhausted = false;
        while (pellet.entity && remaining > 1e-5f) {
            if (steps++ >= MaxStepsPerFrame) {
                exhausted = true;
                break;
            }
            Vector from = pellet.entity->v.origin;
            Vector to = from + pellet.velocity * remaining;
            TraceResult tr{};
            eng->pfnTraceLine(&from.x, &to.x, 0, pellet.owner, &tr);
            if (tr.fStartSolid || tr.fAllSolid) {
                // Wedged inside geometry: never risk crossing a solid.
                destroy(pellet.entity, pellet.serial);
                break;
            }
            if (tr.flFraction >= 1.f) {
                eng->pfnSetOrigin(pellet.entity, &to.x);
                break;
            }
            Vector at(tr.vecEndPos);
            edict_t* hit = tr.pHit;
            float travelled = tr.flFraction;
            if (passThrough(hit)) {
                Vector dir = normal(pellet.velocity);
                Vector past = at + dir * 2.f;
                eng->pfnSetOrigin(pellet.entity, &past.x);
                remaining *= 1.f - travelled;
                continue;
            }
            if (aliveEnemy(hit) || (valid(hit) && hit->v.takedamage)) {
                damagePelletTarget(pellet, hit, tr);
                destroy(pellet.entity, pellet.serial);
                break;
            }
            // Solid surface: reflect on the first two contacts, vanish on the third.
            ++pellet.contacts;
            if (pellet.contacts >= MaxSurfaceContacts) {
                destroy(pellet.entity, pellet.serial);
                break;
            }
            Vector plane(tr.vecPlaneNormal);
            if (length(plane) < .1f) {
                destroy(pellet.entity, pellet.serial);
                break;
            }
            plane = normal(plane);
            pellet.velocity = pellet.velocity - plane * (2.f * dot(pellet.velocity, plane));
            Vector dir = normal(pellet.velocity);
            Vector exit = at + plane * SurfaceOffset + dir * .5f;
            eng->pfnSetOrigin(pellet.entity, &exit.x);
            remaining *= 1.f - travelled;
            if (host.sound) host.sound(pellet.entity, "ricochet/bounce.wav");
        }
        if (pellet.entity && exhausted) destroy(pellet.entity, pellet.serial);
    }
    pelletsStore.erase(std::remove_if(pelletsStore.begin(), pelletsStore.end(),
                                      [](const Pellet& p) { return !p.entity; }),
                       pelletsStore.end());
}

bool hasEntityBudget(int count, int active) {
    if (active + count > MaxPellets) return false;
    if (eng->pfnNumberOfEntities && globals->maxEntities > 0) {
        int freeEntities = globals->maxEntities - eng->pfnNumberOfEntities();
        if (freeEntities < count + 24) return false;
    }
    return true;
}

bool spawnPellet(edict_t* owner, const Vector& origin, const Vector& direction, float damage) {
    edict_t* e = createRuntime("cremation_pellet", owner);
    if (!e) return false;
    e->v.angles = Vector(0, vecToYaw(direction), 0);
    e->v.scale = PelletScale;
    e->v.rendermode = kRenderNormal;
    e->v.renderfx = kRenderFxGlowShell;
    e->v.renderamt = 10;
    e->v.rendercolor = Vector(255, 190, 80);
    eng->pfnSetModel(e, "models/ricochet/disc.mdl");
    Vector mins(-PelletRadius, -PelletRadius, -PelletRadius);
    Vector maxs(PelletRadius, PelletRadius, PelletRadius);
    if (eng->pfnSetSize) eng->pfnSetSize(e, &mins.x, &maxs.x);
    eng->pfnSetOrigin(e, &origin.x);
    Pellet pellet;
    pellet.entity = e;
    pellet.serial = e->serialnumber;
    pellet.owner = owner;
    pellet.ownerSerial = owner->serialnumber;
    pellet.velocity = direction * PelletSpeed;
    pellet.born = globals->time;
    pellet.expires = globals->time + PelletLifetime;
    pellet.damage = damage;
    pelletsStore.push_back(pellet);
    if (trailSprite && eng->pfnMessageBegin) {
        eng->pfnMessageBegin(MSG_BROADCAST, SvcTempEntity, nullptr, nullptr);
        eng->pfnWriteByte(TE_BEAMFOLLOW);
        eng->pfnWriteShort(eng->pfnIndexOfEdict(e));
        eng->pfnWriteShort(trailSprite);
        eng->pfnWriteByte(2);
        eng->pfnWriteByte(2);
        eng->pfnWriteByte(255);
        eng->pfnWriteByte(200);
        eng->pfnWriteByte(90);
        eng->pfnWriteByte(120);
        eng->pfnMessageEnd();
    }
    return true;
}

// ---------------------------------------------------------------------------
// Grenades
// ---------------------------------------------------------------------------
struct Grenade {
    edict_t* entity = nullptr;
    int serial = 0;
    edict_t* owner = nullptr;
    int ownerSerial = 0;
    Vector velocity{0, 0, 0};
    float born = 0;
    float expires = 0;
    float damage = 0;
    float radius = 0;
    float elevation = 0;
    int scale = ExplosionScaleMin;
};
std::vector<Grenade> grenadesStore;

void sendExplosionEffect(const Vector& at, int scale) {
    if (!eng->pfnMessageBegin) return;
    eng->pfnMessageBegin(MSG_PAS, SvcTempEntity, &at.x, nullptr);
    eng->pfnWriteByte(TE_EXPLOSION);
    eng->pfnWriteCoord(at.x);
    eng->pfnWriteCoord(at.y);
    eng->pfnWriteCoord(at.z);
    eng->pfnWriteShort(fireballSprite);
    eng->pfnWriteByte(scale);
    eng->pfnWriteByte(15);
    eng->pfnWriteByte(0); // TE_EXPLFLAG_NONE
    eng->pfnMessageEnd();
    eng->pfnMessageBegin(MSG_PAS, SvcTempEntity, &at.x, nullptr);
    eng->pfnWriteByte(TE_SMOKE);
    eng->pfnWriteCoord(at.x);
    eng->pfnWriteCoord(at.y);
    eng->pfnWriteCoord(at.z);
    eng->pfnWriteShort(smokeSprite);
    eng->pfnWriteByte(scale);
    eng->pfnWriteByte(12);
    eng->pfnMessageEnd();
}

// RadiusDamage from hlsdk/dlls/combat.cpp, with the same occlusion, water rules
// and falloff. TraceAttack would need the DLL multi-damage flush, so damage is
// applied with TakeDamage using the same adjusted amount.
void radiusDamage(edict_t* inflictor, edict_t* owner, const Vector& at, float damage,
                  float radius, float pushScale) {
    if (!eng->pfnFindEntityInSphere) return;
    float falloff = radius > 0.f ? damage / radius : 1.f;
    Vector source = at;
    source.z += 1.f;
    int contents = eng->pfnPointContents ? eng->pfnPointContents(&source.x) : 0;
    bool inWater = contents == CONTENTS_WATER;
    edict_t* entity = nullptr;
    int guard = 0;
    while (true) {
        entity = eng->pfnFindEntityInSphere(entity, &source.x, radius);
        if (!entity || guard++ > 2048) break;
        if (!valid(entity) || !entity->v.takedamage) continue;
        if (entity == inflictor || entity == owner) continue; // Cremation: no self damage.
        if (inWater && entity->v.waterlevel == 0) continue;
        if (!inWater && entity->v.waterlevel == 3) continue;
        Vector spot = center(entity);
        TraceResult tr{};
        eng->pfnTraceLine(&source.x, &spot.x, 0, inflictor, &tr);
        if (!(tr.flFraction >= 1.f || tr.pHit == entity)) continue;
        if (tr.fStartSolid) {
            tr.vecEndPos = source;
            tr.flFraction = 0.f;
        }
        Vector end(tr.vecEndPos);
        float distance = length(source - end);
        float adjusted = damage - distance * falloff;
        if (adjusted <= 0.f) continue;
        takeDamage(entity, inflictor, owner, adjusted, DamageBlast);
        if (host.push && aliveEnemy(entity)) {
            float k = 1.f - distance / radius;
            k = k < 0.f ? 0.f : (k > 1.f ? 1.f : k);
            Vector direction = center(entity) - source;
            host.push(entity, direction, GrenadePushHorizontal * k * pushScale,
                      GrenadePushVertical * k * pushScale, GrenadePushDuration);
        }
    }
}

void explodeGrenade(Grenade& grenade, const Vector& at) {
    if (!valid(grenade.entity)) {
        grenade.entity = nullptr;
        return;
    }
    sendExplosionEffect(at, grenade.scale);
    radiusDamage(grenade.entity, grenade.owner, at, grenade.damage, grenade.radius, 1.f);
    destroy(grenade.entity, grenade.serial);
}

void advanceGrenades(float dt) {
    cvar_t* gravityCvar = eng->pfnCVarGetPointer ? eng->pfnCVarGetPointer("sv_gravity") : nullptr;
    float gravity = gravityCvar ? gravityCvar->value : 800.f;
    if (gravity <= 0.f) gravity = 800.f;
    for (auto& grenade : grenadesStore) {
        if (!valid(grenade.entity) || grenade.entity->serialnumber != grenade.serial) {
            grenade.entity = nullptr;
            continue;
        }
        if (!valid(grenade.owner) || grenade.owner->serialnumber != grenade.ownerSerial) {
            destroy(grenade.entity, grenade.serial);
            continue;
        }
        if (globals->time >= grenade.expires) {
            explodeGrenade(grenade, grenade.entity->v.origin);
            continue;
        }
        grenade.velocity.z -= gravity * GrenadeGravity * dt;
        float remaining = dt;
        int steps = 0;
        bool exhausted = false;
        while (grenade.entity && remaining > 1e-5f) {
            if (steps++ >= MaxStepsPerFrame) {
                exhausted = true;
                break;
            }
            Vector from = grenade.entity->v.origin;
            Vector to = from + grenade.velocity * remaining;
            TraceResult tr{};
            eng->pfnTraceLine(&from.x, &to.x, 0, grenade.owner, &tr);
            if (tr.fStartSolid || tr.fAllSolid) {
                explodeGrenade(grenade, from);
                break;
            }
            if (tr.flFraction >= 1.f) {
                eng->pfnSetOrigin(grenade.entity, &to.x);
                break;
            }
            Vector at(tr.vecEndPos);
            if (passThrough(tr.pHit)) {
                Vector dir = normal(grenade.velocity);
                Vector past = at + dir * 2.f;
                eng->pfnSetOrigin(grenade.entity, &past.x);
                remaining *= 1.f - tr.flFraction;
                continue;
            }
            // CGrenade::Explode pulls the blast out of the wall before the
            // radius test, so the explosion never starts inside the surface.
            Vector plane(tr.vecPlaneNormal);
            if (length(plane) > .1f) at = at + normal(plane) * ExplosionSurfaceOffset;
            explodeGrenade(grenade, at);
            break;
        }
        if (grenade.entity && exhausted) explodeGrenade(grenade, grenade.entity->v.origin);
    }
    grenadesStore.erase(std::remove_if(grenadesStore.begin(), grenadesStore.end(),
                                       [](const Grenade& g) { return !g.entity; }),
                        grenadesStore.end());
}

// ---------------------------------------------------------------------------
// Player state
// ---------------------------------------------------------------------------
struct PlayerState {
    unsigned int seed = 0;
    int clip = ClipSize;
    int shells = StartShells;
    int grenadeCount = StartGrenades;
    float nextPrimary = 0;
    float nextSecondary = 0;
    float reloadEnd = 0;
    float animUntil = 0;
    bool reloading = false;
    bool owns = false;
    bool wasActive = false;
    int sentClip = -1;
    int sentShells = -1;
    int sentGrenades = -1;
    float lastGrenadeDamage = 0;
    float lastGrenadeRadius = 0;
    float lastGrenadeElevation = -1;
};
std::array<PlayerState, 33> players{};

void sendAmmo(edict_t* player, int index, int count) {
    if (!msgAmmoX || !eng->pfnMessageBegin) return;
    int value = count < 0 ? 0 : (count > 254 ? 254 : count);
    eng->pfnMessageBegin(MSG_ONE, msgAmmoX, nullptr, player);
    eng->pfnWriteByte(index);
    eng->pfnWriteByte(value);
    eng->pfnMessageEnd();
}

void sendState(edict_t* player) {
    if (!ready() || !valid(player)) return;
    int i = playerIndex(player);
    if (i < 0) return;
    PlayerState& state = players[i];
    if (msgCurWeapon && eng->pfnMessageBegin) {
        eng->pfnMessageBegin(MSG_ONE, msgCurWeapon, nullptr, player);
        eng->pfnWriteByte(1);                 // state: this is the active weapon
        eng->pfnWriteByte(CarrierWeaponId);   // carrier id keeps the weapon bit
        eng->pfnWriteByte(state.clip);
        eng->pfnMessageEnd();
    }
    sendAmmo(player, HudAmmoShells, state.shells);
    sendAmmo(player, HudAmmoGrenades, state.grenadeCount);
    state.sentClip = state.clip;
    state.sentShells = state.shells;
    state.sentGrenades = state.grenadeCount;
}

void overridePlayerModels(edict_t* player) {
    if (viewModelString < 0) {
        viewModelString = eng->pfnAllocString(ViewModel);
        playerModelString = eng->pfnAllocString(PlayerModel);
    }
    if (player->v.viewmodel != viewModelString) player->v.viewmodel = viewModelString;
    if (player->v.weaponmodel != playerModelString) player->v.weaponmodel = playerModelString;
}

void startReload(edict_t* player, PlayerState& state) {
    if (state.reloading || state.shells <= 0) return;
    state.reloading = true;
    state.reloadEnd = globals->time + ReloadTime;
    state.nextPrimary = std::max(state.nextPrimary, state.reloadEnd);
    host.animation(player, AnimStartReload);
    if (host.sound) host.sound(player, "weapons/reload1.wav");
    state.animUntil = globals->time + .5f;
}

bool firePellets(edict_t* player, PlayerState& state) {
    if (state.reloading) return false;
    if (state.clip < PrimaryAmmoCost) {
        if (state.shells > 0) {
            startReload(player, state);
        } else {
            // Dry magazine: same feedback as the donor's PlayEmptySound.
            if (host.sound) host.sound(player, "weapons/357_cock1.wav");
            state.nextPrimary = globals->time + .15f;
        }
        return false;
    }
    if (!hasEntityBudget(Pellets, int(pelletsStore.size()))) return false;
    float damage = skillValue(BuckshotSkill, FallbackBuckshot) * PelletDamageMultiplier;
    Vector forward, right, up;
    angleVectors(player->v.v_angle + player->v.punchangle, forward, right, up);
    Vector eye = player->v.origin + player->v.view_ofs;
    Vector muzzle = eye + forward * PelletMuzzleOffset;
    TraceResult startTrace{};
    eng->pfnTraceLine(&eye.x, &muzzle.x, 0, player, &startTrace);
    if (startTrace.flFraction < 1.f) muzzle = Vector(startTrace.vecEndPos) - forward;
    unsigned int seed = state.seed ? state.seed : (unsigned int)(globals->time * 1000.f);
    size_t firstPellet = pelletsStore.size();
    for (int i = 1; i <= Pellets; ++i) {
        float x = sharedRandomFloat(seed + i, -.5f, .5f) + sharedRandomFloat(seed + 1 + i, -.5f, .5f);
        float y = sharedRandomFloat(seed + 2 + i, -.5f, .5f) + sharedRandomFloat(seed + 3 + i, -.5f, .5f);
        Vector direction = normal(forward + right * (x * Cone) + up * (y * Cone));
        spawnPellet(player, muzzle, direction, damage);
    }
    // A volley is all or nothing: if the engine refused any entity, remove the
    // partial discharge and keep the ammunition.
    if (pelletsStore.size() - firstPellet < size_t(Pellets)) {
        for (size_t i = firstPellet; i < pelletsStore.size(); ++i)
            destroy(pelletsStore[i].entity, pelletsStore[i].serial);
        pelletsStore.erase(pelletsStore.begin() + std::ptrdiff_t(firstPellet), pelletsStore.end());
        return false;
    }
    state.clip -= PrimaryAmmoCost;
    state.nextPrimary = globals->time + PrimaryCooldown;
    state.nextSecondary = std::max(state.nextSecondary, state.nextPrimary);
    state.animUntil = globals->time + .9f;
    host.animation(player, AnimFire2);
    host.sound(player, "weapons/dbarrel1.wav");
    player->v.punchangle.x -= 2.f;
    player->v.effects = int(player->v.effects) | EF_MUZZLEFLASH;
    sendState(player);
    return true;
}

bool fireGrenade(edict_t* player, PlayerState& state) {
    if (state.grenadeCount <= 0) {
        if (host.sound) host.sound(player, "weapons/357_cock1.wav");
        state.nextSecondary = globals->time + .15f;
        return false;
    }
    if (int(grenadesStore.size()) >= MaxGrenades) return false;
    // Elevated aim is captured here, before this shot's recoil is applied.
    float maxDamage = skillValue(M203Skill, FallbackM203);
    float elevation = -(player->v.v_angle.x + player->v.punchangle.x);
    elevation = elevation < 0.f ? 0.f : (elevation > 90.f ? 90.f : elevation);
    float t = elevation / 90.f;
    float minimum = ElevationMinFraction * maxDamage;
    float damage = minimum + (maxDamage - minimum) * t;
    damage = damage < minimum ? minimum : (damage > maxDamage ? maxDamage : damage);
    float radius = M203RadiusFactor * maxDamage;
    int scale = int(ExplosionScaleMin + (ExplosionScaleMax - ExplosionScaleMin) * t + .5f);
    Vector forward, right, up;
    angleVectors(player->v.v_angle + player->v.punchangle, forward, right, up);
    Vector eye = player->v.origin + player->v.view_ofs;
    Vector origin = eye + forward * GrenadeMuzzleOffset;
    TraceResult tr{};
    eng->pfnTraceLine(&eye.x, &origin.x, 0, player, &tr);
    if (tr.flFraction < 1.f) origin = Vector(tr.vecEndPos) - forward * 2.f;
    edict_t* entity = createRuntime("cremation_grenade", player);
    if (!entity) return false;
    entity->v.angles = Vector(0, vecToYaw(forward), 0);
    entity->v.gravity = GrenadeGravity;
    entity->v.velocity = Vector(0, 0, 0);
    eng->pfnSetModel(entity, "models/grenade.mdl");
    Vector zero(0, 0, 0);
    if (eng->pfnSetSize) eng->pfnSetSize(entity, &zero.x, &zero.x);
    eng->pfnSetOrigin(entity, &origin.x);
    Grenade grenade;
    grenade.entity = entity;
    grenade.serial = entity->serialnumber;
    grenade.owner = player;
    grenade.ownerSerial = player->serialnumber;
    grenade.velocity = forward * GrenadeSpeed;
    grenade.born = globals->time;
    grenade.expires = globals->time + GrenadeLifetime;
    grenade.damage = damage;
    grenade.radius = radius;
    grenade.elevation = elevation;
    grenade.scale = scale;
    grenadesStore.push_back(grenade);
    state.grenadeCount -= SecondaryAmmoCost;
    state.nextSecondary = globals->time + SecondaryCooldown;
    state.nextPrimary = std::max(state.nextPrimary, state.nextSecondary);
    state.animUntil = globals->time + .5f;
    state.lastGrenadeDamage = damage;
    state.lastGrenadeRadius = radius;
    state.lastGrenadeElevation = elevation;
    host.animation(player, AnimFire);
    host.sound(player, "weapons/glauncher.wav");
    player->v.punchangle.x -= 1.f;
    sendState(player);
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------
void attach(const Host& incoming) {
    host = incoming;
    eng = incoming.engine;
    globals = incoming.globals;
    viewModelString = -1;
    playerModelString = -1;
}

void setMessageIds(int curWeapon, int ammoX) {
    msgCurWeapon = curWeapon;
    msgAmmoX = ammoX;
}

int carrierWeaponId() { return CarrierWeaponId; }

void sendWeaponState(edict_t* player) { sendState(player); }

void precache() {
    if (!eng) return;
    const char* models[] = {ViewModel, PlayerModel, WorldModel, "models/ricochet/disc.mdl",
                            "models/grenade.mdl"};
    for (const char* model : models) eng->pfnPrecacheModel(const_cast<char*>(model));
    fireballSprite = eng->pfnPrecacheModel(const_cast<char*>("sprites/explode1.spr"));
    smokeSprite = eng->pfnPrecacheModel(const_cast<char*>("sprites/steam1.spr"));
    trailSprite = eng->pfnPrecacheModel(const_cast<char*>("sprites/ricochet/lgtning.spr"));
    const char* sounds[] = {"weapons/dbarrel1.wav", "weapons/glauncher.wav", "weapons/reload1.wav",
                            "weapons/reload3.wav", "ricochet/bounce.wav"};
    for (const char* sound : sounds) eng->pfnPrecacheSound(const_cast<char*>(sound));
    viewModelString = eng->pfnAllocString(ViewModel);
    playerModelString = eng->pfnAllocString(PlayerModel);
}

void reset() {
    for (auto& pellet : pelletsStore) destroy(pellet.entity, pellet.serial);
    pelletsStore.clear();
    for (auto& grenade : grenadesStore) destroy(grenade.entity, grenade.serial);
    grenadesStore.clear();
    players.fill(PlayerState{});
}

void frame(float dt) {
    if (!ready() || dt <= 0.f) return;
    advancePellets(dt);
    advanceGrenades(dt);
    for (int i = 1; i < 33; ++i) {
        PlayerState& state = players[i];
        if (!state.reloading || globals->time < state.reloadEnd) continue;
        int need = ClipSize - state.clip;
        int take = need < state.shells ? need : state.shells;
        state.clip += take;
        state.shells -= take;
        state.reloading = false;
        edict_t* player = eng->pfnPEntityOfEntIndex(i);
        if (valid(player) && isBouncePlayer(player)) {
            state.animUntil = globals->time + .4f;
            host.animation(player, AnimReload);
            sendState(player);
        }
    }
}

void cmdStart(edict_t* player, unsigned int seed) {
    int i = playerIndex(player);
    if (i < 0) return;
    players[i].seed = seed;
}

bool isBouncePlayer(edict_t* player) {
    if (!ready() || !valid(player)) return false;
    const char* model = text(player->v.viewmodel);
    if (!model || !*model) return false;
    return !std::strcmp(model, ViewModel) || !std::strcmp(model, CarrierViewModel);
}

bool isCarrierClass(const char* classname) {
    return classname && (!std::strcmp(classname, CarrierName) || !std::strcmp(classname, WeaponName));
}

void hudListBytes(int* out) {
    out[0] = HudAmmoShells;
    out[1] = HudMaxShells;
    out[2] = HudAmmoGrenades;
    out[3] = HudMaxGrenades;
    out[4] = HudSlot;
    out[5] = HudSlotPos;
    out[6] = CarrierWeaponId;
    out[7] = 0;
}

void weaponSpawned(edict_t* weapon) {
    if (!ready() || !valid(weapon)) return;
    if (viewModelString < 0) {
        viewModelString = eng->pfnAllocString(ViewModel);
        playerModelString = eng->pfnAllocString(PlayerModel);
    }
    weapon->v.viewmodel = viewModelString;
    weapon->v.weaponmodel = playerModelString;
    eng->pfnSetModel(weapon, WorldModel);
    weapon->v.body = 0;
    weapon->v.rendermode = kRenderNormal;
    weapon->v.renderamt = 0;
    weapon->v.renderfx = kRenderFxNone;
}

void pickedUp(edict_t* player) {
    int i = playerIndex(player);
    if (i < 0) return;
    PlayerState& state = players[i];
    state.owns = true;
    // Only the ammo counters: CurWeapon would make the HUD believe this weapon
    // is already deployed.
    sendAmmo(player, HudAmmoShells, state.shells);
    sendAmmo(player, HudAmmoGrenades, state.grenadeCount);
    state.sentShells = state.shells;
    state.sentGrenades = state.grenadeCount;
}

int remapAnimation(int) { return AnimIdle; }

bool removeRestored(edict_t* entity) {
    if (!ready() || !entity) return false;
    const char* classname = text(entity->v.classname);
    if (!classname) return false;
    if (!std::strcmp(classname, "cremation_pellet") || !std::strcmp(classname, "cremation_grenade")) {
        entity->v.flags |= FL_KILLME;
        entity->v.effects |= EF_NODRAW;
        return true;
    }
    if (!std::strcmp(classname, WeaponName)) {
        weaponSpawned(entity);
        return false;
    }
    return false;
}

void playerPostThink(edict_t* player, int buttons) {
    if (!ready() || !valid(player)) return;
    int i = playerIndex(player);
    if (i < 0) return;
    PlayerState& state = players[i];
    bool active = isBouncePlayer(player);
    if (active) {
        if (!state.wasActive) {
            state.nextPrimary = std::max(state.nextPrimary, globals->time + DeployTime);
            state.nextSecondary = std::max(state.nextSecondary, globals->time + DeployTime);
            state.animUntil = globals->time + DeployTime;
            host.animation(player, AnimDraw);
            sendState(player);
        }
        overridePlayerModels(player);
        if (state.sentClip != state.clip || state.sentShells != state.shells ||
            state.sentGrenades != state.grenadeCount) {
            sendState(player);
        }
        bool attack = (buttons & ButtonAttack) != 0;
        bool attack2 = (buttons & ButtonAttack2) != 0;
        if (player->v.health > 0 && attack != attack2) {
            if (attack && globals->time >= state.nextPrimary)
                fire(player, false);
            else if (attack2 && globals->time >= state.nextSecondary)
                fire(player, true);
        }
        if (globals->time >= state.animUntil && player->v.weaponanim != AnimIdle)
            host.animation(player, AnimIdle);
    }
    state.wasActive = active;
}

bool fire(edict_t* player, bool secondary) {
    if (!ready() || !valid(player)) return false;
    int i = playerIndex(player);
    if (i < 0) return false;
    if (player->v.health <= 0) return false;
    PlayerState& state = players[i];
    if (secondary) {
        if (globals->time < state.nextSecondary) return false;
        return fireGrenade(player, state);
    }
    if (globals->time < state.nextPrimary) return false;
    return firePellets(player, state);
}

Status status(edict_t* player) {
    Status result;
    result.activePellets = int(pelletsStore.size());
    result.activeGrenades = int(grenadesStore.size());
    int i = playerIndex(player);
    if (i >= 0) {
        const PlayerState& state = players[i];
        result.clip = state.clip;
        result.shells = state.shells;
        result.grenades = state.grenadeCount;
        result.lastGrenadeDamage = state.lastGrenadeDamage;
        result.lastGrenadeRadius = state.lastGrenadeRadius;
        result.lastGrenadeElevation = state.lastGrenadeElevation;
    }
    return result;
}

int pellets(PelletInfo* out, int max) {
    int count = 0;
    for (const auto& pellet : pelletsStore) {
        if (count >= max) break;
        if (!valid(pellet.entity)) continue;
        out[count].origin = pellet.entity->v.origin;
        out[count].velocity = pellet.velocity;
        out[count].contacts = pellet.contacts;
        ++count;
    }
    return count;
}

int grenades(GrenadeInfo* out, int max) {
    int count = 0;
    for (const auto& grenade : grenadesStore) {
        if (count >= max) break;
        if (!valid(grenade.entity)) continue;
        out[count].origin = grenade.entity->v.origin;
        out[count].velocity = grenade.velocity;
        out[count].damage = grenade.damage;
        ++count;
    }
    return count;
}

} // namespace bounce

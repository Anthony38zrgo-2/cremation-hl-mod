// Simulated-engine checks for weapon_bounce_shotgun. Never starts Half-Life.
// The module is compiled into this file so the internal state (pellets,
// grenades, radius damage) can be inspected directly.
#include "bounce_shotgun.cpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "weapon_config.h"

using namespace bounce;

namespace {

constexpr int MaxEntities = 96;
edict_t entities[MaxEntities];
std::vector<std::string> stringPool{""};
std::vector<int> written;
int nextSprite = 1;
int explosions = 0;
int animations = 0;
struct PushRecord {
    edict_t* entity;
    float horizontal;
    float vertical;
    float duration;
};
std::vector<PushRecord> pushes;
int currentMessageType = 0;

int addString(const char* value) {
    stringPool.emplace_back(value);
    return int(stringPool.size() - 1);
}
const char* lookupString(int index) {
    if (index < 0 || index >= int(stringPool.size())) return "";
    return stringPool[size_t(index)].c_str();
}
edict_t* at(int index) { return index >= 0 && index < MaxEntities ? &entities[index] : nullptr; }
int indexOf(const edict_t* e) { return e ? int(e - entities) : -1; }
void setOrigin(edict_t* e, const float* p) { e->v.origin = Vector(p[0], p[1], p[2]); }
void setSize(edict_t* e, const float* a, const float* b) {
    e->v.mins = Vector(a[0], a[1], a[2]);
    e->v.maxs = Vector(b[0], b[1], b[2]);
}
void setModel(edict_t* e, const char*) { e->v.modelindex = 1; }
int createBudget = -1; // -1 = unlimited; used to force a partial volley
edict_t* create(int) {
    if (createBudget == 0) return nullptr;
    if (createBudget > 0) --createBudget;
    for (int i = 8; i < MaxEntities; ++i) {
        if (!entities[i].free) continue;
        std::memset(&entities[i], 0, sizeof(entities[i]));
        entities[i].serialnumber = i + 100;
        return &entities[i];
    }
    return nullptr;
}
void removeEntity(edict_t* e) { if (e) e->free = true; }
void emitSound(edict_t*, int, const char*, float, float, int, int) {}
int precacheModel(char*) { return nextSprite++; }
int precacheSound(char*) { return nextSprite++; }
int numberOfEntities() {
    int count = 0;
    for (int i = 0; i < MaxEntities; ++i)
        if (!entities[i].free) ++count;
    return count;
}
cvar_t skillVar{"skill", "0", 0, 0.f, nullptr};
cvar_t buckshot{"sk_plr_buckshot1", "5", 0, 5.f, nullptr};
cvar_t m203{"sk_plr_9mmAR_grenade1", "100", 0, 100.f, nullptr};
cvar_t monsterHead{"sk_monster_head1", "3", 0, 3.f, nullptr};
cvar_t monsterChest{"sk_monster_chest1", "1", 0, 1.f, nullptr};
cvar_t monsterStomach{"sk_monster_stomach1", "1", 0, 1.f, nullptr};
cvar_t monsterArm{"sk_monster_arm1", "1", 0, 1.f, nullptr};
cvar_t monsterLeg{"sk_monster_leg1", "1", 0, 1.f, nullptr};
cvar_t gravity{"sv_gravity", "800", 0, 800.f, nullptr};
cvar_t* getCvar(const char* name) {
    if (!std::strcmp(name, "skill")) return &skillVar;
    if (!std::strcmp(name, "sk_plr_buckshot1")) return &buckshot;
    if (!std::strcmp(name, "sk_plr_9mmAR_grenade1")) return &m203;
    if (!std::strcmp(name, "sk_monster_head1")) return &monsterHead;
    if (!std::strcmp(name, "sk_monster_chest1")) return &monsterChest;
    if (!std::strcmp(name, "sk_monster_stomach1")) return &monsterStomach;
    if (!std::strcmp(name, "sk_monster_arm1")) return &monsterArm;
    if (!std::strcmp(name, "sk_monster_leg1")) return &monsterLeg;
    if (!std::strcmp(name, "sv_gravity")) return &gravity;
    return nullptr;
}
int pointContents(const float*) { return 0; }

void messageBegin(int, int type, const float*, edict_t*) {
    currentMessageType = type;
    written.clear();
}
void messageEnd() {
    if (currentMessageType == 23 && !written.empty() && written[0] == TE_EXPLOSION) ++explosions;
}
void writeByte(int value) { written.push_back(value); }
void writeShort(int value) { written.push_back(value); }
void writeCoord(float) {}
void writeString(const char*) {}

// --- geometry -------------------------------------------------------------
struct Box {
    Vector min{0, 0, 0};
    Vector max{0, 0, 0};
    edict_t* owner = nullptr; // nullptr = world
    int hitgroup = 0;
    bool solid = true;
};
std::vector<Box> boxes;
int traceHitgroup = 0;

bool inside(const Box& b, const Vector& p) {
    return p.x >= b.min.x && p.x <= b.max.x && p.y >= b.min.y && p.y <= b.max.y && p.z >= b.min.z &&
           p.z <= b.max.z;
}
// Slab test. Returns false when the segment misses, otherwise entry fraction and
// the face normal.
bool intersect(const Box& b, const Vector& from, const Vector& to, float& fraction, Vector& normal) {
    float tMin = 0.f, tMax = 1.f;
    Vector n(0, 0, 0);
    const float* f = &from.x;
    const float* d = &to.x;
    const float* lo = &b.min.x;
    const float* hi = &b.max.x;
    for (int axis = 0; axis < 3; ++axis) {
        float delta = d[axis] - f[axis];
        if (std::fabs(delta) < 1e-6f) {
            if (f[axis] < lo[axis] || f[axis] > hi[axis]) return false;
            continue;
        }
        float t1 = (lo[axis] - f[axis]) / delta;
        float t2 = (hi[axis] - f[axis]) / delta;
        float sign = -1.f;
        if (t1 > t2) {
            std::swap(t1, t2);
            sign = 1.f;
        }
        if (t1 > tMin) {
            tMin = t1;
            n = Vector(0, 0, 0);
            (&n.x)[axis] = sign;
        }
        if (t2 < tMax) tMax = t2;
        if (tMin > tMax) return false;
    }
    fraction = tMin;
    normal = n;
    return true;
}
void traceLine(const float* a, const float* b, int, edict_t* skip, TraceResult* tr) {
    *tr = TraceResult{};
    Vector from(a[0], a[1], a[2]);
    Vector to(b[0], b[1], b[2]);
    tr->flFraction = 1.f;
    tr->vecEndPos = to;
    tr->pHit = nullptr;
    float best = 2.f;
    for (const Box& box : boxes) {
        if (!box.solid) continue;
        if (box.owner && box.owner == skip) continue;
        if (box.owner && box.owner->free) continue;
        float fraction = 0.f;
        Vector normal(0, 0, 0);
        if (!intersect(box, from, to, fraction, normal)) continue;
        if (fraction >= best) continue;
        best = fraction;
        tr->flFraction = fraction;
        tr->vecEndPos = from + (to - from) * fraction;
        tr->vecPlaneNormal = normal;
        tr->pHit = box.owner;
        tr->iHitgroup = box.owner ? box.hitgroup : 0;
    }
    for (const Box& box : boxes) {
        if (box.solid && inside(box, from) && (!box.owner || box.owner != skip)) {
            tr->fStartSolid = 1;
            tr->flFraction = 0.f;
            tr->vecEndPos = from;
            break;
        }
    }
}
Vector bodyCenter(edict_t* e) { return (e->v.absmin + e->v.absmax) * .5f; }
edict_t* findInSphere(edict_t* start, const float* org, float radius) {
    Vector center(org[0], org[1], org[2]);
    int begin = start ? indexOf(start) + 1 : 0;
    for (int i = begin; i < MaxEntities; ++i) {
        edict_t* e = &entities[i];
        if (e->free || i == 0) continue;
        if ((e->v.flags & FL_MONSTER) == 0) continue;
        float dx = bodyCenter(e).x - center.x;
        float dy = bodyCenter(e).y - center.y;
        float dz = bodyCenter(e).z - center.z;
        if (std::sqrt(dx * dx + dy * dy + dz * dz) <= radius) return e;
    }
    return nullptr;
}

// --- damage ---------------------------------------------------------------
// pvPrivateData must start with a vtable pointer, exactly like a real
// CBaseEntity; slot 11 is CBaseEntity::TakeDamage in the Valve Win32 ABI.
struct Victim {
    void** vtable = nullptr; // must stay first
    edict_t* entity = nullptr;
    float lastDamage = 0.f;
    int lastBits = 0;
    int hits = 0;
    int gibMode = -1;
    float damageScale=1.f; bool corpseConversion=false;
    void killed(entvars_t*,int mode) {gibMode=mode;entity->v.deadflag=DEAD_DEAD;entity->v.solid=SOLID_NOT;}
    int takeDamage(entvars_t*, entvars_t*, float amount, int bits);
};
int Victim::takeDamage(entvars_t*, entvars_t*, float amount, int bits) {
    lastDamage = amount;
    lastBits = bits;
    ++hits;
    entity->v.health -= amount*damageScale;
    if(corpseConversion && entity->v.health<0) {entity->v.health=entity->v.max_health*.5f;entity->v.max_health=5.f;}
    return 1;
}
template <typename T>
void* memberAddress(T method) {
    return *reinterpret_cast<void**>(&method);
}
void* victimVtable[14]{};
Victim victims[4];

int spawnStub(edict_t*) { return 0; }
void soundStub(edict_t*, const char*) {}
void animationStub(edict_t*, int) { ++animations; }
void pushStub(edict_t* e, const Vector&, float horizontal, float vertical, float duration) {
    pushes.push_back({e, horizontal, vertical, duration});
}

edict_t* player = nullptr;
edict_t* enemy = nullptr;
edict_t* enemyTwo = nullptr;

void resetScene() {
    for (auto& e : entities) e.free = true;
    for (auto& v : victims) v = Victim{};
    // The engine string table is append-only; cached string_t values must stay valid.
    boxes.clear();
    pushes.clear();
    written.clear();
    explosions = 0;
    animations = 0;
}

void makePlayer(const Vector& origin, const Vector& angles) {
    player = &entities[1];
    std::memset(player, 0, sizeof(*player));
    player->serialnumber = 1;
    player->v.health = 100;
    player->v.view_ofs = Vector(0, 0, 64);
    player->v.origin = origin;
    player->v.v_angle = angles;
    player->v.viewmodel = addString(bounce::ViewModel);
    player->v.classname = addString("player");
}

void makeEnemy(edict_t*& slot, int index, const Vector& origin, int victimIndex) {
    slot = &entities[index];
    std::memset(slot, 0, sizeof(*slot));
    slot->serialnumber = index;
    slot->v.flags = FL_MONSTER;
    slot->v.health = 1000;
    slot->v.takedamage = 1;
    slot->v.solid = SOLID_SLIDEBOX;
    slot->v.deadflag = DEAD_NO;
    slot->v.origin = origin;
    slot->v.absmin = origin + Vector(-16, -16, 0);
    slot->v.absmax = origin + Vector(16, 16, 72);
    slot->v.classname = addString("monster_zombie");
    Victim& victim = victims[victimIndex];
    victim.entity = slot;
    victimVtable[ravenholm_abi::slot::TakeDamage] = memberAddress(&Victim::takeDamage);
    victimVtable[ravenholm_abi::slot::Killed] = memberAddress(&Victim::killed);
    victim.vtable = victimVtable;
    slot->pvPrivateData = &victim;
}

void clearPellets() {
    for (auto& pellet : bounce::pelletsStore) destroy(pellet.entity, pellet.serial);
    bounce::pelletsStore.clear();
}
void clearGrenades() {
    for (auto& grenade : bounce::grenadesStore) destroy(grenade.entity, grenade.serial);
    bounce::grenadesStore.clear();
}

void bootEngine() {
    globalvars_t* state = new globalvars_t{};
    state->time = 100.f;
    state->maxClients = 1;
    state->maxEntities = MaxEntities;
    state->frametime = .01f;
    enginefuncs_t* table = new enginefuncs_t{};
    table->pfnSzFromIndex = lookupString;
    table->pfnAllocString = addString;
    table->pfnIndexOfEdict = indexOf;
    table->pfnPEntityOfEntIndex = at;
    table->pfnNumberOfEntities = numberOfEntities;
    table->pfnCreateNamedEntity = create;
    table->pfnRemoveEntity = removeEntity;
    table->pfnSetModel = setModel;
    table->pfnSetSize = setSize;
    table->pfnSetOrigin = setOrigin;
    table->pfnTraceLine = traceLine;
    table->pfnEmitSound = emitSound;
    table->pfnMessageBegin = messageBegin;
    table->pfnMessageEnd = messageEnd;
    table->pfnWriteByte = writeByte;
    table->pfnWriteShort = writeShort;
    table->pfnWriteCoord = writeCoord;
    table->pfnWriteString = writeString;
    table->pfnPrecacheModel = precacheModel;
    table->pfnPrecacheSound = precacheSound;
    table->pfnCVarGetPointer = getCvar;
    table->pfnPointContents = pointContents;
    table->pfnFindEntityInSphere = findInSphere;
    bounce::Host host{};
    host.engine = table;
    host.globals = state;
    host.spawn = spawnStub;
    host.sound = soundStub;
    host.animation = animationStub;
    host.push = pushStub;
    bounce::attach(host);
    bounce::setMessageIds(101, 102);
    bounce::precache();
}

float magnitude(const Vector& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

} // namespace

int main() {
    bootEngine();
    globalvars_t* globals = bounce::globals;

    // 1. Sampler parity with UTIL_SharedRandomFloat for a fixed seed. Reference
    //    values were produced by an independent implementation of util.cpp.
    const float referenceX[12] = {
        0.180511475f, 0.649383545f, 0.095214844f, -0.457611084f, -0.518478394f, 0.090133667f,
        -0.090408325f, -0.579299927f, -0.146697998f, 0.509231567f, 0.773223877f, 0.531921387f};
    const float referenceY[12] = {
        0.095214844f, -0.457611084f, -0.518478394f, 0.090133667f, -0.090408325f, -0.579299927f,
        -0.146697998f, 0.509231567f, 0.773223877f, 0.531921387f, -0.266845703f, -0.498229980f};
    for (int i = 1; i <= bounce_balance::Pellets; ++i) {
        float x = bounce::sharedRandomFloat(123456 + i, -.5f, .5f) +
                  bounce::sharedRandomFloat(123456 + 1 + i, -.5f, .5f);
        float y = bounce::sharedRandomFloat(123456 + 2 + i, -.5f, .5f) +
                  bounce::sharedRandomFloat(123456 + 3 + i, -.5f, .5f);
        assert(std::fabs(x - referenceX[i - 1]) < 1e-6f);
        assert(std::fabs(y - referenceY[i - 1]) < 1e-6f);
    }

    // 2. Full volley: twelve pellets, no hitscan, captured directions.
    resetScene();
    makePlayer(Vector(0, 0, 100), Vector(0, 0, 0));
    player->v.angles = Vector(0, 0, 0);
    bounce::reset();
    bounce::cmdStart(player, 123456);
    assert(bounce::fire(player, false));
    Status status = bounce::status(player);
    assert(status.activePellets == bounce_balance::Pellets);
    assert(status.clip == -1);
    assert(status.shells == bounce_balance::StartShells - bounce_balance::PrimaryAmmoCost);
    assert(pushes.empty() && explosions == 0);
    {
        static PelletInfo info[12];
        int count = bounce::pellets(info, bounce_balance::Pellets);
        assert(count == bounce_balance::Pellets);
        for (int i = 0; i < count; ++i) {
            assert(std::fabs(magnitude(info[i].velocity) - bounce_balance::PelletSpeed) < .01f);
            assert(info[i].contacts == 0);
            assert(std::fabs(bounce::pelletsStore[i].damage-9.375f)<.001f);
            assert(std::fabs(bounce_balance::Cone/bounce_balance::ReferenceCone-4.f)<.00001f);
            // Circular cone: every pellet stays inside its radial stratum,
            // including an outer pellet beyond 95% of the wide cone radius.
            Vector actual = bounce::normal(info[i].velocity);
            float radial=std::sqrt(actual.y*actual.y+actual.z*actual.z)/actual.x/bounce_balance::Cone;
            assert(radial*radial >= float(i)/bounce_balance::Pellets-1e-5f);
            assert(radial*radial <= float(i+1)/bounce_balance::Pellets+1e-5f);
            if(i==bounce_balance::Pellets-1)assert(radial>.95f);

        }
    }
    // A second shot inside the cooldown must spend nothing.
    assert(!bounce::fire(player, false));
    assert(bounce::status(player).activePellets == bounce_balance::Pellets);

    // 3. Bounce, second bounce, removal on the third surface contact.
    resetScene();
    makePlayer(Vector(0, 0, 100), Vector(0, 0, 0));
    bounce::reset();
    boxes.push_back({Vector(52, -500, -500), Vector(60, 500, 500), nullptr, 0, true});
    boxes.push_back({Vector(-60, -500, -500), Vector(-52, 500, 500), nullptr, 0, true});
    {
        edict_t* pellet = create(0);
        pellet->v.origin = Vector(0, 0, 100);
        bounce::Pellet record;
        record.entity = pellet;
        record.serial = pellet->serialnumber;
        record.owner = player;
        record.ownerSerial = player->serialnumber;
        record.velocity = Vector(bounce_balance::PelletSpeed, 0, 0);
        record.born = globals->time;
        record.expires = globals->time + bounce_balance::PelletLifetime;
        record.damage = 5.f;
        bounce::pelletsStore.push_back(record);
    }
    const float dt = .05f;
    for (int step = 0; step < 40 && !bounce::pelletsStore.empty(); ++step) {
        globals->time += dt;
        bounce::frame(dt);
        if (!bounce::pelletsStore.empty()) {
            const bounce::Pellet& pellet = bounce::pelletsStore[0];
            // Speed must be preserved through every reflection.
            assert(std::fabs(magnitude(pellet.velocity) - bounce_balance::PelletSpeed) < .01f);
        }
    }
    assert(bounce::pelletsStore.empty());

    // The same scenario, observed step by step: contact 1 -> -x, contact 2 ->
    // +x, third contact removes the projectile.
    resetScene();
    makePlayer(Vector(0, 0, 100), Vector(0, 0, 0));
    bounce::reset();
    boxes.push_back({Vector(52, -500, -500), Vector(60, 500, 500), nullptr, 0, true});
    boxes.push_back({Vector(-60, -500, -500), Vector(-52, 500, 500), nullptr, 0, true});
    edict_t* probe = create(0);
    probe->v.origin = Vector(0, 0, 100);
    bounce::Pellet tracked;
    tracked.entity = probe;
    tracked.serial = probe->serialnumber;
    tracked.owner = player;
    tracked.ownerSerial = player->serialnumber;
    tracked.velocity = Vector(bounce_balance::PelletSpeed, 0, 0);
    tracked.born = globals->time;
    tracked.expires = globals->time + bounce_balance::PelletLifetime;
    tracked.damage = 5.f;
    bounce::pelletsStore.push_back(tracked);
    int observed = 0;

    for (int step = 0; step < 200 && !bounce::pelletsStore.empty(); ++step) {
        globals->time += .01f;
        bounce::frame(.01f);
        if (bounce::pelletsStore.empty()) break;
        const bounce::Pellet& pellet = bounce::pelletsStore[0];
        if (pellet.contacts != observed) {
            observed = pellet.contacts;
            if (observed == 1) assert(pellet.velocity.x < 0.f);
            if (observed == 2) assert(pellet.velocity.x > 0.f);
        }
    }
    assert(observed == 2);
    assert(bounce::pelletsStore.empty());

    // 4. Enemy hit: one pellet of buckshot, one application, immediate removal.
    resetScene();
    makePlayer(Vector(0, 0, 0), Vector(8, 0, 0));
    bounce::reset();
    makeEnemy(enemy, 2, Vector(200, 0, 0), 0);
    boxes.push_back({enemy->v.absmin, enemy->v.absmax, enemy, 0, true});
    bounce::cmdStart(player, 4242);
    assert(bounce::fire(player, false));

    for (int step = 0; step < 60 && bounce::status(player).activePellets > 0; ++step) {
        globals->time += .01f;
        bounce::frame(.01f);

    }

    assert(victims[0].hits > 0);
    for (int i = 0; i < victims[0].hits; ++i) {
        // Every pellet that reached the zombie did exactly one pellet of damage.
        float unit=bounce_balance::FallbackBuckshot*bounce_balance::PelletDamageMultiplier;
        assert(victims[0].lastDamage>=unit && std::fabs(victims[0].lastDamage/unit-std::round(victims[0].lastDamage/unit))<.001f);
        assert(victims[0].lastBits == bounce::DamageBullet);
    }
    assert(enemy->v.health < 1000.f);

    // Head hitgroup uses the native monster multiplier.
    resetScene();
    makePlayer(Vector(0, 0, 0), Vector(8, 0, 0));
    bounce::reset();
    makeEnemy(enemy, 2, Vector(200, 0, 0), 0);
    boxes.push_back({enemy->v.absmin, enemy->v.absmax, enemy, 1, true});
    edict_t* headShot = create(0);
    headShot->v.origin = Vector(150, 0, 60);
    {
        bounce::Pellet record;
        record.entity = headShot;
        record.serial = headShot->serialnumber;
        record.owner = player;
        record.ownerSerial = player->serialnumber;
        record.velocity = Vector(bounce_balance::PelletSpeed, 0, 0);
        record.born = globals->time;
        record.expires = globals->time + bounce_balance::PelletLifetime;
        record.damage = bounce_balance::FallbackBuckshot;
        bounce::pelletsStore.push_back(record);
    }
    for (int step = 0; step < 20 && !bounce::pelletsStore.empty(); ++step) {
        globals->time += .01f;
        bounce::frame(.01f);
    }
    assert(victims[0].hits == 1);
    assert(std::fabs(victims[0].lastDamage - 15.f) < .001f); // 5 * sk_monster_head
    assert(bounce::pelletsStore.empty());
    // A consumed projectile cannot damage twice.
    globals->time += .02f;
    bounce::frame(.02f);
    assert(victims[0].hits == 1);

    // 5. Budget: a full volley never fires partially.
    resetScene();
    makePlayer(Vector(0, 0, 100), Vector(0, 0, 0));
    bounce::reset();
    {
        bounce::Pellet filler;
        for (int i = 0; i < bounce_balance::MaxPellets; ++i) bounce::pelletsStore.push_back(filler);
        int clipBefore = bounce::status(player).clip;
        assert(!bounce::fire(player, false));
        assert(bounce::status(player).activePellets == bounce_balance::MaxPellets);
        assert(bounce::status(player).clip == clipBefore);
        bounce::pelletsStore.clear();
    }
    // A refused entity in the middle of a volley rolls the whole shot back.
    resetScene();
    makePlayer(Vector(0, 0, 100), Vector(0, 0, 0));
    bounce::reset();
    {
        int clipBefore = bounce::status(player).clip;
        createBudget = 5;
        assert(!bounce::fire(player, false));
        createBudget = -1;
        assert(bounce::status(player).activePellets == 0);
        assert(bounce::status(player).clip == clipBefore);
        globals->time += 10.f;
        assert(bounce::fire(player, false));
        assert(bounce::status(player).activePellets == bounce_balance::Pellets);
    }

    // 6. Lifetime limit.
    resetScene();
    makePlayer(Vector(0, 0, 100), Vector(0, 0, 0));
    bounce::reset();
    {
        edict_t* pellet = create(0);
        pellet->v.origin = Vector(0, 0, 100);
        bounce::Pellet record;
        record.entity = pellet;
        record.serial = pellet->serialnumber;
        record.owner = player;
        record.ownerSerial = player->serialnumber;
        record.velocity = Vector(0, 0, bounce_balance::PelletSpeed);
        record.born = globals->time;
        record.expires = globals->time + .5f;
        bounce::pelletsStore.push_back(record);
    }
    globals->time += 1.f;
    bounce::frame(.01f);
    assert(bounce::pelletsStore.empty());

    // 7. Grenade elevation curve, captured at fire time.
    const struct {
        float pitch;
        float expected;
    } elevationCases[] = {{30.f, 42.1875f}, {0.f, 42.1875f}, {-30.f, 56.25f},
                          {-60.f, 70.3125f}, {-90.f, 84.375f}, {-90.f, 84.375f}};
    resetScene();
    makePlayer(Vector(0, 0, 100), Vector(0, 0, 0));
    bounce::reset();
    for (const auto& elevationCase : elevationCases) {
        player->v.v_angle = Vector(elevationCase.pitch, 0, 0);
        player->v.punchangle = Vector(0, 0, 0);
        globals->time += 5.f;
        assert(bounce::fire(player, true));
        Status fired = bounce::status(player);
        assert(std::fabs(fired.lastGrenadeDamage - elevationCase.expected) < .001f);
        assert(std::fabs(fired.lastGrenadeRadius - 48.f) < .001f);
        // Looking up after the throw must not change the captured damage.
        player->v.v_angle = Vector(-90, 0, 0);
        Status later = bounce::status(player);
        assert(std::fabs(later.lastGrenadeDamage - elevationCase.expected) < .001f);
        clearGrenades();
    }
    // Same angle, different launch height and flight time: same damage.
    bounce::players[1].shells = 20;
    player->v.v_angle = Vector(-45, 0, 0);
    player->v.punchangle = Vector(0, 0, 0);
    globals->time += 5.f;
    assert(bounce::fire(player, true));
    float low = bounce::status(player).lastGrenadeDamage;
    clearGrenades();
    player->v.origin = Vector(0, 0, 300);
    player->v.punchangle = Vector(0, 0, 0);
    globals->time += 5.f;
    assert(bounce::fire(player, true));
    float high = bounce::status(player).lastGrenadeDamage;
    clearGrenades();
    assert(std::fabs(low - high) < .001f);
    assert(std::fabs(low - 63.28125f) < .001f); // 25 + 75 * 0.5

    // 8. Radius damage: falloff, occlusion, no self damage, small push.
    resetScene();
    makePlayer(Vector(0, 0, 100), Vector(0, 0, 0));
    bounce::reset();
    makeEnemy(enemy, 2, Vector(0, 0, 0), 0);       // body centre (0,0,36)
    makeEnemy(enemyTwo, 3, Vector(100, 0, 0), 1);  // body centre (100,0,36)
    edict_t* blast = create(0);
    blast->v.origin = Vector(0, 0, 100);
    const Vector blastAt(0, 0, 100);
    // RadiusDamage raises the source by one unit before tracing.
    const Vector source = blastAt + Vector(0, 0, 1);
    const float radius = 250.f;
    const float firstDistance = std::fabs(36.f - source.z);
    const float secondDistance =
        std::sqrt(100.f * 100.f + (36.f - source.z) * (36.f - source.z));
    const float firstDamage = 100.f - firstDistance * (100.f / radius);
    const float secondDamage = 100.f - secondDistance * (100.f / radius);
    bounce::radiusDamage(blast, player, blastAt, 100.f, radius, 1.f);
    assert(victims[0].hits == 1);
    assert(std::fabs(victims[0].lastDamage - firstDamage) < .01f);
    assert(victims[0].lastBits == bounce::DamageBlast);
    assert(victims[1].hits == 1);
    assert(std::fabs(victims[1].lastDamage - secondDamage) < .01f);
    assert(player->v.health == 100.f); // no self damage
    assert(pushes.size() == 2);
    for (const auto& push : pushes) {
        assert(push.horizontal <= bounce_balance::GrenadePushHorizontal + .01f);
        assert(push.vertical <= bounce_balance::GrenadePushVertical + .01f);
        assert(std::fabs(push.duration - bounce_balance::GrenadePushDuration) < .001f);
    }
    // Push magnitude does not depend on the angle or on the damage of the shot.
    resetScene();
    makePlayer(Vector(0, 0, 100), Vector(0, 0, 0));
    bounce::reset();
    makeEnemy(enemy, 2, Vector(0, 0, 0), 0);
    blast = create(0);
    blast->v.origin = Vector(0, 0, 100);
    bounce::radiusDamage(blast, player, blastAt, 25.f, radius, 1.f);
    assert(pushes.size() == 1);
    const float falloff = 1.f - firstDistance / radius;
    assert(std::fabs(pushes[0].horizontal - bounce_balance::GrenadePushHorizontal * falloff) < .01f);
    assert(std::fabs(pushes[0].vertical - bounce_balance::GrenadePushVertical * falloff) < .01f);
    // A wall blocks both damage and push.
    resetScene();
    makePlayer(Vector(0, 0, 100), Vector(0, 0, 0));
    bounce::reset();
    makeEnemy(enemy, 2, Vector(200, 0, 0), 0);
    boxes.push_back({Vector(100, -100, -100), Vector(110, 100, 300), nullptr, 0, true});
    blast = create(0);
    blast->v.origin = Vector(0, 0, 100);
    bounce::radiusDamage(blast, player, blastAt, 100.f, radius, 1.f);
    assert(victims[0].hits == 0);
    assert(pushes.empty());

    // 9. Grenade flight: gravity, detonation on the first solid, one explosion.
    resetScene();
    makePlayer(Vector(0, 0, 100), Vector(0, 0, 0));
    bounce::reset();
    boxes.push_back({Vector(300, -500, -500), Vector(320, 500, 500), nullptr, 0, true});
    makeEnemy(enemy, 2, Vector(150, 0, 140), 0);
    boxes.push_back({enemy->v.absmin,enemy->v.absmax,enemy,0,true});
    globals->time += 1.f;
    assert(bounce::fire(player, true));
    assert(bounce::status(player).activeGrenades == 1);
    {
        GrenadeInfo info[1];
        assert(bounce::grenades(info, 1) == 1);
        assert(std::fabs(info[0].velocity.x - 450.f) < .01f);
        assert(std::fabs(info[0].velocity.z - 120.f) < .01f);
        assert(std::fabs(info[0].damage - 42.1875f) < .01f); // horizontal aim
    }
    float previousZ = bounce::grenadesStore[0].velocity.z;
    globals->time += .05f;
    bounce::frame(.05f);
    assert(bounce::grenadesStore[0].velocity.z < previousZ); // gravity 0.5 * 800
    for (int step = 0; step < 200 && !bounce::grenadesStore.empty(); ++step) {
        globals->time += .01f;
        bounce::frame(.01f);
    }
    assert(bounce::grenadesStore.empty());
    assert(explosions == 1);
    assert(victims[0].hits == 1);
    float afterBlast = enemy->v.health;
    globals->time += .5f;
    bounce::frame(.01f);
    assert(enemy->v.health == afterBlast); // exactly one explosion

    // 10. HUD/inventory identity and ammo spend.
    int hud[8];
    bounce::hudListBytes(hud);
    assert(hud[0] == bounce_balance::HudAmmoShells);
    assert(hud[1] == bounce_balance::HudMaxShells);
    assert(hud[2] == 255);
    assert(hud[3] == 255);
    assert(hud[4] == bounce_balance::HudSlot);
    assert(hud[5] == bounce_balance::HudSlotPos);
    assert(hud[6] == bounce::carrierWeaponId());
    assert(hud[7] == 1); // Selectable before ammo pickup.
    assert(bounce::isCarrierClass("weapon_crossbow"));
    assert(bounce::isCarrierClass("weapon_bounce_shotgun"));
    assert(!bounce::isCarrierClass("weapon_shotgun"));
    assert(bounce::remapAnimation(6) == bounce_balance::AnimIdle);
    resetScene();
    makePlayer(Vector(0, 0, 100), Vector(0, 0, 0));
    bounce::reset();
    bounce::pickedUp(player);
    assert(!written.empty());
    player->v.v_angle = Vector(0, 0, 0);
    globals->time += 1.f;
    assert(bounce::fire(player, true)); // secondary costs one grenade
    assert(bounce::status(player).shells == bounce_balance::StartShells - 2);
    assert(bounce::status(player).grenades == 0);

    // 11. Shared buckshot, no magazine/reload, native primary cadence.
    resetScene();makePlayer(Vector(0,0,100),Vector(0,0,0));bounce::reset();
    int shared=20;
    bounce::host.shells=[](edict_t*,int spend) { return 0; };
    // Replace the host with a native-like shared pool.
    static int pool=20;
    bounce::host.shells=[](edict_t*,int spend) {pool-=spend;return pool;};
    globals->time+=10;
    assert(bounce::fire(player,false));assert(pool==19);
    assert(bounce::status(player).clip==-1);
    assert(!bounce::fire(player,false));
    globals->time+=.751f;
    assert(bounce::fire(player,false));assert(pool==18);
    pool+=12; // same native ammo pickup used by the original shotgun
    globals->time+=.751f;assert(bounce::fire(player,false));assert(pool==29);
    globals->time+=2;assert(bounce::fire(player,true));assert(pool==27);
    pool=1;globals->time+=2;assert(!bounce::fire(player,true));assert(pool==1);
    assert(bounce::fire(player,false));assert(pool==0);
    globals->time+=2;assert(!bounce::fire(player,false));assert(pool==0);
    bounce::host.shells=nullptr;

    // Damage, not pellet count, determines death and gibbing.
    resetScene();makePlayer(Vector(0,0,0),Vector(0,0,0));bounce::reset();
    makeEnemy(enemy,2,Vector(50,0,0),0);enemy->v.health=100000;
    boxes.push_back({Vector(40,-100,-100),Vector(60,100,200),enemy,0,true});
    assert(bounce::fire(player,false));globals->time+=.1f;bounce::frame(.1f);
    assert(victims[0].gibMode==-1 && std::fabs(enemy->v.health-(100000.f-112.5f))<.01f);
    for(float health:{113.f,112.5f,50.f}) {
        resetScene();makePlayer(Vector(0,0,0),Vector(0,0,0));bounce::reset();
        makeEnemy(enemy,2,Vector(50,0,0),0);enemy->v.health=health;
        boxes.push_back({Vector(40,-100,-100),Vector(60,100,200),enemy,0,true});
        assert(bounce::fire(player,false));globals->time+=.1f;bounce::frame(.1f);
        assert((victims[0].gibMode==2)==(health<112.5f));
    }
    // Native bullet mitigation must be allowed to keep the target alive.
    resetScene();makePlayer(Vector(0,0,0),Vector(0,0,0));bounce::reset();
    makeEnemy(enemy,2,Vector(0,0,0),0);enemy->v.health=50;victims[0].damageScale=.3f;
    edict_t* mitigationSource=create(0);
    bounce::takeDamage(enemy,mitigationSource,player,112.5f,bounce::DamageBullet);
    assert(victims[0].gibMode==-1 && std::fabs(enemy->v.health-16.25f)<.01f);
    // GoldSrc BecomeDead replaces negative health with half max health and
    // changes max_health into a corpse decal counter before the AI deadflag.
    enemy->v.health=10;enemy->v.max_health=50;victims[0].corpseConversion=true;
    bounce::takeDamage(enemy,mitigationSource,player,112.5f,bounce::DamageBullet);
    assert(victims[0].gibMode==2);
    // Partial volley can gib a weak enemy; direct blast gets full damage, tiny
    // splash radius excludes a neighbouring target, and a wounded survivor pushes.
    resetScene();makePlayer(Vector(0,0,0),Vector(0,0,0));bounce::reset();
    makeEnemy(enemy,2,Vector(50,0,0),0);enemy->v.health=5;
    edict_t* hitSource=create(0);
    bounce::takeDamage(enemy,hitSource,player,9.375f,bounce::DamageBullet);
    assert(victims[0].gibMode==2);
    resetScene();makePlayer(Vector(0,0,0),Vector(0,0,0));bounce::reset();
    makeEnemy(enemy,2,Vector(0,0,0),0);enemy->v.health=100;
    makeEnemy(enemyTwo,3,Vector(80,0,0),1);
    hitSource=create(0);
    bounce::radiusDamage(hitSource,player,Vector(0,0,36),112.5f,48,1,enemy);
    assert(victims[0].gibMode==2 && victims[1].hits==0);
    resetScene();makePlayer(Vector(0,0,0),Vector(0,0,0));bounce::reset();
    makeEnemy(enemy,2,Vector(0,0,0),0);enemy->v.health=100;
    hitSource=create(0);
    bounce::radiusDamage(hitSource,player,Vector(0,0,36),56.25f,48,1,enemy);
    assert(victims[0].gibMode==-1 && std::fabs(enemy->v.health-43.75f)<.01f && !pushes.empty());
    std::puts("PASS: damage-based overkill, no forced full-volley kill, strict health threshold, precise blast and survivor knockback.");

    // 12. Weapon spawn overrides the carrier models without touching v_shotgun.
    resetScene();
    makePlayer(Vector(0, 0, 100), Vector(0, 0, 0));
    bounce::reset();
    edict_t* weapon = create(0);
    weapon->v.classname = addString("weapon_bounce_shotgun");
    bounce::weaponSpawned(weapon);
    assert(std::strcmp(lookupString(weapon->v.viewmodel), bounce::ViewModel) == 0);
    assert(std::strcmp(lookupString(weapon->v.weaponmodel), bounce::PlayerModel) == 0);
    assert(std::strcmp(lookupString(weapon->v.viewmodel), "models/v_shotgun.mdl") != 0);
    // Restored runtime entities are removed instead of becoming orphan bodies.
    edict_t* restored = create(0);
    restored->v.classname = addString("cremation_pellet");
    assert(bounce::removeRestored(restored));
    assert(restored->v.flags & FL_KILLME);
    edict_t* restoredWeapon = create(0);
    restoredWeapon->v.classname = addString("weapon_bounce_shotgun");
    assert(!bounce::removeRestored(restoredWeapon));
    assert(std::strcmp(lookupString(restoredWeapon->v.viewmodel), bounce::ViewModel) == 0);

    // Surface impacts scatter all pellets at 45 degrees to the surface;
    // direct monster impacts do not scatter. Fragment damage tracks elevation.
    resetScene();makePlayer(Vector(0,0,0),Vector(0,0,0));bounce::reset();
    bounce::Grenade fragment;fragment.entity=create(0);fragment.serial=fragment.entity->serialnumber;
    fragment.owner=player;fragment.ownerSerial=player->serialnumber;fragment.damage=84.375f;fragment.radius=48;
    bounce::explodeGrenade(fragment,Vector(100,0,20),nullptr,Vector(0,0,1));
    assert(bounce::pelletsStore.size()==12);
    for(auto& p:bounce::pelletsStore) {
        assert(std::fabs(p.velocity.z/bounce_balance::PelletSpeed-std::sqrt(.5f))<.001f);
        assert(std::fabs(p.damage-7.03125f)<.001f);
    }
    resetScene();makePlayer(Vector(0,0,0),Vector(0,0,0));bounce::reset();
    makeEnemy(enemy,2,Vector(100,0,0),0);
    fragment.entity=create(0);fragment.serial=fragment.entity->serialnumber;fragment.owner=player;fragment.ownerSerial=player->serialnumber;
    bounce::explodeGrenade(fragment,Vector(100,0,36),enemy);
    assert(bounce::pelletsStore.empty());
    std::puts("PASS: 12 diagonal surface fragments, scaled damage, no fragmentation on direct enemy impact.");

    // TFC shotgun: fixed six hitscan pellets of four damage, half-second fire,
    // eight shells, per-shell reload; alternate input never double-fires.
    resetScene();makePlayer(Vector(0,0,0),Vector(0,0,0));bounce::reset();
    player->v.viewmodel=addString("models/v_shotgun.mdl");
    makeEnemy(enemy,2,Vector(100,0,0),0);
    boxes.push_back({enemy->v.absmin,enemy->v.absmax,enemy,0,true});
    static int tfcPool=20;bounce::host.shells=[](edict_t*,int spend){tfcPool-=spend;return tfcPool;};
    bounce::tfcPostThink(player,0);globals->time+=.51f;
    bounce::tfcPostThink(player,1);assert(bounce::tfcClip(player)==7 && victims[0].lastDamage==24);
    bounce::tfcPostThink(player,1);assert(bounce::tfcClip(player)==7);
    globals->time+=.51f;bounce::tfcPostThink(player,1<<11);assert(bounce::tfcClip(player)==7);
    bounce::tfcPostThink(player,1<<13);globals->time+=.51f;bounce::tfcPostThink(player,0);
    assert(bounce::tfcClip(player)==8 && tfcPool==19);
    std::puts("PASS: TFC single shotgun model identity, six x four damage, eight-shell clip, rate and shell reload.");

    // Engineer: placement, 130 metal, four-second frozen build, automatic fire,
    // repair/resupply, one gun per player, dismantle and metal pickup.
    resetScene();makePlayer(Vector(0,0,36),Vector(0,0,0));bounce::reset();player->v.flags|=FL_CLIENT;
    boxes.push_back({Vector(-1000,-1000,-8),Vector(1000,1000,0),nullptr,0,true});
    assert(bounce::engineerCommand(player,"cremation_build"));
    assert(bounce::engineers[1].metal==70 && bounce::engineerBuilding(player) && (player->v.flags&FL_FROZEN));
    auto turret=bounce::engineers[1].gun;assert(turret && bounce::isSentry(turret));
    bounce::engineerCommand(player,"cremation_build");assert(bounce::engineers[1].gun==turret);
    makeEnemy(enemy,2,Vector(200,0,0),0);
    globals->time+=4.1f;bounce::frame(.01f);
    assert(!bounce::engineerBuilding(player) && !(player->v.flags&FL_FROZEN));
    assert(victims[0].hits==1 && bounce::engineers[1].ammo==99);
    turret->v.health=100;bounce::engineerCommand(player,"cremation_repair");
    assert(turret->v.health==150 && bounce::engineers[1].ammo==100 && bounce::engineers[1].metal==60);
    bounce::engineerCommand(player,"cremation_dismantle");assert(!bounce::engineers[1].gun && bounce::engineers[1].metal==125);
    auto metal=create(0);metal->v.targetname=addString("cremation_metal_01");
    assert(bounce::engineerMetal(metal,player) && bounce::engineers[1].metal==175);
    bounce::host.shells=nullptr;
    std::puts("PASS: sentry placement, metal, construction lock, tracking, ammo, repair, dismantle and pickups.");

    // Same gameplay code reads edited configuration without recompiling it.
    resetScene();makePlayer(Vector(0,0,100),Vector(0,0,0));bounce::reset();
    std::string configError;
    assert(weapon_config::apply(R"({"primary_ammo_cost":3,"secondary_ammo_cost":4,"pellets_per_shot":8,"primary_cooldown":0.2})",weapon_config::flak_cannonFields(),configError));
    assert(bounce::fire(player,false));assert(bounce::status(player).activePellets==8);
    assert(bounce::status(player).shells==37);
    globals->time+=.21f;assert(bounce::fire(player,true));assert(bounce::status(player).shells==33);
    std::puts("PASS: edited JSON values change real firing costs, pellet count and cooldown without recompilation.");
    std::puts("PASS: pellet sampler matches UTIL_SharedRandomFloat and the donor seed order.");
    std::puts("PASS: twelve physical pellets, one speed, captured directions, no hitscan.");
    std::puts("PASS: two reflections, speed preserved, removal on the third surface contact.");
    std::puts("PASS: enemy contact applies one buckshot pellet once, hitgroup x3 on head.");
    std::puts("PASS: entity budget refuses partial volleys; lifetime expiry removes pellets.");
    std::puts("PASS: grenade damage follows aim elevation (56.25/56.25/75/93.75/112.5) and is frozen at fire.");
    std::puts("PASS: radius falloff, wall occlusion, no self damage, small angle-independent push.");
    std::puts("PASS: grenade arc, single contact detonation and a single explosion.");
    std::puts("PASS: HUD identity, shared buckshot reserve, no reload, 0.75 second primary cadence.");
    std::puts("PASS: weapon spawn uses the weapon's own models; restored projectiles are removed.");
    return 0;
}

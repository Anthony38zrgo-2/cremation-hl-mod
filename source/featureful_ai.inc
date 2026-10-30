// Featureful AI (Fase 3) — subset implementable en el proxy Cremation.
// El donor hl_ravenholm.dll conserva la IA nativa; aquí aplicamos la capa
// Featureful sin reemplazar el donor:
//  1. stop_attacking_dying (monsters_stop_attacking_dying_monsters=true)
//  2. forget enemy por tiempo + LOS (npc_forget_enemy_time)
//  3. wander tras combate liberando el latch pegajoso (npc_active_after_combat)
//  4. cadáver no-sólido tras N segundos (Non-solid corpse / doors fade)
// Requiere enemy_combat_policy.inc incluido ANTES (usa su offset m_hEnemy).

static cvar_t ffForgetTime = {const_cast<char*>("cremation_ff_forget_time"), const_cast<char*>("8"), FCVAR_SERVER, 8.0f, nullptr};
static cvar_t ffWander = {const_cast<char*>("cremation_ff_wander"), const_cast<char*>("1"), FCVAR_SERVER, 1.0f, nullptr};
static cvar_t ffCorpseSolidTime = {const_cast<char*>("cremation_ff_corpse_time"), const_cast<char*>("3"), FCVAR_SERVER, 3.0f, nullptr};
static cvar_t ffStopDying = {const_cast<char*>("cremation_ff_stop_dying"), const_cast<char*>("1"), FCVAR_SERVER, 1.0f, nullptr};
static bool ffCvarsRegistered = false;

struct FeaturefulAiState {
    edict_t* entity = nullptr;
    int serial = 0;
    float lastEnemySeen = -1000.0f;   // última vez con enemigo vivo+visible
    float lastEnemyLostAt = -1000.0f; // cuándo se dejó de ver
    float corpseAt = 0.0f;            // cuándo murió (0 = vivo)
    Vector lastEnemyPos{0, 0, 0};
    bool hadEnemy = false;
};
static std::vector<FeaturefulAiState> ffStates;

static void featurefulAiRegisterCvars() {
    if (ffCvarsRegistered || !engine.pfnCVarRegister) return;
    engine.pfnCVarRegister(&ffForgetTime);
    engine.pfnCVarRegister(&ffWander);
    engine.pfnCVarRegister(&ffCorpseSolidTime);
    engine.pfnCVarRegister(&ffStopDying);
    ffCvarsRegistered = true;
}

static void featurefulAiReset() {
    featurefulAiRegisterCvars();
    ffStates.clear();
}

static FeaturefulAiState& featurefulAiFor(edict_t* e, bool create = false) {
    static FeaturefulAiState dummy{};
    if (!valid(e) || !engine.pfnIndexOfEdict) return dummy;
    int i = engine.pfnIndexOfEdict(e);
    if (i < 1 || (globals && i >= globals->maxEntities)) return dummy;
    if (i >= int(ffStates.size())) {
        if (!create) return dummy;
        ffStates.resize(globals ? globals->maxEntities : (i + 1));
    }
    auto& s = ffStates[i];
    if (s.entity != e || s.serial != e->serialnumber) {
        if (!create) return dummy;
        s = FeaturefulAiState{};
        s.entity = e;
        s.serial = e->serialnumber;
    }
    return s;
}

static bool ffIsMonster(edict_t* e) {
    return valid(e) && (e->v.flags & FL_MONSTER) && !(e->v.flags & FL_CLIENT) && e->pvPrivateData;
}

// Lee m_hEnemy nativo vía el offset descubierto por la policy (save-data scan).
static edict_t* ffNativeEnemy(edict_t* e) {
    if (!ffIsMonster(e)) return nullptr;
    if (enemyCombatEnemyOffset == -2) enemyCombatEnemyOffset = cultistFindEnemyOffset();
    if (enemyCombatEnemyOffset < 0) return nullptr;
    auto handle = static_cast<char*>(e->pvPrivateData) + enemyCombatEnemyOffset;
    auto target = *reinterpret_cast<edict_t**>(handle);
    auto start = reinterpret_cast<uintptr_t>(engine.pfnPEntityOfEntIndex(0));
    auto pointer = reinterpret_cast<uintptr_t>(target);
    if (!globals) return nullptr;
    if (pointer < start || pointer >= start + sizeof(edict_t) * globals->maxEntities ||
        (pointer - start) % sizeof(edict_t)) return nullptr;
    int serial = *reinterpret_cast<int*>(handle + 4);
    if (!valid(target) || target->serialnumber != serial) return nullptr;
    return target;
}

static void ffClearNativeEnemy(edict_t* e) {
    if (!ffIsMonster(e)) return;
    e->v.enemy = nullptr;
    if (enemyCombatEnemyOffset < 0) return;
    auto handle = static_cast<char*>(e->pvPrivateData) + enemyCombatEnemyOffset;
    *reinterpret_cast<edict_t**>(handle) = nullptr;
    *reinterpret_cast<int*>(handle + 4) = 0;
}

static bool ffVisible(edict_t* e, edict_t* target) {
    if (!valid(e) || !valid(target)) return false;
    Vector eye = e->v.origin + e->v.view_ofs;
    Vector to = (target->v.absmin + target->v.absmax) * 0.5f;
    TraceResult tr{};
    engine.pfnTraceLine(&eye.x, &to.x, 0, e, &tr);
    return tr.flFraction >= 0.999f || tr.pHit == target;
}

// PRE-think: se llama en hookThink ANTES de raven.pfnThink(e).
static void featurefulAiPreThink(edict_t* e) {
    if (!ffIsMonster(e)) return;
    if (e->v.deadflag != DEAD_NO || e->v.health <= 0) return; // cadáveres van al post
    auto& s = featurefulAiFor(e, true);
    if (s.entity != e) return;
    float now = globals ? globals->time : 0.0f;

    edict_t* enemy = valid(e->v.enemy) ? e->v.enemy : ffNativeEnemy(e);
    bool enemyAlive = valid(enemy) && enemy->v.health > 0 && enemy->v.deadflag == DEAD_NO;

    // 1. stop_attacking_dying: enemigo muriendo -> soltar YA (Featureful true).
    if (ffStopDying.value > 0.5f && enemy && !enemyAlive) {
        ffClearNativeEnemy(e);
        s.hadEnemy = false;
        s.lastEnemyLostAt = now;
        return;
    }

    if (enemyAlive) {
        // 2. forget por tiempo + LOS: solo cuenta como "visto" si hay línea real.
        if (ffVisible(e, enemy)) {
            s.lastEnemySeen = now;
            s.lastEnemyPos = enemy->v.origin;
            s.hadEnemy = true;
        } else {
            float forget = ffForgetTime.value;
            if (forget > 0.5f && s.lastEnemySeen > -999.0f && now - s.lastEnemySeen >= forget) {
                ffClearNativeEnemy(e);
                s.hadEnemy = false;
                s.lastEnemyLostAt = now;
                // 3. wander: liberar el latch pegajoso para que la IA nativa
                // + mansion_waves retomen patrulla en vez de esperar eternamente.
                if (ffWander.value > 0.5f) {
                    auto& latch = mansionEngageFor(e);
                    if (latch.actor == e) latch.engaged = false;
                }
            }
        }
    } else if (s.hadEnemy) {
        // Sin enemigo: si wander activo y pasó el olvido, liberar latch.
        float forget = ffForgetTime.value;
        if (ffWander.value > 0.5f && forget > 0.5f && now - s.lastEnemySeen >= forget) {
            auto& latch = mansionEngageFor(e);
            if (latch.actor == e) latch.engaged = false;
            s.hadEnemy = false;
        }
    }
}

// POST-think: se llama en hookThink DESPUÉS de raven.pfnThink(e).
static void featurefulAiPostThink(edict_t* e) {
    if (!ffIsMonster(e)) return;
    if (e->v.deadflag == DEAD_NO && e->v.health > 0) return;
    auto& s = featurefulAiFor(e, true);
    if (s.entity != e) return;
    float now = globals ? globals->time : 0.0f;
    if (s.corpseAt <= 0.0f) s.corpseAt = now;
    float solidTime = ffCorpseSolidTime.value;
    if (solidTime >= 0.0f && now - s.corpseAt >= solidTime) {
        // Non-solid corpse: no bloquear puertas/pushables/jugador.
        if (e->v.solid != SOLID_NOT) e->v.solid = SOLID_NOT;
    }
}

static void featurefulAiInstall(edict_t* e) {
    if (!ffIsMonster(e)) return;
    featurefulAiRegisterCvars();
    featurefulAiFor(e, true);
}

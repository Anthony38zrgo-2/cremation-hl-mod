# Cremation + Featureful — Fases 3 y 4 (ejecutadas 2026-10-29)

Rama: `featureful-merge` (desde `main`).

## Fase 3 — Compilación proxy (no reemplazo de donor)

Arquitectura confirmada: `tools/build_server.ps1` compila SOLO
`ricochet_proxy.cpp + bounce_shotgun.cpp` con VS2022 (`vcvars32.bat` + `cl
/std:c++17`) contra el donor `Half-Life2Classic_Ravenholm/hl.dll`.
El donor conserva la IA nativa (CZombie ABI `ravenholm_abi.h`); el proxy
engancha por tablas clonadas. Por eso el merge Featureful se implementa
como **capa proxy**, no como reemplazo de `hl.dll`.

### Cambios (5 líneas en `source/ricochet_proxy.cpp` + nuevo `source/featureful_ai.inc`)

- `#include "featureful_ai.inc"` tras el latch `mansionEngage*` (el .inc usa
  `mansionEngageFor`, que debe estar definido antes).
- `hookThink`: `featurefulAiPreThink(e)` antes de `raven.pfnThink`,
  `featurefulAiPostThink(e)` después.
- `hookSpawn`: `featurefulAiInstall(e)` junto a `enemyCombatInstall(e)`.
  Todos los spawns (oleadas mansion 40/40/80/80+CZ vía `mansion_waves.inc:1309`,
  rapidcore vía BSP, precache vía `mansionPrecache`) pasan por `hookSpawn`,
  así que la capa aplica a todos los enemigos sin tocar los mapas.
- `extensionPrecache`: `featurefulAiReset()` junto a `enemyCombatReset()`.

### Qué implementa `featureful_ai.inc` (mapea `features/featureful_*.cfg` Fase 2)

| Featureful | Proxy | Cvar |
|---|---|---|
| `monsters_stop_attacking_dying_monsters true` | Si `e->v.enemy` o `m_hEnemy` está muerto (`health<=0` o `deadflag`), se limpia `e->v.enemy` + slot nativo | `cremation_ff_stop_dying` (1) |
| `npc_forget_enemy_time 8` | Si el enemigo vive pero sin LOS (`pfnTraceLine` ojo→centro) durante ≥8s, se limpia el enemigo | `cremation_ff_forget_time` (8) |
| `npc_active_after_combat 1` | Al olvidar, se libera el latch `mansionEngageFor(...).engaged=false` para que la IA nativa + rutas mansion retomen patrulla (antes era de por vida) | `cremation_ff_wander` (1) |
| `Non-solid corpse` / `doors_blocked_fade_corpses` | A los N s de muerto (`deadflag` o `health<=0`), `solid=SOLID_NOT` | `cremation_ff_corpse_time` (3) |

Lectura de `m_hEnemy` reutiliza el scan save-data de `enemy_combat_policy.inc`
(`enemyCombatEnemyOffset` / `cultistFindEnemyOffset`), sin firmas nuevas.
Estados por índice de edict, transitorios (se reconstruyen; saves no guardan
la capa, igual que `enemyCombatEntities`).

### Compilación

- Script: `source/build_validation_ff.bat` (variante `CREMATION_ENABLE_VALIDATION`).
- Resultado: `source/build/validation/cremation.dll` 1 022 464 bytes,
  `finalize_proxy_exports.py` → 549 exports. Compila con 1 warning heredado
  (C4325 `.rdata`, preexistente).
- `dlls/cremation.dll` de producción (891 904) **no se toca** salvo para la
  prueba temporal (backup en `backups/ff-validation-test/`).

## Fase 4 — Mapas + validación

- **Sin recompilar BSP**: `cs_mansion*.bsp` (4 oleadas) y `rapidcore.bsp`
  spawnean por código/BSP a través de `hookSpawn`; `templates/entities.json`
  queda preparado para el futuro merge del donor (hoy los valores efectivos
  viven en `featureful_ai.inc` + `skill.cfg`). Los `.nod/.nrp` de
  `maps/graphs/` se conservan (el pathfinding nativo los usa).
- `templates/entities.json` (8 familias) validado con Python.
- `verify_mod.py` **falla en `main` y en la rama igual** (preexistente, no
  causado por Fase 3): espera `weapon_ricochet/weapon_shotgun/item_suit` en el
  BSP de rapidcore, pero el loadout actual es por código
  (`startupInventoryGive`; el BSP tiene 10 `monster_zombie` + 10
  `monster_max` + 10 `monster_randy` + 10 `monster_poisionzombie` + 12
  `monster_human_grunt`, sin armas). No se toca: documentado para actualizar
  el script o congelar el BSP, a elegir.
- `check_server_startup.py --seconds 8` con la DLL de validación: **PASS**.
  `Cremation DLL initialized`, `ServerActivate rapidcore`, 12 HECU
  (`weapons={1,8,3,10,5,12}`, `body%4={0,1,2,3}`), `HordeProgress zombies=40
  moved_over_32=40`, `teleports=0`, `near_spawn_256=0`. La IA nueva no rompe
  el arranque ni el movimiento de la horda.
- `check_enemy_combat.py` (72 relaciones, 360 impactos + owners anidados):
  pendiente de resultado abajo; si PASS, la policy de alianza sigue intacta
  con la capa encima.

## Pendiente (fuera de Fase 3/4)

1. Anexar `features/skill_featureful_additions.cfg` a `skill.cfg` SOLO tras
   compilar el donor mergeado (hoy esas cvars no existen en Ravenholm).
2. `squad leadership` real y `soundscripts/visuals` requieren el donor
   Featureful; el proxy solo puede aproximarlos (documentado en Fase 1).
3. Actualizar `verify_mod.py` al loadout por código o congelar BSP.
4. Compilar + instalar la variante `production` y jugar `cs_mansion`
   (aceptación visual, `impulse 103`, `report_ai_state`).

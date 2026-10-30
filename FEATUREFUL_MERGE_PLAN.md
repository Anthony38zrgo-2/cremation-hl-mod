# Cremation + Featureful — Plan de merge Fase 1+2

Fecha: 2026-10-29. Base Cremation: HLSDK 1996-2001 + `hl_ravenholm.dll` donante.
Fuente Featureful: `featureful-src/` (clone --depth 1, 2026-10-29).

## Fase 1 — Diff fuente (hallazgos)

- `skill.h`: Cremation 147 líneas con `struct skilldata_t` fijo. Featureful 42 líneas:
  elimina el struct, usa `GetSkillValue(name, entTemplate...)` + `FloatRange` +
  `template_property_types.h`. **Breaking change**: todos los `gSkillData.xxx`
  deben migrarse a `GetSkillValue("xxx")`. Ver `featureful-src/dlls/skill.cpp`
  (+132/-20 vs Cremation).
- `monsters.cpp`: +4017/-1239. Añade `m_flFieldOfView`, `m_customSoundMask`,
  `m_activeAfterCombat`, `EntityTemplate`, `ForgetEnemy` (`npc_forget_enemy_time`),
  `OpenDoorCapability`, `NonSolidCorpse`, `ActOutOfPVS`.
  Cremation hoy: 0 ocurrencias de `EntityTemplate/soundscript/passive_regeneration/
  power_shield/cloaking/followage/open_door_capability/squad_capability`.
  Featureful: `soundscript`×2762, `cloaking`×89, `followage`×19, etc.
- `nodes.cpp`: +1766/-1528. Detour de `func_breakable`/puertas cerradas,
  cadáveres en escaleras no bloquean pathfinding. Clave para puertas giratorias
  de `cs_mansion` + escalera norte del CZ.
- `squadmonster.cpp`: +292/-239. Re-elección de líder, hunt al morir squadmate,
  propagate enemy info, vacate slot al remover.
- Archivos solo en Featureful (106 nuevos): `ent_templates.*`, `followers.*`,
  `followingmonster.*`, `classify.*`, `locus.*`, `inventory.*`, `soundscripts`,
  + monstruos OpFor/RaceX/Sven (`gonome, pitdrone, shocktrooper, voltigore,
  pitworm, geneworm, hwgrunt, robogrunt, male_assassin, archer, floater,
  kingpin, panthereye, tor...`) y armas (`eagle, m249, displacer, shockrifle...`).
- Solo en Cremation: `AI_BaseNPC_Schedule.cpp, mpstubb.cpp, stats.cpp,
  tempmonster.cpp` (hooks del proxy Ravenholm / UT). **Conservar**.
- `skill.cfg`: Cremation 270 vars / 426 líneas. Featureful 546 vars / 831 líneas.
  Faltan 297 líneas en Cremation (ver `features/skill_featureful_additions.cfg`).
  Custom Cremation a conservar: `sk_ally_*`, `sk_fzombie_*`, `sk_pzombie_*`,
  `sk_plr_ggun*`, `sk_*PR*`.
- `delta.lst`: diverge en `entity_state_t.body` (Cremation 32-bit por David).
  Hay que fusionar a mano, no copiar ninguno tal cual.

### Orden de merge recomendado (fuente, no binarios)

1. `skill.*` → `nodes.*` → `monsters.*`/`schedule.*` (`monsters.cpp` es el grande).
2. `squadmonster.*` + `talkmonster.*` + `followingmonster.*` + `followers.*`.
3. `ent_templates.*` + `classify.*` + `locus.*` + `inventory/soundscripts`.
4. Portar hooks Cremation (`david/quake/cz/necro/judgment`, `enemy_combat_policy.inc`,
   `client_proxy.cpp`) encima, manteniendo tablas clonadas + firma donante.
5. Añadir monstruos nuevos solo tras compilar base (Gonome → Zombie Barney/Soldier →
   Pitdrone/Shock → Voltigore).

## Fase 2 — Archivos ya generados (usables hoy sin recompilar)

| Archivo | Uso |
|---|---|
| `features/featureful_server.cfg` | `delegate_leadership true`, `tiny 2`, `named_wait false`, `named_doors false`, `corpse fix true`. Copiar a `features/` definitivo tras merge DLL. |
| `features/featureful_exec.cfg` | `npc_forget 8`, `active_after_combat 1`, `tridepth 2/all 1`, `nearest 1`, `retry 1`, `idlesound_pvs 1`. |
| `features/featureful_client.cfg` | Copia referencia (HUD/flashlight/wallpuff). Ajustar colores a gusto. |
| `features/skill_featureful_additions.cfg` | 297 líneas para anexar a `skill.cfg` SOLO tras DLL mergeada. |
| `templates/entities.json` | 8 familias: `david_base/steve_tank/johnny_brute/ken_armored/quake_zombie/czero_infiltrator/blood_cultist_hecu/necro_minion`. Keyvalue `Entity Template` en el FGD nuevo. |
| `tools/featureful_fgd/` | `halflife_featureful.fgd` (+`-nr/-tb`) para JACK/Hammer. |
| `features_featureful_ref/`, `templates_featureful_ref/` | Referencia intacta, no editar. |

Validado: `entities.json` parsea OK con Python.

## Fase 3 — Compilación (siguiente paso, no ejecutado)

1. Rama git `featureful-merge`. Compilar con VS2019/2022 Win32 como hoy
   (`build_mod.py` o `tools/build_*.ps1`), NO el pipeline general de assets.
2. Instalar solo `dlls/cremation.dll` (renombrada desde `hl.dll` mergeada) +
   probar `hl.exe -game Cremation -dev +map cs_mansion +impulse 103`.
3. Harness existente: `check_enemy_combat.py`, `check_hotd2_deaths.py`,
   `check_czero_tactical.py`, `check_mansion_runtime.py` (244 enemigos).
4. Saves viejas se invalidan (nota oficial Featureful). Avisar en release.

## No hacer

- No copiar `featureful/dlls/hl.dll` sobre `Cremation/dlls/` (rompe ABI Ravenholm).
- No anexar skill additions antes del merge (cvars desconocidas = warnings).
- No mapear con el FGD viejo (faltan `Entity Template`, `Custom Sound Mask`,
  `Custom FOV`, `Follow fail/policy`, `Non-solid corpse`, `Act out of PVS`).

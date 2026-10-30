# Reescritura completa de Cremation sobre Half-Life Featureful

Fecha: 2026-10-29. Fuentes: [Featureful SDK Overview](https://freeslave.github.io/halflife-featureful/),
[Developing the mod](https://freeslave.github.io/halflife-featureful/docs/development/developing-the-mod/),
[Feature configuration](https://freeslave.github.io/halflife-featureful/docs/configuration/),
[Monsters](https://freeslave.github.io/halflife-featureful/docs/entity-guide/monsters/),
[Entity templates](https://freeslave.github.io/halflife-featureful/docs/configuration/entity-templates/),
[Weapon templates](https://freeslave.github.io/halflife-featureful/docs/configuration/weapon-templates/),
[Soundscripts](https://freeslave.github.io/halflife-featureful/docs/configuration/soundscripts/),
[Visuals](https://freeslave.github.io/halflife-featureful/docs/configuration/visuals/),
[Skill variables](https://freeslave.github.io/halflife-featureful/docs/configuration/skill-variables/),
[Locus](https://freeslave.github.io/halflife-featureful/docs/entity-guide/locus-system/),
[Compatibility](https://freeslave.github.io/halflife-featureful/docs/extras/compatibility/).
Código de referencia: `featureful-src/` (clonado 2026-10-29).

## 1. Por qué reescribir en vez de seguir con el proxy

Arquitectura actual: `cremation.dll` es un proxy de 2 `.cpp` que engancha por
**tablas virtuales clonadas** un donor cerrado (`hl_ravenholm.dll`,
ABI fijada en `ravenholm_abi.h`: slots 8/10/11/13/14/17/57/63/73/93/114 + RVAs).
Toda la IA custom vive en ~40 `.inc` con hooks frágiles (firma del donor,
offsets por scan de save-data, `VirtualProtect` por slot). Cada sistema nuevo
cuesta un hook más. El `AGENTS.md` del repo lo demuestra: el bug del muzzle del
pulse gun existe **porque** viewmodel y mundo viven en DLLs distintas.

Reescribir sobre el fork `featureful-src` convierte el 80% de esos hooks en
**datos** (JSON/cfg) y deja código solo donde hay comportamiento nuevo de
verdad (anatomía HOTD2, bomba CZ, reactor). El juego pasa a compilarse como un
mod normal (`hl.dll` + `cl_dlls/client.dll` propios), actualizable con el
paquete dev-only ([Updating](https://freeslave.github.io/halflife-featureful/docs/development/updating-your-mod/)).

## 2. Base del proyecto

1. `source/hlsdk` deja de ser el submódulo `ValveSoftware/halflife` y pasa a
   ser un fork de `FreeSlave/halflife-featureful` (rama `cremation`).
   Compila con VS2022 Win32 igual que hoy; además se gana Linux/Xash3D gratis.
2. `Cremation/` se convierte en mod Featureful estándar: `dlls/`, `cl_dlls/`,
   `delta.lst` (fusionando el `body` 32-bit de David), `events/`, `features/`,
   `templates/`, `sound/soundscripts.json`, `skill.cfg`.
   [Getting started](https://freeslave.github.io/halflife-featureful/docs/development/getting-started/).
3. Desarrollo con `-dev +developer 2 +sv_cheats 1` y el loop de validación
   nativo: `dump_soundscripts`, `dump_visuals`, `get_skill`,
   `report_ai_state`/`watch_ai_schedules`, `impulse 103/101`,
   `calc_position`, `entities_count`.
   ([Comandos](https://freeslave.github.io/halflife-featureful/docs/development/developing-the-mod/))

## 3. Mapa de migración: cada `.inc` actual → sistema Featureful

### 3.1 Enemigos base y horda (David/Steve/Johnny/Ken/Max/Randy, fast/poison/purple)

Hoy: hooks sobre `CZombie` + `enemy_combat_policy.inc` + `mansion_waves.inc`
(oleadas 40/40/80/80 hardcodeadas en C++).

- Clases reales en el fork derivadas de `monster_zombie`
  (donante de comportamiento: `monster_gonome`, `monster_zombie_soldier`,
  `monster_zombie_barney` ya existen). Se acabaron las tablas clonadas.
- Diferencias por variante en `templates/entities.json` (ya iniciado en Fase 2):
  `health` (con rangos `[90,110]` para variedad natural),
  `check_melee_attack1/2`, `trace_hull_attacks {1,2,3}`,
  `gib_disposition` (calavera `hgibs` al 5% como el ejemplo oficial),
  `squad_capability`, `open_door_capability`, `soundscripts`,
  `autoprecache_sounds:true` (los eventos 1004/1008/1011-1013 del modelo
  HOTD2 se precachean solos), `skill` overrides por template.
  [Entity templates](https://freeslave.github.io/halflife-featureful/docs/configuration/entity-templates/)
- `enemy_combat_policy.inc` (R_AL entre enemigos) → `classify` nativo:
  facción zombie única + `squad_capability.allow_different_classification`,
  sin tocar el slot 63. El test de 72 relaciones se reescribe como test de
  `classify`, no de hooks.
- `mansion_waves.inc` → `monstermaker` + `templates/warpball.json`
  (teleport de oleadas con efecto) + `squad` + contadores
  (`game_counter`, `trigger_condition`, `multisource`/`env_state` como
  [masters](https://freeslave.github.io/halflife-featureful/docs/entity-guide/master-entities/))
  + `journal` para el estado de oleada. La oleada pasa de 1500 líneas de C++
  a entidades + 1 módulo fino de conteo/respawn CZ.
- Comportamiento global por cfg, sin código:
  `monsters_delegate_squad_leadership true`,
  `monsters_stop_attacking_dying_monsters true`,
  `npc_forget_enemy_time 8`, `npc_active_after_combat 1`,
  `npc_tridepth 2 + npc_tridepth_all 1`, `handle_tiny_creatures 2`,
  `doors_blocked_fade_corpses true`.
  (Nuestro `featureful_ai.inc` de Fase 3 es el prototipo: al reescribir se
  **borra**, lo hace el motor.)

### 3.2 Daño localizado / desmembramiento HOTD2: DESCARTADO

El desmembramiento de David (anatomía 16 nodos, thresholds cabeza/limbs,
gibs locales, `cremation_david_local_damage`) es una feature descartada y no
se porta. David/Steve/Johnny/Ken/Max/Randy usan muerte y gibbing estándar
(`humanGibSpawn`, 6 piezas `hgibs`, configurables por `gib_disposition`).
Los assets de partes locales (`models/cremation/hotd2/part_*.mdl`) y las
cvars `david_*_fraction`, `cremation_david_local_damage` se eliminan en la
reescritura. El modo actual (`local_damage 0`, todo por CZombie nativo) ya es
el comportamiento final: no hay nada que migrar aquí.

### 3.3 Quake zombie

Hoy: Think/anim/daño propios + tabla clonada. En rewrite: clase propia
`monster_quake_zombie` en el fork (el código de Quake-reglas 60/25/regen se
conserva casi tal cual, pero como clase, no como hook) + template:
`size 32x32x104`, `trace_hull_attacks` largo, `damage_info` con
`type_policy:add` para plasma/fuego persistente, `children:null`.
El proyectil de carne pasa a `projectile` con `detonate_on_touch` y visual
propia. `quake-zombie-patrols.json` se sustituye por `path_corner` nativos
(ya funcionan patrullando en Featureful, con wait times).

### 3.4 CZ infiltrador + bomba + HECU

Hoy: `czero_zombie.inc` (física de escalera, granadas con conteo, Glock
clip 20, bomba 35s, defuse 5s) + `czero_navigation.h` generado del BSP.

- `monster_czero_zombie` como clase OpFor-like + template:
  `open_door_capability:true` (adiós física de ventana),
  `primary_weapon:[{max_clip:20}]`, `squad_capability:{can_recruit:true}`,
  granadas como ataques `projectile` con cooldown por `skill`
  (`sk_*_maxcharge` existe para shocktrooper como modelo).
- Bomba/defuse/reactor: entidades (`func_bombzone`-like propio mínimo +
  `trigger_hurt_remote` con `Target [LE]=!activator`,
  [locus](https://freeslave.github.io/halflife-featureful/docs/entity-guide/locus-system/)
  para posiciones de plantado en rehenes) + `journal` (objetivo) +
  `subtitles`/`messagebox` (avisos). La navegación generada del BSP se
  elimina: nodos del mapa + `npc_tridepth` + puertas que la IA ya sabe abrir.
- HECU cultistas: `monster_human_grunt_ally`-like + `speech_prefix`,
  `followers.json` para los que siguen al jugador, `medic_drop_healthkit`.
- `skillopfor.cfg` ([doc](https://freeslave.github.io/halflife-featureful/docs/configuration/skill-variables/#skillopforcfg))
  separa el balance OpFor/CZ del `skill.cfg` base.

### 3.5 Necromancer + minion + Judgment boss

- Minion: `classify:"Player Ally"` + `followers.json` +
  `scripted_following` (reemplaza el `Classify` condicional) +
  `equipment_drop`/`loot_drop` (corazones) + `passive_regeneration` leve.
- Orbe rojo / detonación: arma `weapon_necromancer` como template
  `projectile` (`speed 450`, gravedad) + `tool`/`user command` para la
  detonación remota ([user commands](https://freeslave.github.io/halflife-featureful/docs/configuration/user-commands/)).
- Judgment boss: `power_shield` (con `PowerShield.Render/Debris`),
  `active_regeneration` (con `ActiveRegen.*`), `cloaking` si aplica,
  `children` (invocaciones), `loot_drop`. Hoy es código; pasa a ser 90% JSON.

### 3.6 Sonido: borrar `blood_*voices.inc`, `sentences.txt` artesanal

- `sound/soundscripts.json` con `waves[]` + `pitch/volume/attenuation`
  por variante (David/Steve/Johnny/Ken/CZ/cultista/Quake). Derivados al
  estilo `Otis.Pain ← Barney.Pain` para reutilizar (documentado en
  [derived soundscripts](https://freeslave.github.io/halflife-featureful/docs/configuration/soundscripts/#derived-soundscripts)).
- `wave:""` para silenciar (útil para asesinos silenciosos).
- Eventos de animación del modelo HOTD2 (1014) → soundscripts con
  `autoprecache_soundscripts:true`. Ver
  [model animation events](https://freeslave.github.io/halflife-featureful/docs/entity-guide/model-animation-events/).

### 3.7 Visuales: borrar `humanoid_gibs.inc`, sangre/efectos hardcodeados

- `templates/visuals.json`: beams del Link Gun (`color/width/noise/life`),
  `Bullsquid.Spit`-style para plasma, `Regen.*`/`PowerShield.*` para boss,
  `NPC.Gibbed`, `NPC.BodyDrop_Heavy/Light`, `NPC.SwishSound/AttackHit/Miss`
  para melee. ([Visuals](https://freeslave.github.io/halflife-featureful/docs/configuration/visuals/))
- `own_visual` (modelos/carbonizados por template) + `gib_visual` +
  `gib_disposition` (6 piezas `hgibs` + calavera ocasional a la cara, como el
  ejemplo oficial).

### 3.8 Armas: borrar `client_proxy.cpp`, `bounce_shotgun.cpp` como armas-code, 521 forwarders

Es el mayor ahorro. `templates/weapons.json` (+ presets en
`templates/weapons/`, [presets](https://freeslave.github.io/halflife-featureful/docs/configuration/weapon-templates-presets/))
cubre, sin C++: `fire`/`alt_fire` (`type: bullet/melee/projectile`,
`damage` por skill/range/`damage_info`, `burst`, `spread` universal/CS,
`kickback`, `cycle_time`, `hit_cycle_time`, `pump_delay`, `charge_*` para el
pipewrench cargado, `zoom` para la mira, `laser_spot` para shock,
`projectile` con `speed/cooked/grenade_physics`, `shell`, `recharge`,
`reload` por fases, `switch_mode` para Glock→Glock burst, `tool` para el
necromancer). El bug de `AGENTS.md` (muzzle desalineado por proxy) desaparece
por construcción: vista y mundo en el mismo DLL.
`ricochet_proxy.cpp`, `forwarders.inc`, `finalize_proxy_exports.py` y los
`client_*.inc` se eliminan. `weapon_blood.inc`, `bounce_shotgun_balance.h`,
`weapon_config.h` → `ammo_types` + `ammo_amounts.cfg` + `maxammo.cfg` +
`hud_weapon_layout.cfg` + `weapon_carry_categories.cfg`.

### 3.9 HUD/cliente/mapas: borrar `cl_dlls/client_ravenholm.dll` + `client_proxy.cpp`

- `cl_dlls` de Featureful + `featureful_client.cfg` (ya creado en Fase 2):
  color/escala HUD, crosshair, wallpuffs, light de muzzle.
- Tactical/Judgment/radar/mansion HUDs por mensajes custom →
  `hud.txt` + `monsterinfo` (`sp_allowmonsterinfo 2` ya puesto) +
  `journal` (objetivos bomba/oleadas) + `messagebox` + `hud_message`/`titles` +
  `subtitles` (`captions.txt`) para voces HOTD2/CZ.
- Mapas: FGD featureful (ya en `tools/featureful_fgd/`), `master entities`
  para compuertas del reactor, `calc_*` + `!activator`/`*locus` para la lógica
  que hoy está en `mansion_reactor.inc`/`tactical_hud.inc`, `map config`
  (`.cfg` por mapa) para cvars por mapa, `followers.json` para aliados.
- `displaynames` + `save_titles.txt` + `player_templates` para el toque final.

### 3.10 Saves y guardado

`CremationDavid1`/`CremationTactical1` + campos manuales →
`TYPEDESCRIPTION` en clases reales (save nativo, más robusto) +
`delta.lst` de Featureful fusionado + `savetitles`. Los saves viejos se
invalidan una vez (avisar en release, como indica la
[doc de update](https://freeslave.github.io/halflife-featureful/docs/development/updating-your-mod/#notes)).

## 4. Límites honestos (lo que Featureful NO trae)

- No hay base `CZombie` Ravenholm: la anatomía HOTD2 se porta igual (3.2).
- `move-with` no existe (rama separada sin merge); usar `motion_manager`.
- Sin partículas Aurora; sin CTF; sin `weapon_uzi_akimbo`; sin skill por mapa
  (solo global + `skillopfor.cfg`).
- Masters limitados a ciertas entidades (no cualquier entidad es master como
  en Spirit tardío); `trigger_relay` con semántica SvenCoop (flag para
  forward-activator); `func_pushable` 512 = `Explosives only`.
  ([Compatibility](https://freeslave.github.io/halflife-featureful/docs/extras/compatibility/))
- Todo lo anterior tiene workaround en el plan; nada bloquea.

## 5. Plan de ejecución (compilando siempre, jugable siempre)

1. Rama `rewrite-featureful` desde `featureful-merge`. Fork como `source/hlsdk`.
   Primer hito: compila + `cs_mansion` arranca con zombies vanilla = éxito.
2. Oleada 1: skill (rangos + `get_skill`) → templates 8 familias →
   soundscripts/visuals base. Harness: `dump_*` + `verify_mod` actualizado.
3. Oleada 2: clases David-set (solo templates + `trace_hull`, sin anatomía)
   → Quake (3.3) → CZ/bomba (3.4) → necro/boss (3.5). Cada una con su
   `check_*` adaptado.
4. Oleada 3: armas por templates (3.8) → HUD/cliente (3.9) → mapas
   (masters/locus/journal) → saves (3.10).
5. Cierre: `shipping` ([doc](https://freeslave.github.io/halflife-featureful/docs/development/shipping-your-mod/)),
   `skillopfor.cfg`, balance final con rangos, release con aviso de saves.

Estimación de código resultante: ~40 `.inc` con hooks → ~5 módulos propios
(bomba, reactor, conteo oleadas, 2 armas exóticas si alguna no cabe
en templates) + JSON/cfg. El resto se borra.

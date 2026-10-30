# Cremation → Featureful — mapa de migración (.inc/.h)

86 archivos custom en `Cremation\source\` (se ignoran `hlsdk\`, `build\`, `featureful\`).
Los `.cpp` (`ricochet_proxy.cpp`, `client_proxy.cpp`, `bounce_shotgun.cpp`, `test_*.cpp`) quedan fuera de esta tabla: se reescriben como clases del fork, no se migran.
Destinos JSON (`templates/entities.json`, `sound/soundscripts.json`, `templates/visuals.json`, `templates/weapons.json`) viven en el contenido del mod según [docs Featureful](https://freeslave.github.io/halflife-featureful/); `skill.cfg`, `skillopfor.cfg` y `features/*.cfg` igual. Verificado en el fork: `dlls/` ya trae `shockrifle.cpp`, `controller.cpp`, `turret.cpp`, `monstermaker.cpp`, `locus.cpp`, `ent_templates.cpp`, `classify.cpp`, `relationship.h`, `npc_behavior.h`, `sentences.h`, `skill.cpp`; `cl_dll/` trae `hud_journal.cpp`, `hud_meter.cpp`, `hud_monsterinfo`, `inventory_hud.cpp`, `objecthint_manager.cpp`; `features/` trae `featureful_server.cfg`, `featureful_weapons.cfg`, `ammo_amounts.cfg`, `maxammo.cfg`.
Prioridad: **P0** = sin esto no hay modo Mansion jugable · **P1** = contenido a portar tras P0 · **P2** = eliminar (proxy/ABI/probes/datos generados).

| Archivo | Qué hace hoy | Destino Featureful | Prio |
|---|---|---|---|
| `mansion_waves.inc` (~108 KB) | Director de oleadas 1–4: estado en entvars (`frags`=oleada), spawn por sector, tipos, velocidades, asedio a puertas, scientist señuelo, drop de RPG | Entidades del mapa: `monstermaker` por sector + `game_counter`/`multisource` + `trigger_*`; velocidades a `skill.cfg`; puertas a `func_door`/`func_breakable` | P0 |
| `mansion_navigation.inc` | Nav de horda incremental: presupuesto, caché de traces, reintentos, patrullas/sitio | ELIMINA lógica; nodos `info_node`/waypoints en el `.bsp` + schedules nativos | P0 |
| `czero_navigation.h` (156 KB) | Grafo nav `czNav[]` generado para el CZ | ELIMINA; rehacer con nodos del mapa | P0 |
| `enemy_combat_policy.inc` | Política global hostiles: retaliación/guardia, `m_hEnemy`, excluye `monster_minion`/`cremation_sentry` | `dlls/` C++ (base `CBaseMonster`/gamerules) + `features/featureful_server.cfg` (`npc_forget_enemy_time` y cvars `monsters_*`) | P0 |
| `featureful_ai.inc` | Capa parcial Featureful vía cvars `cremation_ff_*` (stop-attacking-dying, forget+wander, cadáver no-sólido) | `features/featureful_server.cfg` nativo (`monsters_stop_attacking_dying_monsters`, `npc_forget_enemy_time`, `npc_active_after_combat`); ELIMINA el `.inc` | P0 |
| `david_zombie.inc` + `david_logic.h` | Variantes HOTD2 sobre CZombie nativo (`monster_zombie/johnny/max/ken`): chances por cvar, hacha Johnny, hitboxes regionales, crawl, `sk_zombie_dmg_one_slash` | C++ nueva (subclase CZombie; trace regional en la clase) + `skill.cfg` + `sound/soundscripts.json` + `templates/visuals.json` | P0 |
| `czero_zombie.inc` | `monster_czero_zombie` sobre HGrunt: knife pursuit, daño/muerte propios, `ScreenFade` | C++ nueva (hereda `CHGrunt`) + `skill.cfg` + `soundscripts.json` + `templates/visuals.json` | P0 |
| `czero_objective.inc` | IA objetivo: entradas, escaleras, planta bomba `cz_planted_bomb` (35 s), defuse 5 s con kit, sonidos c4 | C++ (schedules/tasks en la clase CZ) + entidades mapa (zonas, `func_breakable`) + `soundscripts.json` | P0 |
| `mansion_cz_respawn.inc` | Reaparición solo del NPC táctico, cola en entvars guardadas | `monstermaker` + `game_counter` en mapa; estado a la clase CZ | P0 |
| `mansion_radar.inc` + `client_radar.inc` + `radar_geometry.h` | Servidor publica `HordeRadar`+wave; cliente dibuja radar con rects de terreno | C++ nueva en `cl_dll/` (HUD radar) + mensaje; rects a datos de mapa/`features/*.cfg` | P0 |
| `tactical_hud.inc` + `client_tactical_hud.inc` | Snapshot servidor (bomba, puertas 2000 HP, oleada, timers DMC) + overlay cliente con sprites propios | C++ nueva en `cl_dll/` (HUD táctico; ver `hud_meter.cpp` como base) + `templates/visuals.json` (sprites) | P0 |
| `tactical_save.inc` | Guarda `iuser/fuser/vuser` vía TYPEDESCRIPTION del donor | `DEFINE_FIELD` en las clases C++ (save-restore nativo Featureful) | P0 |
| `weapon_registry.inc` | Filtra/renombra `WeaponList` (357, shockrifle, ricochet, nailgun, pipewrench, rpg, handgrenade) | `templates/weapons.json` + `features/ammo_amounts.cfg`, `maxammo.cfg` | P0 |
| `weapon_blood.inc` | Puffs de sangre + escalas de daño (DMC, melee David, engage en herir) | `dlls/combat.cpp` + multiplicadores a `skill.cfg`/`features/*.cfg`; sangre a motor de decals | P0 |
| `mansion_waves.h` | Composición oleadas: `hordeTotal`, `kind()`, `classname()` (zombie/max/randy/tpose/grunt/czero), perímetro cultistas | Datos para `monstermaker`/`game_counter` del mapa + `skill.cfg` | P0 |
| `quake_zombie.inc` | `monster_quake_zombie` independiente estilo `zombie.qc` + `quake_zombie_gib`, 10 sonidos | C++ nueva + `skill.cfg` + `soundscripts.json` + `templates/visuals.json` | P1 |
| `tpose_zombie.inc` | `monster_tpose`: vuelo estilo Controller, bola de energía, muerte tipo minion, `sk_controller_speedball1` | C++ nueva (hereda `CController`) + `skill.cfg` + `soundscripts.json` | P1 |
| `judgment_boss.inc` | `monster_kuarl`+`monster_zeal` (carriers Gargantua/Controller), HP compartida 1500/1050/450, slot de oleada | C++ 2 clases + `skill.cfg` + `soundscripts.json` + `templates/entities.json` | P1 |
| `client_judgment_hud.inc` | Overlay HP del boss + música según estado (`judgment_leprosy`, `myfuneral_loop`, `menu_introduction`) | `hud_monsterinfo`/`hud_meter` + `soundscripts.json`; trigger musical vía entidad `target_cdaudio`/ambient | P1 |
| `necromancer.inc` + `necro_charred.inc` | Arma sobre `v_hgun`: invoca `monster_minion` (Classify 11), fuego; tabla 19 modelos charred | `templates/weapons.json` + summon vía `monstermaker`/template + `templates/visuals.json` (charred) + `soundscripts.json` | P1 |
| `link_gun.inc` + `link_plasma.inc` | Secundaria continua (ammo nailgun, beam, glows; precache en mundo) + sprites plasma body/core/halo | `templates/weapons.json` + `templates/visuals.json` + `soundscripts.json`; boca del viewmodel según `AGENTS.md` (matriz de huesos, no offset fijo) | P1 |
| `shockrifle.inc` | Shock Rifle sobre `v_gauss`: hitscan, orbe, combo, msg `ShockFx` | `templates/weapons.json` (base `dlls/shockrifle.cpp` ya existe; portar combo) + `visuals.json` + `soundscripts.json` | P1 |
| `ut2004_rpg.inc` | RPG cargado (carga múltiple, vuelo recto) sobre `weapon_rpg` | `templates/weapons.json` + ajuste a `dlls/rpg.cpp` + `soundscripts.json` | P1 |
| `tfc_gameplay.inc` | Escopeta TFC (8 cartuchos) + sentry engineer (`monster_sentry`, metal, base+arma) | `templates/weapons.json` + sentry a `dlls/turret.cpp` existente o `templates/entities.json` + `soundscripts.json` | P2 |
| `bounce_shotgun.h` + `bounce_shotgun_balance.h` | `weapon_bounce_shotgun` flak: perdigones con rebote, granada de contacto; balance | C++ nueva + `templates/weapons.json` + `soundscripts.json`; balance a `features/*.cfg` | P1 |
| `deagle.inc` | Desert Eagle sobre carrier Egon, 7+reserva .357 | `templates/weapons.json` `weapon_deagle` + `soundscripts.json` + `skill.cfg` | P1 |
| `awp.inc` | AWP CZ sobre carrier Python: zoom, tiempos, presentación | `templates/weapons.json` `weapon_awp` + `soundscripts.json` + `skill.cfg` | P1 |
| `winchester1887.inc` | Winchester sobre .357: 8 perdigones, 8 sonidos | `templates/weapons.json` `weapon_winchester1887` + `soundscripts.json` | P1 |
| `weapon_config.h` + `weapon_config_bindings.inc` | Lector JSON de balance (flak + ricochet) con rangos | `features/*.cfg` (cvars); ELIMINA lector | P1 |
| `ricochet_balance.h` | Balance disco (velocidad, slow, radios, cargas) | `features/*.cfg`; arma a C++ nueva `weapon_ricochet` (hoy en `ricochet_proxy.cpp`) | P1 |
| `dmc_powerups.inc` | 4 artefactos 30 s (`item_artifact_*`) + suit, timers en `fuser` | `templates/entities.json` + `soundscripts.json`; timers a save-restore de la clase | P1 |
| `heart_pickups.inc` | Loot `cremation_heart`, roll al morir, lifetimes en entvars | `templates/entities.json` (item de salud) + `skill.cfg` (probabilidad/valor) | P1 |
| `blood_zombie_voices.inc` + `blood_zombie_assets.h` | Voces `monster_zombie` por evento (Rise/Swipe/Step/Alert/Idle/Pain/Death/Attack) | `sound/soundscripts.json` (+ `sentences.h` si aplica) | P1 |
| `blood_cultist_assets.h` + `blood_cultist_voices.h` | Voces `monster_human_grunt` cultista (Alert/Search/Pain/Death/Victory/Burning) | `sound/soundscripts.json` | P1 |
| `hecu_alliance.h` | HECU con Classify 7 (aliado de zombies) | `dlls/relationship.h`/tabla de relaciones o `skillopfor.cfg` | P1 |
| `humanoid_gibs.inc` | Gibs anatómicos con sangre/decals para zombie/grunt/ken/randy | `templates/visuals.json` (modelos gib) + cvars `violence_*` existentes | P1 |
| `czero_smoke.inc` | Humo/flash del CZ (sprites, `ScreenFade`, bloqueo LOS `czSmokeBlocks`) | Entidad granada C++ + `templates/visuals.json` + `soundscripts.json` | P1 |
| `mansion_reactor.inc` + `reactor_layout.h` | Pit radiactivo, launch pad balístico, recompensas deathmatch (pool bounce/nailgun/shockrifle/rpg, displays, stations) | Entidades mapa (`trigger_hurt`, `trigger_push`, pickups `weapon_*`, `game_counter`) + `soundscripts.json` | P1 |
| `mansion_revolver_ammo.inc` | 8 pickups .357 (4 por planta) + marker anti-dupe | Entidades `ammo_357` en el mapa (+ `weaponstay`) | P1 |
| `vostok_cover.inc` | Props cover (`vostok_cover_*`, `truth_*`): malla animada + `func_wall` invisible | Entidades mapa (`cycler`/`func_wall`/clip) + `templates/visuals.json` | P1 |
| `client_music.inc` | Reproductor PCM externo (waveOut, hilo, crossfade) para menú/oleadas | ELIMINA hilo; música a `soundscripts.json` + entidad ambient/`target_cdaudio` | P1 |
| `client_weather.inc` | Lluvia roja + niebla carmesí exterior (tri API, cvars `cremation_red_rain/_density`) | `env_rain`/niebla del mapa + `features/*.cfg`; ELIMINA render cliente | P1 |
| `david_assets.h` / `steve_assets.h` / `johnny_assets.h` / `max_assets.h` / `ken_assets.h` | Secuencias/huesos generados por `tools/convert_hotd2_*.py` | `templates/visuals.json` (secuencias); ELIMINA fuente generada | P1 |
| `client_cs_weapons.inc` | Presentación CS (modelos/anim/FOV) desacoplada de Python/Egon | `templates/visuals.json`; ELIMINA hook cliente | P2 |
| `client_revolver.inc` | Restaura sprite crosshair .357 | ELIMINA (HUD Featureful lo gestiona) | P2 |
| `client_font.inc` | Raster GDI de fuentes OFL a texturas GL | ELIMINA (VGUI/HUD Featureful) | P2 |
| `forwarders.inc` | 521 tail-jumps ABI del donor (nombres + clases + armas) | ELIMINA (link directo al fork) | P2 |
| `client_forwarders.inc` | Stubs ABI del cliente (vgui/engine) | ELIMINA | P2 |
| `ravenholm_abi.h` | Slots vtable verificados (Classify=8, TakeDamage=11, …) | ELIMINA (clases reales del fork) | P2 |
| `raven_ammo.h` | Offset `m_rgAmmo` descubierto por save-fields | ELIMINA (`ammoregistry`/`ammo_amounts` Featureful) | P2 |
| `entity_handle.h` | Handle edict+serial anti-reciclaje | ELIMINA (`EHANDLE` nativo) | P2 |
| `czero_animation_validation.inc` | Capturas de poses CZ (env `CREMATION_CZERO_ANIMATIONS`) | ELIMINA | P2 |
| `czero_validation.inc` | Probe CZ (`CREMATION_CZERO_VALIDATE`) | ELIMINA | P2 |
| `czero_tactical_validation.inc` | Escenario integración (`CREMATION_TACTICAL_VALIDATE`) | ELIMINA | P2 |
| `david_validation.inc` | Fixtures David (`check_david_runtime.py`) | ELIMINA | P2 |
| `randy_validation.inc` | `monster_randy` vs `monster_fastzombie` | ELIMINA | P2 |
| `hotd2_death_validation.inc` | Fixtures muerte HOTD2 | ELIMINA | P2 |
| `enemy_combat_validation.inc` | Cobertura combate (`CREMATION_ENEMY_COMBAT_VALIDATE`) | ELIMINA | P2 |
| `heart_validation.inc` | Probe corazones (`CREMATION_HEART_VALIDATE`) | ELIMINA | P2 |
| `quake_zombie_validation.inc` | Probe Quake (`CREMATION_QZ_VALIDATE`) | ELIMINA | P2 |
| `judgment_validation.inc` | Fixture boss + puertas | ELIMINA | P2 |
| `judgment_probe_poses.h` | Poses SMD generadas para el probe | ELIMINA | P2 |
| `mansion_rules_validation.inc` | Checks pickups/respawn | ELIMINA | P2 |
| `mansion_tactical_validation.inc` | Fixture siege/nav/combate | ELIMINA | P2 |
| `tactical_hud_validation.inc` | Probe HUD (`CREMATION_HUD_VALIDATE`) | ELIMINA | P2 |
| `tactical_save_validation.inc` | Probe save/load (`CREMATION_SAVE_PROBE`) | ELIMINA | P2 |
| `reactor_validation.inc` | Fixture reactor (`CREMATION_REACTOR_VALIDATE`) | ELIMINA | P2 |
| `reactor_preview.inc` | Cámaras screenshots (`CREMATION_REACTOR_PREVIEW`) | ELIMINA | P2 |
| `dmc_preview.inc` | Screenshots artefactos (`CREMATION_DMC_PREVIEW`) | ELIMINA | P2 |
| `truth_validation.inc` | Review exterior Truth (`CREMATION_TRUTH_VALIDATE`) | ELIMINA | P2 |
| `vostok_validation.inc` | Checks cover (`CREMATION_VOSTOK_VALIDATE`) | ELIMINA | P2 |

Notas: `monster_randy/max/ken/johnny/steve` son actores David ya exportados (ver `RANDY/KEN/MAX/STEVE/JOHNNY_ZOMBIE.md`); `monster_minion` (nigromante), `monster_kuarl/zeal`, `monster_tpose`, `monster_quake_zombie`, `monster_czero_zombie`, `cremation_heart`, `cz_planted_bomb`, `dmc_player_state`, `cz_flash/smoke_grenade`, `cremation_sentry`, `cremation_johnny_axe` deben registrarse en `templates/entities.json` + `fgd/`. Sonidos `cremation/**` y modelos `models/cremation/**`, sprites `sprites/cremation/**` se reutilizan tal cual vía precache en JSON. Toda la familia `*_validation.inc`/`*_preview.inc` y `forwarders*.inc`/`ravenholm_abi.h`/`raven_ammo.h`/`entity_handle.h`/`hecu_alliance.h` desaparece con el proxy.

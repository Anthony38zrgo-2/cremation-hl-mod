# Análisis comparativo: Cremation original vs rewrite Featureful

Fecha: 2026-10-29. Contexto: el rewrite (rama `rewrite-featureful`) sustituyó el
proxy `cremation.dll` sobre Ravenholm por un fork Featureful. El jugador reportó:
comportamientos perdidos, armas sin comportamiento original y mapa incorrecto.
Este documento compara con evidencia qué se perdió y por qué.

## Veredicto

El rewrite validó la arquitectura (fork compila, engine real, JSONs válidos)
pero **no portó el gameplay de Cremation**: los enemigos quedaron como
subclases vanilla con `DefaultModel` (skin, sin comportamiento), las armas
quedaron como 6 armas configurables genéricas re-skineadas, y los mapas
fueron reescritos con `monstermaker` perdiendo el director de oleadas.
≈80% del contenido bespoke no existe en el rewrite.

## 1. Enemigos: original vs rewrite

| Sistema | Original (proxy Ravenholm) | Rewrite actual |
|---|---|---|
| Director de oleadas | `mansion_waves.inc` (~108 KB): 40/40/80/80, composición por sector, rutas `mansion_route_*`/`mansion_ct_*`, asedio a puertas, scientist señuelo, drop RPG, estado en entvars | `monstermaker` inyectados en el BSP; roaming en nodos; sin rutas/asedio/señuelo; oleadas fijas por mapa, sin flujo campaña |
| David/Steve/Johnny/Max/Ken/Randy | Modelos HOTD2 + lógica propia: variantes (cadenas, hacha Johnny, Ken 2x daño, Randy armadura), carrera David por ocupación, hitboxes, carbonizados, voces, gibs | Solo `DefaultModel` (max/randy); David/Steve/Johnny/Ken **no existen**; IA = CZombie vanilla |
| Quake zombie | Reglas Quake: 60 destruye/25 derriba/regen, proyectil carne, 5s suelo, regeneración con acumulación plasma en armorvalue | Clase con modelo Quake; IA vanilla; reglas ausentes |
| CZ infiltrador | Rutas físicas (puertas/escalera/ventana), smoke+flash con LoS, Glock tras plantar, defensa en cobertura, defuse con kit 5s | Grunt con modelo knife + bomba simplificada (v1: planta y detona; sin kit/defensa/smoke/flash/rutas) |
| Tpose | Vuelo propio, bola de energía, muerte tipo minion | Subclase de Controller con `visuals` rojos |
| Cultistas/HECU | Classify 7 alianza con zombies, voces custom, 12 HECU rapidcore con variantes | Template `classify: Alien Monster` (solo stats) |
| Política de combate | `enemy_combat_policy.inc`: R_AL entre enemigos, bloqueo de fuego amigo con owner-chain, limpieza de m_hEnemy | **Ausente** (depende solo de classify nativo) |
| Radar/HUD táctico | Servidor+cliente con mensaje propio | **Ausente** |

## 2. Armas: original vs rewrite

| Arma original | Comportamiento original | Rewrite |
|---|---|---|
| Ricochet (disco) | Física de disco con rebote, freeze, 4 cargas | **No existe** |
| Bounce shotgun (flak) | Perdigones rebotantes + granada de contacto | **No existe** (weapon_shotgun2 = escopeta genérica 8 pellets) |
| Link gun | Primaria/secundaria continua, plasma cliente-servidor con muzzle proyectado por matriz de huesos, charred | **No existe** |
| Shock rifle | Combo cargado, orbe, hitscan+proyectil | Vanilla OpFor shockrifle con modelo (sin combo) |
| Necromancer | Invoca minion, detona minion, fuego 12/s | **No existe** |
| Nailgun | Proyectil clavo propio | Vanilla nailgun con modelo |
| Gravity gun (ggun) | Agarre/lanzamiento físico | **No existe** |
| Deagle/AWP/Winchester | Zoom, sonidos, presentación, tiempos propios | Re-skins: `weapon_eagle`/`weapon_sniperrifle2`/`weapon_shotgun2` genéricos |
| Pipewrench | Carga propia | Vanilla pipewrench con sonidos |
| TFC sentry, DMC powerups, UT2004 RPG | Sistemas completos | **Ausentes** |

## 3. Mapas

- **cs_mansion original**: el director spawneaba todo desde código; los 543
  `info_target` (`mansion_route_*`, `mansion_side_*`, `mansion_ct_*`) eran
  waypoints de las rutas. El mapa intacto + código = comportamiento.
- **Rewrite**: BSPs reescritos con 40 `monstermaker` (873 entidades vs 748).
  Los waypoints quedan sin uso; spawning/camino/campaña cambian. → "el mapa no
  es correcto". Los backups en `backups/rewrite-maps/` conservan los originales.

## 4. Causa raíz

El objetivo "rewrite de corrido" se ejecutó como **spike de arquitectura**:
probar que el fork compila, corre en el engine y acepta JSONs. Para lograrlo
en el tiempo disponible se priorizó "que arranque y sea jugable", no paridad
de gameplay. Las clases bespoke no se portaron; solo bindings de modelo.
La conclusión es que el port de gameplay bespoke es trabajo de semanas, no de
una sesión, y debe hacerse **incrementalmente, sistema a sistema**, con el
juego siempre jugable.

## 5. Restauración y camino correcto

- **Restaurado**: el estado original jugable (rama `featureful-merge`:
  `cremation.dll` proxy 896 KB + `hl_ravenholm.dll`, mapas prístinos,
  skill/delta/liblist originales) — verificado con `check_server_startup.py`.
- **Preservado**: todo el rewrite en la rama `rewrite-featureful`
  (fork `source/featureful/`, herramientas, bomba CZ v1, 6 armas templates,
  docs). No se borra nada.
- **Camino recomendado (paridad real)**: portar al fork un sistema por
  iteración, extrayendo la lógica de cada `.inc` original como métodos de
  clases reales del fork (no proxies):
  1. Director de oleadas como módulo del fork (routes+asedio+scientist).
  2. Familia David (modelos+variantes+reglas).
  3. Quake/CZ/Tpose reglas completas.
  4. Política de combate como gamerules/relaciones.
  5. Armas bespoke como `CConfigurableWeapon`-derived con templates.
  6. HUD/radar/journal Featureful.
  Cada paso: compilar → smoke en engine → aceptación → commit.

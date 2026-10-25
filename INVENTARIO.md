# Inventario de Cremation

## Armas habilitadas

Las ocho armas forman parte del equipo inicial de Rapidcore.

| Arma | Identificador | Funcionamiento |
|---|---|---|
| Shock Rifle / ASMD de UT99 | `weapon_shockrifle` | Rayo instantáneo, esfera violeta y combo al dispararle a la esfera. |
| Flak Cannon | `weapon_bounce_shotgun` | Perdigones que rebotan y granada secundaria con fragmentación. |
| Ricochet / Panthergun | `weapon_ricochet` | Disco azul que atraviesa y ralentiza enemigos, con rebotes en paredes; secundario guiado que explota al contacto. Cuatro cargas regenerables. |
| Necromancer | `weapon_necromancer` | Invoca un esqueleto aliado; el secundario lo detona y deja fuego. |
| Motosierra / Pipe Wrench | `weapon_pipewrench` | Aspecto y sonidos de motosierra, con ataques cuerpo a cuerpo basados en la palanca. |
| Plasma Rifle / Link Gun | `weapon_nailgun` | Plasma verde primario y enlace secundario con daño creciente. Es la evolución de la antigua Nailgun, no otra arma adicional. |
| Granada de mano / TNT | `weapon_handgrenade` | Explosión temporizada y fuego durante cinco segundos; aspecto de TNT. |
| RPG | `weapon_rpg` | Un misil primario; secundario carga hasta tres para lanzarlos juntos. Reserva máxima de 30. |

## Enemigos presentes en Rapidcore

52 enemigos iniciales, repartidos en cinco tipos.

| Enemigo | Cantidad | Identificador y características |
|---|---:|---|
| Zombi normal morado | 10 | `monster_zombie`; modelo morado y comportamiento nativo de Ravenholm. |
| Zombi de Quake | 10 | `monster_quake_zombie`; lanza restos, se derriba y regenera daño ordinario. |
| Zombi rápido | 10 | `monster_fastzombie`. |
| Zombi venenoso | 10 | `monster_poisionzombie`; esa es la escritura real de su clase. |
| Cultista Blood / HECU | 12 | `monster_human_grunt`; cuatro cabezas y seis combinaciones de armamento. |

Los cultistas son aliados de los zombis y enemigos del jugador. Hay dos soldados por combinación:

- Subfusil.
- Escopeta.
- Subfusil y granadas.
- Escopeta y granadas.
- Subfusil y lanzagranadas.
- Escopeta y lanzagranadas.

## Aliados y construcciones

| Elemento | Disponibilidad |
|---|---|
| Esqueleto del Necromancer | Aliado invocable, `monster_minion`. |
| Sentry TFC de nivel 1 | Torreta construible aliada. B construye, N desmonta y J repara/reabastece. |

## Armas implementadas y deshabilitadas

| Arma | Identificador | Estado |
|---|---|---|
| Escopeta simple TFC | `weapon_shotgun` | Recursos y lógica incluidos; recogida, selección y disparo deshabilitados mediante `ShotgunsPlayable=false`. |
| Winchester 1887 | `weapon_winchester1887` | Recursos y lógica incluidos; recogida, selección y disparo deshabilitados mediante `ShotgunsPlayable=false`. |

## Entidades heredadas de Ravenholm

Entradas expuestas por la DLL que no forman parte de los 52 enemigos iniciales. Su disponibilidad fuera de Rapidcore no está comprobada en juego.

### Criaturas, enemigos y unidades

- Headcrab normal, rápido, venenoso y pequeño.
- Alien Controller, Alien Grunt, Alien Slave y Vortigaunt.
- Bullsquid y Houndeye.
- Barnacle, Gargantua, Gonarch y Nihilanth.
- Tentacle y Tentaclemaw.
- Asesina humana y soldados descendiendo por cuerda.
- Metrocop y escáner.
- Apache y Osprey.
- Ichthyosaur, leech y Snark.
- Torretas nativas: miniturret, sentry y turret.
- Entidades auxiliares: Bloater, Mortar, fauna, personajes amistosos, cadáveres y actores de escenas.

### Entradas de armas base

Entradas de compatibilidad; no constituyen un segundo arsenal confirmado. Varios identificadores sirven de soporte interno para armas personalizadas.

| Entrada | Identificadores |
|---|---|
| Pistola | `weapon_9mmhandgun`, `weapon_glock` |
| MP5 | `weapon_9mmAR`, `weapon_mp5` |
| Revólver .357 | `weapon_357`, `weapon_python`; bloqueados por la deshabilitación de Winchester. |
| Ballesta | `weapon_crossbow` |
| Palanca | `weapon_crowbar` |
| Egon | `weapon_egon` |
| Gauss | `weapon_gauss` |
| Hornetgun | `weapon_hornetgun` |
| Gravity Gun | `weapon_ggun` |
| PR | `weapon_PR`, `weapon_pr` |
| Carga explosiva remota | `weapon_satchel` |
| Mina láser | `weapon_tripmine` |
| Snarks | `weapon_snark` |

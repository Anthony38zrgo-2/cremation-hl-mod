# Escopeta de rebote (`weapon_bounce_shotgun`)

Arma independiente de Cremation. Reutiliza los modelos de la escopeta de Ravenholm
bajo nombres propios, pero es otra entidad de inventario, con otra ranura de HUD,
otro cargador y sus propios ataques. No sustituye ni la escopeta normal ni Ricochet.

Este documento registra lo implementado y lo comprobado. La validación visual y de
sensación en el juego sigue siendo manual y está pendiente.

## Identidad de inventario (portador interno)

| Elemento | Valor |
|---|---|
| Entidad del mapa / clase | `weapon_bounce_shotgun` |
| Portador nativo | `weapon_crossbow` (ID 6), sin uso en Rapidcore |
| Banda del HUD | 2 (0-based) = grupo 3, posición 3 (mp5=0, escopeta=1, ballesta=2) |
| ID de HUD | 6, el del portador, para que el bit de arma (`pev->weapons`) y la selección funcionen |
| Munición mostrada | índice 20 (cartuchos) e índice 21 (granadas AR), exclusivos del cliente |
| Cargador | 8 cartuchos, reserva inicial 40, granadas iniciales 6 |
| Vista / tercera persona / mundo | `models/cremation/v_bounce_shotgun.mdl`, `p_…`, `w_…` |

El nombre visible llega al cliente reescribiendo el mensaje `WeaponList`: el motor
anuncia la ballesta con el nombre de su clase, y Cremation lo sustituye por
`weapon_bounce_shotgun` y por los ocho bytes de identidad (munición, banda, posición,
id). El comando que envía el HUD es exactamente el nombre de clase, de modo que
`CBasePlayer::SelectItem` lo encuentra sin alias.

Como el portador es la ballesta, su `CCrossbow::Spawn` no cambia el `classname` de la
entidad; tras el spawn se sobrescriben vista, modelo de jugador y modelo de mundo.
Los ataques, la recarga y el zoom heredados se filtran limpiando los botones antes de
`PreThink`/`PostThink`, igual que hace Ricochet con la pistola.
Además, se mantiene `cl_lw 0` en la configuración guardada y se envía al entrar el
jugador: con predicción activa, el cliente heredado podía ejecutar el zoom de la
ballesta. Los mensajes `SetFOV`, el FOV público del jugador y sus datos de red se
normalizan sólo mientras está activa esta arma, sin acceder a campos privados.

## Primario: 12 perdigones físicos con rebotes

- 12 perdigones (rama de un jugador de `CShotgun::SecondaryAttack`), cono de
  referencia `0.08716` ampliado un 15% por petición posterior: `0.100234` por eje.
- La semilla es la que el motor entrega por `pfnCmdStart` para ese usercmd: la misma
  semilla compartida que usaría la escopeta donante. Se guarda por jugador y se
  captura al disparar.
- Desplazamientos: dos muestras `UTIL_SharedRandomFloat` por eje con las semillas
  `seed+i`, `seed+1+i`, `seed+2+i`, `seed+3+i`, exactamente como
  `FireBulletsPlayer`. La implementación del generador y su tabla de 256 enteros
  están portadas literalmente de `hlsdk/dlls/util.cpp`.
- Dirección: `normalize(forward + right*(x*cone) + up*(y*cone))`, con la base de
  apuntado `v_angle + punchangle` del disparo. No se aplica corrección de autoaim
  (ver «Diferencias y límites»).
- Velocidad 1000 u/s, sin gravedad. Traza continua del segmento completo en cada paso.
- Contactos con superficie: el primero y el segundo reflejan
  (`v' = v - 2*dot(v,n)*n`, conservando el módulo) y el proyectil sigue volando; el
  tercero lo retira sin rebotar, sin explosión y sin daño radial.
- Contacto con un enemigo vivo, antes o después de rebotar: aplica **un perdigón**
  una sola vez y retira el proyectil. Nunca atraviesa ni rebota en un enemigo.
- Objetos rompibles con `takedamage`: reciben el impacto una vez y consumen el
  perdigón. Agua, triggers, entidades no sólidas y otros proyectiles no consumen rebotes.
- Daño: `1.5 * sk_plr_buckshot{1,2,3}` (7.5 de base actualmente) con `DMG_BULLET` y el
  multiplicador nativo de hitgroup de monstruo (`sk_monster_head` = 3, resto 1).
- Límites: 96 perdigones activos de esta arma (Ricochet conserva sus 24), 4 segundos
  de vida, 8 iteraciones por paso para esquinas. Si no cabe la descarga completa no se
  gasta munición ni se lanza una descarga parcial; tampoco si el motor rechaza alguna
  de las doce entidades, en cuyo caso se retiran las ya creadas. Sin cartuchos suena
  un clic seco en vez de recargar en el vacío.
- Presentación: disco de Ricochet a escala 0.35, color cálido (nada de azul) y estela
  corta.

## Secundario: granada de contacto con daño por elevación

- Una granada por disparo, 800 u/s, gravedad 0.5 (`CGrenade::ShootContact`), sin
  sumar la velocidad del jugador. Se lanza desde la boca del arma, apartada de la pared.
- Detona en el primer contacto sólido, también contra un enemigo. No rebota.
- La elevación se captura **al disparar**, antes del retroceso de ese disparo:

```text
elevation_deg = clamp(-(v_angle.x + punchangle.x), 0, 90)
t             = elevation_deg / 90
Dmin          = 0.25 * Dmax
Dshot         = clamp(Dmin + (Dmax - Dmin) * t, Dmin, Dmax)
```

  `Dmax` se lee de `sk_plr_9mmAR_grenade{1,2,3}` (100 en este `skill.cfg`), nunca está
  fijado en el código. Mirar arriba después de lanzar, cambiar de altura o alargar el
  vuelo no altera el daño.
- Radio fijo `2.5 * Dmax` (250 con el valor actual) y caída radial lineal, con la misma
  regla de oclusión y de agua que `RadiusDamage` de `combat.cpp`.
- El daño se resuelve una sola vez con `DMG_BLAST`; no se suma daño directo al enemigo
  alcanzado, así que el total no supera el del lanzagranadas.
- Empuje pequeño, independiente del ángulo y del daño: 140 horizontal y 35 vertical por
  `falloff = clamp(1 - distancia/radio, 0, 1)`, durante 0.5 s. Reutiliza el mecanismo de
  empuje de Ricochet (física temporal de caída y supresión del avance de la IA), pero con
  magnitudes propias; el empuje de Ricochet no cambia.
- La explosión propia no daña al jugador: es la regla actual de Cremation y se mantiene.
- Vida máxima de seguridad: 6 segundos; una granada que no encuentre un sólido detona
  igualmente.

## Cadencia, munición y recarga

- Primario 1.5 s y 2 cartuchos; secundario 1 s y 1 granada. Cualquier disparo bloquea
  también el otro modo para que no salgan a la vez.
- Recarga: cuando quedan menos de 2 cartuchos, el primario inicia una recarga de 1.2 s
  que llena el cargador desde la reserva (animación `start_reload`/`reload`).
- El runtime heredado no ofrece ninguna vía soportada para escribir
  `CBasePlayer::m_rgAmmo`, así que esta arma lleva su propio cargador y su propia reserva
  y los muestra con `CurWeapon` y `AmmoX` dirigidos a los índices 20 y 21. No se toca la
  reserva real de cartuchos de la escopeta normal. Es la desviación más importante
  respecto de la propuesta inicial y está anotada abajo.

## Valores definitivos (`bounce_shotgun_balance.h`)

| Parámetro | Valor |
|---|---|
| Perdigones / cono | 12 / 0.08716 |
| Velocidad del perdigón | 1000 u/s |
| Contactos con superficie | 2 rebotes; retirada en el tercero |
| Vida del perdigón / límite | 4 s / 96 activos |
| Cadencia primaria / coste | 1.5 s / 2 cartuchos |
| Cargador / reserva inicial | 8 / 40 |
| Granada | 800 u/s, gravedad 0.5, radio 250, vida 6 s |
| Curva de daño | 25 % de `Dmax` + 75 % × (elevación/90) |
| Empuje | 140 horizontal, 35 vertical, 0.5 s |
| Cadencia secundaria / coste | 1 s / 1 granada |
| Granadas iniciales | 6 |

## Archivos

| Archivo | Responsabilidad |
|---|---|
| `source/bounce_shotgun.cpp` / `.h` | Módulo: proyectiles, daño, granada, cargador, mensajes de munición. |
| `source/bounce_shotgun_balance.h` | Todos los valores del arma, separados de Ricochet. |
| `source/ricochet_proxy.cpp` | Inventario, HUD, hooks de motor, factorías y modelos. |
| `tools/install_bounce_assets.py` | Copias de modelos, icono HUD y descriptor; `bounce-assets.json`. |
| `tools/build_mod.py` | Instala recursos y añade la entidad al equipo inicial del mapa. |
| `tools/verify_mod.py` | Exige las tres armas, recursos propios y exportaciones. |
| `source/test_bounce.cpp` | Pruebas del módulo con motor simulado. |
| `source/test_ricochet.cpp` | Pruebas de Ricochet y de la integración HUD del arma nueva. |

## Pruebas ejecutadas

`build_ricochet.ps1` ejecuta, en este orden: generación de exportaciones, pruebas de
Ricochet, pruebas de la escopeta de rebote, compilación Win32, finalización de nombres
en `.rdata`, instalación de recursos, mapa, carga nativa y verificación estática.

- `test_bounce.cpp`: muestreador idéntico a `UTIL_SharedRandomFloat` con valores de
  referencia independientes; 12 perdigones con la misma velocidad y direcciones
  capturadas; dos reflexiones con módulo conservado y retirada en el tercer contacto;
  daño de un perdigón aplicado una vez y ×3 en cabeza; presupuesto sin descargas
  parciales; vida máxima; curva 25/25/50/75/100 con el daño congelado al disparar;
  mismo ángulo a distinta altura; caída radial, oclusión por pared, sin daño propio,
  empuje pequeño e independiente del daño; arco y detonación única; HUD, gasto y
  recarga.
- `test_ricochet.cpp`: regresiones de Ricochet más la reescritura exacta de
  `WeaponList`/`CurWeapon` del arma nueva y la conservación de los bytes de la escopeta.
- `test_native.cpp`: carga real de la DLL Win32, API 140 y constructor de zombi.
- `verify_mod.py`: las tres armas existen por separado en el BSP, el modelo de la
  escopeta normal no se sobrescribe, el sprite y el descriptor son válidos, las
  exportaciones están completas y el lector antiguo de GoldSrc resuelve cada nombre.
- `check_server_startup.py`: el motor real carga la DLL, `worldspawn` y
  `ServerActivate rapidcore`, y sigue vivo ocho segundos.

## Diferencias y límites conocidos

1. **Sin autoaim en el primario.** `GetAutoaimVector` es un método privado de
   `CBasePlayer` y no está exportado; la base de apuntado es `v_angle + punchangle`.
   La distribución y el orden de semillas sí son los de la escopeta donante.
2. **Daño con `TakeDamage`.** `TraceAttack` de los monstruos solo acumula en la lista
   multi-damage del DLL, que este proxy no puede vaciar. Se aplica el multiplicador de
   hitgroup y se llama a `TakeDamage` (ranura 11). El daño final coincide, pero se
   pierden los efectos de sangre/decales del `TraceAttack` nativo y cada perdigón se
   aplica por separado en vez de agregarse en un solo impacto.
3. **Munición propia.** Ver arriba: no se descuenta la reserva nativa de cartuchos.
4. **Granada.** Se reproduce el arco, la oclusión y la caída radial; no se generan los
   decales de quemadura ni las chispas del `CGrenade::Explode` original.
5. **La elevación incluye el retroceso acumulado** (`punchangle`), porque forma parte
   del apuntado visible en el momento del disparo. El nuevo retroceso se aplica después.
6. **Selección de Ricochet.** El proxy traducía el comando `weapon_ricochet` a
   `weapon_9mmhandgun`, pero `SelectItem` compara el *classname* de la entidad, que es
   `weapon_ricochet`; la traducción impedía seleccionarla con las teclas del HUD. Se ha
   eliminado el alias para que las tres armas se seleccionen por su nombre real. Los
   ataques, el grupo 2, Freeze Shot y Triple Shot no cambian.

## Reproducción

```powershell
# desde la raíz de Half-Life
& .\Cremation\tools\build_ricochet.ps1
python .\Cremation\tools\check_server_startup.py
```

Evidencias: `validation-bounce.txt`, `server-startup-result.json`,
`server-startup-check.txt`, `bounce-assets.json`, `file-manifest.json`.

## Pendiente de comprobación manual

- Aspecto del arma en primera y tercera persona y animaciones de disparo, recarga y
  despliegue sobre `v_bounce_shotgun.mdl`.
- Icono, posición en el grupo 3 y contadores de munición en el HUD real.
- Sensación de los rebotes (velocidad, estela, sonido) y legibilidad de los perdigones.
- Curva de daño de la granada y empuje sobre los zombis (incluido si la IA lo respeta
  en escaleras y agua).
- Alternancia entre las tres armas, guardados y cambio de mapa sin proyectiles huérfanos.

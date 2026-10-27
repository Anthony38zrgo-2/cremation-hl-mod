# Plan de refactorización de Cremation

Estado: plan basado en el código local del 26 de octubre de 2026. Las fases 0–1 están implementadas; la fase 2 tiene sus primeras bases ABI y de handles, pero aún no introduce contexto de motor. El progreso y las comprobaciones de esta ejecución están en [REFACTOR_PROGRESS.md](REFACTOR_PROGRESS.md). La regla de combate entre enemigos ya está implementada.

## Objetivo y problemas observados

El servidor conserva `ricochet_proxy.cpp` como punto de entrada, pero ese archivo incluye gran parte de la implementación mediante `.inc`. Hay 46 archivos `.inc` en el directorio `source`, y 13 de ellos se llaman `*validation.inc`; también existen fixtures en otros archivos. El servidor mezcla armas, adaptación de monstruos, efectos, precache, guardado, lógica de mapas, pruebas y exportación del donante. `bounce_shotgun.cpp` incluye además `tfc_gameplay.inc`.

`david_zombie.inc` reúne la familia David/Steve/Johnny: selección de modelos, animación, daño, anatomía, guardado, carrera, lanzamiento de hachas y defensa. La acumulación de estados ha permitido que una corrección de muerte interfiera con otra animación.

Las tablas virtuales se modifican en varios módulos. `weaponBloodInstall`, `humanGibInstall`, `davidInstall`, `qzInstall`, `czInstall`, `cultistInstall` y `necroInstall` dependen de su orden de instalación y de búsquedas por identidad de tabla. Duplicar o mover esos hooks sin un contrato puede perder un adaptador o introducir recursión.

Los scripts `build_ricochet.ps1`, `build_mansion.ps1`, `build_david_zombie.ps1`, `build_quake_zombie.ps1` y `build_client_effects.ps1` repiten partes de la compilación/instalación. El nombre de algunos scripts ya no describe todo lo que compilan.

El código generado debe distinguirse del escrito a mano: `forwarders.inc`, las cabeceras de huesos y navegación, los modelos y sus informes tienen generadores propios. Un archivo con pocas líneas puede contener una gran tabla generada; dividirlo manualmente no mejora su mantenimiento.

## Estructura de destino

```text
src/
  engine/
    api/                  contexto y tipos del motor
    abi/                  donante, firmas, slots y metadatos verificados
    hooks/                registro y composición de adaptadores
    exports/              exportaciones y reenvío generados
    entities/             handles con serial y resolución de propietarios
    save/                 versiones y migraciones
  server/
    combat/               facciones, atribución, daño y estados temporales
    actors/
      hotd2/              familia común, anatomía, animaciones y guardado
      johnny/             hachas y defensa
      quake/              estados y proyectiles
      czero/              combate, navegación y objetivo de bomba
      cultist/            adaptación HECU y voces
    weapons/              un módulo por arma o familia real
    effects/              sangre, gibs y efectos de muerte
    gameplay/             oleadas, reactor, objetos y poderes DMC
  client/
    rendering/            boca del arma, proyección y efectos
    hud/                  radar y HUD táctico
    audio/                música
  generated/              resultados generados; nunca edición manual
tests/
  unit/
  engine/                 fixtures y ejecutores HLDS
tools/
  build/
  assets/                 extracción, conversión, instalación y auditoría
docs/
  architecture/
  features/
```

Las rutas de assets que usa el juego se mantienen durante la migración. Esta estructura describe el código fuente y las herramientas, no un cambio de nombres de modelos, entidades o mapas.

## Fases y criterios de aceptación

| Orden | Trabajo | Resultado verificable |
|---|---|---|
| 0 | Inventariar funciones, features, hooks, estados, exports y consumidores dinámicos; registrar la DLL y assets de referencia | Cada subsistema tiene propietario y estado: activo, compatibilidad, prueba, temporalmente desactivado o retirado |
| 1 | Separar pruebas y previews del código de producción; corregir expectativas de pruebas antiguas y retirar código demostrado inalcanzable | La DLL de producción no compila funciones de fixtures; el target de validación conserva su cobertura; ninguna función retirada tiene consumidores |
| 2 | Crear un contexto explícito de motor y un módulo de ABI/handles | Slots, RVA, firmas y resolución de entidades no se repiten en actores y armas; los handles validan su serial |
| 3 | Unificar la instalación de hooks y el flujo de daño | Un adaptador se registra una sola vez, recibe su continuación explícita y funciona tras spawn/restore; no se desenvuelven manualmente tablas de otros módulos |
| 4 | Dividir la familia HOTD2 y sus máquinas de estados | Animación, vida, anatomía, hachas y guardado tienen interfaces separadas; una muerte interrumpe acciones exactamente una vez |
| 5 | Extraer Quake, CZ, cultistas, oleadas, armas y efectos a unidades `.cpp` | El arranque contiene registro de módulos y callbacks; desaparece la dependencia del orden textual de `.inc` |
| 6 | Separar cliente, generadores y documentación; unificar compilación e instalación | Targets x86 de servidor, cliente, pruebas y assets independientes; instalar la DLL no ejecuta el pipeline global de modelos |
| 7 | Auditoría final de código muerto, compatibilidad y dependencias | Sin funciones retiradas, duplicados de fixtures ni rutas obsoletas; exports y partidas de referencia pasan las pruebas acordadas |

Cada fase debe poder compilarse, probarse e instalarse por separado. Los cambios de comportamiento se separan de los movimientos de archivos para que una regresión pueda localizarse y revertirse con precisión. No se mezclan estas fases con los cambios locales previos de modelos, armas o mapas.

## Contratos que deben introducirse primero

1. **HookRegistry.** Distinguir tablas nativas y adaptadas, guardar originales una sola vez y componer funciones de daño/relación sin depender del puntero actual de la tabla. Las firmas verificadas del donante pertenecen al módulo ABI.
2. **EntityHandle.** Entidad y serial juntos; resolver objeto privado, propietario y atacante desde un único lugar. Extraer la búsqueda de metadatos `m_hEnemy`, hoy alojada en el módulo de voces de cultistas.
3. **DamageContext.** Víctima, atacante acreditado, inflictor, tipo, cantidad y origen del ataque. Rechazar fuego amigo antes de las reacciones; conservar escalas DMC/Steve, daño nativo, muerte, anatomía y efectos en un orden documentado y con una sola llamada al donante.
4. **ActorLifecycle.** `Spawn`, `Think`, `Frame`, `Save`, `Restore`, `Killed` y `Precache` registrados explícitamente. Una misma acción no se inicia desde varios callbacks sin una transición de estado común.
5. **AnimationController.** Una secuencia tiene propietario, prioridad y finalización. La muerte se elige una vez; se distingue terminar un fotograma de terminar con una pose tumbada. Los eventos de impacto/lanzamiento se consumen una sola vez.
6. **SaveSchema.** Los nombres y versiones existentes se migran expresamente. Los campos históricos que aún leen partidas antiguas no se confunden con funciones sin uso.

## Eliminación de funciones y código que ya no se utilizará

La eliminación forma parte obligatoria de las fases 1 y 7. La búsqueda inicial de referencias en los archivos propios `.cpp/.h/.inc` no encontró funciones `static` evidentes con una única aparición. Ese resultado es un filtro textual, no una prueba de que todo el código sea necesario: hay caminos desactivados, consumidores dinámicos y pruebas incluidas en la misma unidad de compilación.

Antes de retirar cada función se comprobará su alcance desde callbacks activos, tablas de funciones, exports, entidades de mapas, eventos de modelos, serialización y scripts. Si no pertenece a una funcionalidad conservada y no tiene consumidores, se elimina la implementación junto con declaraciones, registro de hooks, llamadas de precache, datos y pruebas que exclusivamente ejercían esa funcionalidad retirada. No se sustituye por stubs vacíos ni se deja comentada.

| Hallazgo concreto | Acción prevista | Prueba de que puede retirarse o moverse |
|---|---|---|
| Funciones de `*validation.inc`, `hotd2_death_validation.inc`, `enemy_combat_validation.inc`, previews y helpers `*Probe*` | Sacarlas de la DLL de producción; conservar las pruebas útiles en un target de validación. Borrar fixtures duplicados o que ya no prueban un comportamiento existente | Grafo de referencias de la build de producción y ejecución de los fixtures conservados |
| Ramas `prone_attack_*`, `prone_*`, `crawl*` y `crawl_idle` del conversor de Johnny | Eliminar las ramas copiadas que la lista `SEQS` de Johnny nunca puede seleccionar; retirar sus helpers si quedan sin consumidores | `SEQS` contiene 21 secuencias sin esos nombres; comparar hashes de los modelos generados antes/después |
| Prueba de serialización en `david_validation.inc` que exige versión 3 mientras `DavidSaved` usa versión 5 | Actualizar el contrato de la prueba y añadir casos explícitos de migración; retirar únicamente expectativas obsoletas | Round trip de versión actual y lectura de los formatos antiguos admitidos |
| Implementaciones de escopetas detrás de `ShotgunsPlayable=false` | Registrar que el propio código las considera temporalmente desactivadas. La retirada definitiva debe formar parte de una decisión de producto, con eliminación completa de su registro y assets exclusivos | Inventario de features y referencias; comprobar que no se lleva consigo la sentry/ingeniero de `tfc_gameplay.inc` ni APIs compartidas |
| Campo `prone` histórico, migraciones y secuencias numeradas de modelos | Mantener la compatibilidad necesaria; retirar solo cuando exista una migración y una política explícita de versiones de guardado | Carga de partidas antiguas y readback de secuencias/eventos |
| Exports y forwarders generados | Conservar los requeridos por GoldSrc, mapas y donante aunque no tengan llamadas C++ visibles | Comparación del listado de exports y arranque real del motor |

No se usará `rg` sin resultados como único criterio para borrar un export, un callback o una función registrada por puntero. Tampoco se borrarán datos fuente de extracción por no aparecer en el código de juego: forman parte de la reproducción de los assets.

## Compilación y comprobaciones por etapa

- Introducir targets MSVC Win32 con una configuración común; definir `CREMATION_ENABLE_VALIDATION` solo en la DLL de pruebas. Los ejecutores HLDS actuales apuntarán a esa build explícita.
- Conservar la finalización de nombres de exportación en `.rdata` y la comparación del listado antes de instalar. Los scripts PowerShell actuales pasarán a ser entradas pequeñas de una única configuración de compilación.
- Mover una familia de archivos por paso y usar avisos de funciones no referenciadas y un análisis de llamadas como apoyo a la limpieza. Los resultados se contrastan con registros dinámicos y compatibilidad.
- Ejecutar pruebas asociadas al módulo modificado: relaciones/daño entre enemigos, muerte continua y poses finales, daño del jugador y aliados, propietario de proyectiles, guardado/carga, composición de oleadas y eventos de animación.
- Para cambios de assets, comparar geometría/materiales/alpha y comprobar las poses del MDL instalado; para el cliente, comprobar visualmente la proyección y los attachments en una partida.
- La aceptación final incluye una partida nueva de `cs_mansion`, carga de una partida anterior, cambio de mapa y cierre/reinicio del juego. No basta con que la DLL compile.

## Primer paso recomendado

Continuar la fase 2 desde los cimientos ya añadidos: terminar de centralizar los slots/RVA restantes y encapsular acceso al engine. Después extraer ABI/handles y el registro de hooks antes de dividir los actores. Mover inmediatamente todos los `.inc` a carpetas dejaría intactas las dependencias de orden que causan los problemas actuales.

La regla ya implementada en `enemy_combat_policy.inc` proporciona un punto de partida para el futuro módulo `server/combat`, con pruebas propias y una política explícita de atribución de daño.

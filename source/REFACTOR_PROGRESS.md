# Progreso de la refactorización

Fecha: 26 de octubre de 2026

El plan general y los criterios de aceptación están en [REFACTOR_PLAN.md](REFACTOR_PLAN.md). Se avanzó con cambios acotados que preservan el contenido local de mapas y assets.

## Fase 0: inventario

- Confirmado `source/ricochet_proxy.cpp` como entrada de servidor y que incorpora módulos mediante `.inc`.
- Inventariados 46 `.inc`, 13 `*validation.inc`, 10 `.cpp` y 17 `.h` propios (sin contar el SDK).
- Registradas las dependencias de orden de los adaptadores virtuales; por eso aún no se movieron masivamente archivos.

## Fase 1: producción separada de validación

- `tools/build_server.ps1` compila por separado `source/build/production/cremation.dll` y `source/build/validation/cremation.dll`; `-Install` instala únicamente producción.
- Se mantiene el mismo conjunto de 541 exports en ambas DLL. Las pruebas de fixture, previews, validadores y ramas exclusivas de previsualización quedan condicionadas por `CREMATION_ENABLE_VALIDATION`.
- Los ejecutores de pruebas de motor reemplazan temporalmente la DLL instalada por la variante de validación y la restauran en `finally`; rechazan el reemplazo si `hl.exe` o `hlds.exe` ya está activo.
- `tools/build_ricochet.ps1` conserva los tests de configuración, Ricochet, Bounce Shotgun, carga nativa y compilación del launcher. La auditoría estática de assets (`verify_mod.py`) es opcional con `-VerifyAssets`, de modo que compilar la DLL no depende de modificar mapas o volver a instalar todo el paquete.
- La expectativa obsoleta de la prueba David (versión 3 frente a versión actual 5) fue corregida. El test Ricochet ahora usa stubs para las consultas de claves de física del motor y valida que el secundario del Nailgun se reserve al Link Gun continuo. Se actualizó también el flag HUD de selección vacía de Bounce Shotgun.
- Se eliminaron del conversor de Johnny las ramas inalcanzables `prone_*`/`crawl*`; la generación actor-only conservó sin cambios los hashes de sus dos MDL instalados y del header de assets.

## Fase 2: base ABI y handles, parcial

- Añadido `source/ravenholm_abi.h` con nombres para slots, RVA y offsets verificados; varias referencias duplicadas de actores y armas ya usan esas constantes.
- Esta pasada también reemplaza índices vtable directos para `MyMonsterPointer`, `BloodColor` y el hook DMC de daño; los offsets del probe de animación David tienen nombres explícitos.
- Añadido `source/entity_handle.h`, que valida el serial y el estado de una entidad al resolverla.
- `enemy_combat_policy.inc` usa ahora el handle común y evita clasificar entidades que no estén registradas. No existe todavía un contexto de motor común, y quedan referencias ABI por migrar.

## Validación de esta ejecución

- La cadena `tools/build_ricochet.ps1` ejecutó correctamente los tests de configuración, Ricochet, Bounce Shotgun, carga nativa, compilación de DLL de producción y validación y launcher. Solo se mantiene la advertencia conocida de MSVC C4325 en `.rdata`.
- `tools/check_enemy_combat.py` pasó antes de la última migración de nombres ABI: nueve tipos de enemigos, 72 relaciones dirigidas y 360 impactos, con atribución de daño de jugador/proyectil preservada. Un reintento tras la pasada ABI terminó antes de cargar el mod: HLDS reportó que no pudo cargar `SDL3.dll` y cerró al inicializar Steam; por tanto el verificador de combate queda pendiente de una nueva ejecución con el runtime HLDS operativo.
- `tools/check_hotd2_deaths.py` pasó: seis escenarios de muerte continua y poses finales tumbadas para las variantes David/Steve/Johnny.
- Producción instalada en `dlls/cremation.dll`, igual byte por byte a `source/build/production/cremation.dll` (SHA-256 `EE8A9CF8D662A8318C0866ED312508C960E411E204840314EE308051D1CC3750`).
- `verify_mod.py` se probó por separado y no pasa con el mapa instalado actual: `rapidcore.bsp` no incluye cuatro objetos de loadout (`item_suit`, `weapon_ricochet`, `weapon_shotgun`, `weapon_bounce_shotgun`) que exige esa auditoría. No se ejecutó el pipeline `build_mod.py` porque reinstala/copía mapas y assets fuera del alcance de separar las builds; la opción `-VerifyAssets` conserva la comprobación para cuando el usuario actualice esos assets.

## Siguiente trabajo

Completar el inventario de ABI/RVA, añadir un contexto explícito del engine y después diseñar el registro de hooks con composición de continuaciones. Solo tras esos contratos se dividirán los actores HOTD2 y los demás módulos en unidades compiladas separadas. Las fases 3–7 siguen pendientes.

# David en Cremation

Implementado e instalado el 26 de octubre de 2026. Sustituye únicamente a `monster_zombie` por el modelo original de David de HOTD2. El proxy conserva el CZombie de `hl_ravenholm.dll` para percepción, rutas, combate, resistencias, salud y muerte general.

## Recursos y animaciones

El modelo tiene 16 huesos, 721 triangulos, 28 texturas y 26 secuencias. Incluye espera, caminar, correr, tres ataques, dolor, muertes y el arrastre original 0x3E7. El hull siempre mide 32x32x72; perder una pierna activa el arrastre sin bajar el cuerpo. Velocidades: 42 al caminar, 130 al correr y 16 al arrastrarse.


Las variantes heridas y muñones proceden del banco original. Manos, pies y cabeza pueden ocultarse completamente; una amputación proximal también oculta sus descendientes. El abdomen conserva las asignaciones originales de vértices entre torso y pelvis. La variante carbonizada comparte exactamente rig, secuencias y bodygroups. Las texturas conservan sus colores originales, incluidas las heridas verdes de esta edición de HOTD2.

La locomoción cambia de secuencia solo al cambiar entre caminar, correr y arrastrarse. El `MoveExecute` nativo se invoca en cada paso de ruta; volver a llamar `SetActivity` en cada paso reiniciaba el temporizador y entrecortaba la animación. Las tres animaciones de ataque quedan disponibles bajo la actividad de melee que realmente pide CZombie. Se seleccionan cíclicamente y cada animación acepta un solo evento de daño, evitando repetir el golpe si el motor vuelve a despachar el mismo evento.

El mapeo de dolor se contrastó con la tabla original de kind 7: cabeza 0x3D1, pecho 0x3D6, estómago 0x3CE, brazo derecho/izquierdo 0x3D5/0x3D3 y pierna derecha/izquierda 0x3C1/0x3C0. Los golpes usan ahora 0x3EA/0x3E9/0x3C9 del banco compatible de Steve, con contacto en los cuadros 21/27/20. Los clips anteriores 0x3F4/0x3F5 son embestidas de mordida. Detalles: `source/HOTD2_MELEE_FIX.md`. El daño de sus eventos sigue siendo el del zombie de Cremation.

El modelo usa múltiples bodygroups cuyo entero supera 255. `Cremation/delta.lst` amplía exclusivamente `entity_state_t.body` a 32 bits para transmitirlos íntegros al cliente. Los campos de jugador y entidades custom y todos los demás parámetros permanecen como en la tabla de Valve usada como base. No hace falta recompilar la DLL cliente: GoldSrc negocia las descripciones delta del servidor.

## Carrera según ocupación alrededor del jugador

El objetivo debe ser un jugador visible al que el zombie esté persiguiendo por su ruta nativa, a 128–768 unidades. Si existe otro enemigo vivo del jugador en un radio de 256 unidades alrededor del jugador, David camina. Se consideran todos los tipos hostiles, usando la relación nativa, con filtro de altura y paredes que separan las zonas. Los aliados, minions y sentries del jugador no cuentan.


## Daño localizado y muerte

El modo actual (`cremation_david_local_damage 0`) usa una sola caja general por zombie y devuelve hitgroup genérico. No aplica heridas, amputaciones, gibs locales ni decapitación por disparo. Salud, resistencia, daño recibido y muerte los sigue gestionando el CZombie nativo; el gibbing general de seis piezas por daño extremo permanece activo. El sistema anatómico anterior se conserva y se puede reactivar poniendo el cvar a 1.

Con el dano localizado activado, los umbrales son cabeza 35 % de la salud inicial y extremidades 25 %. Una cabeza desprendida produce muerte inmediata; las heridas y amputaciones no restan salud adicional. Los disparos usan cajas orientadas por hueso. El hull de David siempre mide 32x32x72 unidades.

El golpe melee sigue usando el `CheckTraceHullAttack` nativo: una traza de alcance corto que daña un solo `pHit` por evento. La corrección de animaciones impide reinicios de ruta que repetían o cortaban eventos. Si se observan varios Davids golpeando al jugador en el mismo instante, son eventos independientes de varios NPC; la reserva actual limita a uno corriendo por jugador, pero no serializa sus ataques.


## Restos

Si se vuelve a activar el daño localizado, los 13 modelos de partes alimentan el simulador anatómico existente: gravedad 800, pasos de 10 ms, rebote, amortiguación, giro, sonidos, sangre y reposo. Con el modo actual no aparecen gibs locales. El gibbing general conserva seis piezas y su duración de 25 s.

Al reactivar daño localizado, las piezas locales empiezan a desvanecerse a los 0,75 s y se eliminan al cumplir 1 s. Son decorativas, no infligen daño ni empujan al jugador. Hay un máximo de 128 piezas locales y una reserva de 32 edicts para el juego.

## Guardado y compatibilidad

La extension `CremationDavid1` conserva el campo antiguo prone para leer partidas existentes, pero lo limpia al restaurar. Los Davids guardados tumbados recuperan el hull de pie y una secuencia erguida. Se reinstalan hooks y se recalculan objetivos y reservas de carrera. Los restos de un segundo se descartan.

Se encadenan los wrappers existentes de sangre, voces, loot y gibs. Los hooks de David utilizan una tabla clonada y verifican los puntos de entrada del donor actual. No se cambia la IA de Quake, rápidos, venenosos, CZERO, HECU o minions. Las sentries son entidades base con FL_MONSTER, por lo que se excluyen antes de llamar métodos exclusivos de CBaseMonster.

## Configuración y reconstrucción

Los valores por defecto estan registrados en el servidor: dano localizado desactivado y pistola 9 mm (Ricochet) seleccionada al entrar, con sus cargas completas. La postura tumbada por salud esta desactivada. `exec config/david.cfg` permite aplicar la configuracion. `cremation_david 0` deja los nuevos spawns con el modelo anterior; usa un mapa o partida nueva para un cambio completo.

```powershell
python tools/convert_hotd2_david.py
python tools/verify_david_assets.py
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_david_zombie.ps1 -Install
python tools/check_david_runtime.py
python tools/check_david_mansion.py
```

El build dirigido no ejecuta `build_mod.py`, no reinstala otros assets y conserva una copia previa de `dlls/cremation.dll` en `backups/before-hotd2-david/`. No ejecutar el pipeline general para reconstruir este cambio.

## Validación realizada

- Pruebas de lógica: segmentos finitos de disparo, anatomía de descendientes, ataques sin miembros, condiciones de carrera y codificación de bodygroups.
- Lectura del MDL instalado: 64 muestras de poses comparadas con los SMD, altura, orientación de polígonos, máscaras de transparencia, texturas, triángulos y ancho de transmisión de bodygroups. Hashes de los 15 modelos instalados comparados con el manifiesto.
- Render de inspección desde los datos del MDL compilado: `assets-source/hotd2-david/David_compiled_preview.png`; escena de revisión `David_compiled_review.blend` en el mismo directorio.
- DLL Win32 instalada: API 140, 542 exports heredados, constructor y ABI de Ravenholm comprobados.
- GoldSrc real con un jugador del motor: misma salud inicial, misma resistencia a balas y daño simple/doble de 10/25 en skill 1; movimiento nativo 42/130; reserva única y cancelación por enemigos próximos al jugador; aliados y sentry no bloquean.
- Trazado real de cabeza, amputación con descendientes, mano aislada, ausencia de hitboxes fantasma y un impacto que alcanza una entidad detrás de la mano ausente. Fade a los 0,8 s y desaparición después de 1 s; arrastre; decapitación con salud restante y con daño ya letal; gibbing general único. Serialización real de los nuevos campos con el donor.
- Think nativo sin sustitución: detectó al jugador y recorrió aproximadamente 278–300 unidades en la prueba de cinco segundos, usando carrera contextual.
- `cs_mansion`: las cuatro oleadas mixtas existentes completaron el ensayo, con 91 spawns normales usando David, sin errores de precache. El ensayo mata 244 enemigos muy rápidamente; su servidor hijo necesita `-num_edicts 4096` para soportar los gibs de 25 s acumulados. No se cambió el presupuesto de entidades ni los tiempos de la partida normal.

La revision actual desactiva la postura tumbada automatica, restaura el hull y las animaciones erguidas al cargar partidas anteriores y elimina el ajuste especial de punteria tumbada. El arrastre solo se activa al perder una pierna. Se recompilo e instalo la DLL.

Evidencia: `validation-david-diagnostics.txt`, `validation-david-engine.txt`, `validation-david-mansion-diagnostics.txt`, `validation-david-mansion-engine.txt` y `assets-source/hotd2-david/compiled-readback.json`.

El test general `test_ricochet.cpp` falla en su aserción antigua `discs.empty()` de la línea 95; se reprodujo también compilando el proxy de HEAD anterior a David. `verify_mod.py` falla por sus expectativas de armas en rapidcore, cuyo BSP no fue modificado aquí. No se consideran aprobadas esas comprobaciones generales.

Queda la aceptación visual/jugable en una sesión gráfica: sensación de carrera, sincronía perceptiva de contactos, todas las armas y una partida guardada/cargada completa. Se verificó el serializador real de los campos nuevos, no un recorrido completo de savegame gráfico. Para probar el cambio instalado, cerrar completamente Half-Life y comenzar una partida nueva de Cremation.

## Postura tumbada desactivada - 2026-10-26

El campo antiguo `prone` se conserva para compatibilidad de guardado, pero se descarta al restaurar. Las secuencias de ataque tumbado permanecen en el MDL y ya no se seleccionan para David.

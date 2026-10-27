# Corrección de golpes de David y Steve

Se sustituyen los movimientos de mordida 0x3F4/0x3F5 de los tres ataques de pie. Los registros originales de Steve en `HOD2.EXE`, tabla `0x00592F18[8][0]`, proporcionan golpes compatibles con ambos esqueletos de 16 nodos:

| Secuencia GoldSrc | Movimiento original | Contacto original (60 Hz) | Cuadro exportado (30 Hz) | Evento de daño |
|---|---|---|---|---|
| 3, derecha | 0x3EA | 41 | 21 | 1 |
| 4, izquierda | 0x3E9 | 54 | 27 | 2 |
| 5, ambas manos | 0x3C9 | 40 | 20 | 3 |

El contacto impar se redondea al siguiente cuadro almacenado. La inspección del movimiento original confirma un barrido derecho, uno izquierdo y un golpe descendente con ambas manos.

Había además dos errores en la integración. La IA nativa solicita ACT_MELEE_ATTACK2 (31): las pruebas de persecución observaron 0 solicitudes de 30 y 6 de 31 para David. La rama de 31 forzaba siempre la secuencia 5; la selección variada sólo se aplicaba a 30. Ahora ambas actividades usan el mismo selector uniforme para David y Steve, con 1/3 de probabilidad para cada golpe cuando ambos brazos están disponibles.

Tras sobrescribir la secuencia elegida por el donante se invoca su `CBaseAnimating::ResetSequenceInfo`, RVA 0x6BF0, verificado en el mismo binario Ravenholm que protege el control ABI. Así se recalculan la velocidad nativa de cuadros, duración, estado de finalización y reloj de eventos. Cambiar sólo `pev.sequence` dejaba esos datos asociados al movimiento anterior. La ralentización del arma se conserva después del reinicio.

## Comprobaciones realizadas

- Lectura de los MDL instalados: 76 poses de cada personaje coinciden con los SMD originales. Los tres golpes tienen un único evento, el cuadro correcto y la mano correspondiente por delante de la cabeza en el contacto; no son poses de mordida.
- Prueba con la segunda actividad melee: los tres golpes se seleccionan y su velocidad nativa es `256 / duración`; el estado de secuencia finalizada queda desactivado.
- IA natural de David, con persecución real: selecciones derecha/izquierda/doble = 2/2/2, cinco contactos despachados; el último ataque aún estaba en curso al medir.
- IA natural de Steve, con selección controlada de los tres tipos: selecciones 3/2/2, seis contactos despachados y salud real del jugador reducida a 902. La selección uniforme se comprueba aparte para ambos modelos.
- El daño nativo de David continúa en 10/25; Steve recibe el factor 1.10 antes de armadura/redondeo, 11/27.5. Las pruebas de serialización, miembros, muerte y carrera siguen pasando.

Los movimientos archivados de ataque tumbado no se seleccionan por salud; se mantiene desactivada esa función.

Vista desde los MDL compilados: `assets-source/hotd2-steve/David_Steve_strikes.png`. Pruebas repetibles: `tools/verify_david_assets.py`, `tools/verify_steve_assets.py`, `tools/check_david_runtime.py`. Copia anterior: `backups/before-readable-strikes/`.

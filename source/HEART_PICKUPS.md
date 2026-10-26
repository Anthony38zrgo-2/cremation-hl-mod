# Corazones de salud

Cada enemigo hostil hace una tirada entera uniforme 1..100 al morir; 1..10
generan un corazón. La marca `spawnflags & (1<<29)` evita tiradas repetidas
al dañar/desmembrar un cadáver y se conserva en los datos nativos de guardado.
No caen de aliados ni durante derribos recuperables del zombi Quake.

Se reutiliza el cuerpo `PHeartM` de `models/agibs.mdl`, con una copia que
desactiva las estelas de gib del motor. No se modifican los gibs originales.
El pickup flota y gira sin colisión, con glow shell y luz roja. Seis sprites
cuadrados rojos orbitan alrededor, con oscilación vertical.

Recogerlo restaura hasta 25 de vida, con límite de 100. Un jugador muerto no
puede recogerlo y la salud completa no lo consume. Requiere proximidad y línea
libre, sin recoger a través de paredes. Se consume una sola vez.

El corazón y sus partículas desaparecen al recogerlo o transcurrir 10 segundos
desde su creación, comprobado antes de permitir la recogida. Posición base y
temporizadores usan entvars nativas (`oldorigin`, `teleport_time`, `dmgtime`).
La restauración permite regenerar las partículas sin duplicar el pickup; la
aceptación interactiva de guardar/cargar sigue pendiente.

El hook de muerte se instala junto al de sangre para todos los monstruos.
La transición de daño a muerte cubre también la muerte directa personalizada
de Quake. Se preservan el daño, los gibs y los hooks del resto del mod.

- Recursos: `python tools/prepare_heart_pickup.py`.
- Prueba real de 16 comprobaciones: `python tools/check_heart_runtime.py`.
- Evidencia y captura: `diagnostics/heart-runtime.json`, `heart-trace.log`,
  `heart-pickup-preview.png`.

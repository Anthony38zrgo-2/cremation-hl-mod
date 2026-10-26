# Plan de integración de David en Cremation

Estado: implementado e instalado, 2026-10-26. Este documento conserva el diseño acordado; los detalles finales, pruebas y límites de validación están en [DAVID_ZOMBIE.md](DAVID_ZOMBIE.md).

Ajuste actual solicitado: daño localizado desactivado y una caja general, pistola Ricochet seleccionada al inicio, y David pasa a estar tumbado por debajo del 50 % de su salud inicial. Tumbado usa ataques y muerte en postura baja.

## Objetivo y alcance

Reemplazar la apariencia, animaciones e hitboxes de todos los `monster_zombie` normales por David de HOTD2, con carrera condicional, heridas por zona, desprendimiento de partes, decapitación letal y compatibilidad con el gibbing general de Cremation. Conservar el daño del zombie normal y su balance base. Quake, rápidos, venenosos, CZERO, HECU y minions conservan su identidad.

La selección será por classname exacto `monster_zombie`, con estado explícito de David. No utilizar coincidencias de texto que también incluyan Quake u otros tipos. Usar un modelo dedicado y asignarlo en spawn/restore evita que otros NPC que compartan recursos hereden accidentalmente este sistema.

## Evidencia actual

- `liblist.gam` carga `dlls/cremation.dll`. `source/ricochet_proxy.cpp` carga `hl_ravenholm.dll`; los cambios de comportamiento se integran en este proxy. El CZombie del HLSDK disponible no demuestra el comportamiento del binario Ravenholm.
- El zombie normal usa actualmente los recursos instalados por `tools/install_purple_zombie.py`. La variante carbonizada se encuentra en `models/cremation/charred/zombie.mdl`.
- `source/humanoid_gibs.inc` envuelve TakeDamage/Killed y sustituye los gibs nativos de humanoides por seis piezas anatómicas. Su simulador usa pasos de 10 ms, gravedad 800, rebote, rozamiento, reposo, sonido y sangre; las piezas generales duran 25 s.
- `source/weapon_blood.inc` registra impactos de TraceLine y encadena TakeDamage. Varias armas personalizadas llaman directamente a TakeDamage, por lo que no todas suministran hitgroup.
- `source/tactical_save.inc` ya implementa campos adicionales a través del serializador del donor. No asumir que iuser/fuser se guardan automáticamente.
- Recursos recuperados: `D:/games/House_of_the_dead_2_ISO/David_analysis/David_blender/David_HOTD2.blend`. Modelo completo: 16 piezas, 721 triángulos, rig de 16 nodos; banco de 73 piezas y 28 texturas. Waiting 0x3CF, Walk 0x3FE y Run 0x3C8 están extraídos. La pose se verificó contra matrices originales.
- Las tablas del ejecutable confirman variantes de herida de David: torso 7994/7995/7996; cabeza 7976/7977; brazos 7983/7980; antebrazos 8008/8005; manos 8002/7999; muslos 7991/7988; espinillas 7943/7940. Los valores 0/1/2 de estas tablas son controles, no IDs de mallas.
- `skill.cfg` declara salud 50/50/100, zarpazo simple 10/20/20 y doble 25/40/40 según dificultad. Tomar estos valores como configuración existente y comprobar el resultado efectivo del donor al iniciar una partida.

## Decisiones de comportamiento

### Daño y balance

Mantener salud configurada, daño por evento de ataque, alcance, resistencias y modificadores efectivos del zombie normal actual. Antes del reemplazo, medirlos en el binario activo. No importar el sistema numérico de daño/dificultad de HOTD2.

Los ataques nuevos reproducen las animaciones originales, pero sus eventos llaman a la ruta de ataque del zombie normal, una vez por contacto previsto. Correr no aumenta el daño ni multiplica golpes. Los umbrales de desmembramiento son contadores regionales y no una segunda resta de salud. La decapitación letal constituye la excepción solicitada al resultado de supervivencia.

### Cuándo corre

Condición corregida por el usuario: la carrera depende de que el jugador objetivo no tenga otros enemigos vivos cerca, en un rango medio alrededor del jugador. El propósito es evitar que muchos enemigos se acumulen rápidamente sobre él. La zona de comprobación está centrada en el jugador, no en David. Los aliados del jugador no bloquean esta condición.

Propuesta inicial ajustable, no parámetros actuales del mod:

1. David vivo persiguiendo al jugador, con visión directa y ruta transitable; distancia al blanco entre 128 y 768 unidades.
2. Ningún otro enemigo vivo del jugador dentro de un radio medio inicial de 256 unidades alrededor de ese jugador. Contar cualquier tipo de enemigo hostil, no solo otros David: zombies normales, rápidos, venenosos, Quake, HECU hostiles y demás amenazas. Excluir al David evaluado, cadáveres, gibs, aliados y entidades sin capacidad de combate. Determinar hostilidad por relaciones efectivas.
3. Comprobar proximidad horizontal y separación vertical para no contar enemigos de otra planta. Una pared que realmente separe las zonas de combate evita un bloqueo falso; verificar acceso local/visibilidad desde el jugador, no desde David. Los enemigos a los lados, delante o detrás cuentan: lo relevante es la ocupación del entorno cercano del jugador.
4. Cabeza y ambas piernas funcionales; fuera de ataque, flinch, caída y muerte.
5. Condición sostenida 0,4 s para entrar; reconsideración cada 0,2 s. Si otro enemigo entra en las 256 unidades del jugador, David deja de correr y continúa caminando. Para reanudar, exigir que la zona quede despejada hasta 320 unidades y respetar un enfriamiento inicial de 1 s; estos radios distintos evitan oscilaciones en el borde. También cancelar por pérdida de ruta, herida incapacitante o proximidad de ataque.
6. Evitar que todos decidan correr en el mismo frame hacia un jugador todavía despejado: permitir como máximo un David corriendo hacia cada jugador a la vez, con reserva asignada de forma determinista entre candidatos elegibles. La reserva se libera al dejar de correr, morir, perder el objetivo o desaparecer. Si hay otros candidatos, deben volver a evaluar la ocupación del entorno antes de obtenerla. Esto evita una carrera simultánea de toda la oleada mientras el primer enemigo aún está lejos.

Los radios 256/320 son una propuesta de rango medio que se ajustará en combate. El requisito fijo es comprobar enemigos alrededor del jugador y limitar la llegada rápida de varios David; no comprobar si el jugador tiene compañía aliada ni exigir que el atacante esté aislado de otros David.

La locomoción real y ACT_WALK/ACT_RUN deben cambiar juntas. Medir velocidad del ciclo original y adaptarla a unidades GoldSrc tras fijar escala. No simular carrera aumentando solo framerate. Conservar el enrutado y evasión nativos donde sea posible; verificar el enlace concreto para selección de actividad/ruta del donor antes de elegir hook. Evitar que `hordeWalkAnimation` o el fallback de oleadas sobrescriban ACT_RUN. Respetar ralentizaciones de armas y no aplicar multiplicadores de velocidad acumulativamente.

### Heridas, desprendimientos y muerte

- Cabeza, torso, brazo/antebrazo/mano de cada lado y muslo/espinilla/pie de cada lado tienen estado regional. El daño corporal total sigue la ruta normal una sola vez.
- Primero usar las mallas heridas auténticas; después la separación donde exista una variante coherente. Inventariar visualmente cada transición antes de etiquetarla. No asumir que las 73 piezas representan 73 amputaciones.
- Al desprender una zona: ocultar su geometría y descendientes que correspondan, mostrar el muñón adecuado, desactivar sus hitboxes, generar una sola pieza separada con el aspecto y transformación de ese instante, y conservar permanentemente ese estado en el zombie.
- Brazo perdido: impedir el golpe de ese brazo. El brazo restante mantiene el daño de su golpe normal; un doble zarpazo no debe conservar dos contactos fantasma.
- Una pierna perdida: cancelar carrera y pasar a caída/arrastre si existe un clip original compatible. Si no existe, la fase de assets debe resolver una animación coherente antes de habilitar esa amputación en producción; no dejar un zombie andando sobre un hueco. Con ambas piernas inutilizadas, permanece en locomoción de suelo o muere según la transición aprobada en esa fase. Propuesta base: caída y arrastre, sin bonificar daño.
- Decapitación: impactos reales de cabeza acumulan daño regional; al superar su umbral se desprende la cabeza y el zombie muere inmediatamente, aunque le quede salud corporal. No implica que cualquier bala decapite automáticamente. El umbral será explícito y ajustable por dificultad, decidido tras medir el daño efectivo actual.
- Conservar flinches y movimientos de daño de HOTD2 además de los cambios de malla. Extraer y clasificar del banco mot/zom.bin los ataques, dolor, caídas, muertes y las reacciones compatibles. Actualmente solo espera/caminar/correr están incluidos en el .blend entregado.
- Daño extremo: la decisión de gibbing existente tiene prioridad sobre la transición localizada. El zombie desaparece mediante la misma ruta de gibbing que los otros monstruos; no generar a la vez un segundo cuerpo/cabeza/corazones ni otra muerte. Mantener inicialmente el conjunto general de gibs instalado.
- Link Gun, incineración y Necromancer conservan sus rutas especiales. Las variantes carbonizadas de David necesitan la misma jerarquía, secuencias, bodygroups y estado de partes; nunca cambiar a la antigua silueta del zombie durante una muerte térmica.

Los umbrales regionales no están fijados todavía: se calibran usando el daño recibido efectivo, tras resistencias, hitgroups y dificultad. Guardarlos en una configuración propia. Armas de área sin un impacto anatómico válido no provocan decapitación por una estimación arbitraria; sí pueden matar y activar gibbing normal.

## Implementación por fases

### 1. Modelo GoldSrc y catálogo de animaciones

Crear `assets-source/hotd2-david/` con copia trazable de los recursos, manifiesto de hashes y exportador reproducible. Compilar en `models/cremation/hotd2/david/` un `david.mdl`, modelos de partes separadas y variante carbonizada.

Exportar rig y mallas a SMD/QC: GoldSrc usa una influencia por vértice; el abdomen recuperado ya asigna cada vértice a un nodo original, así que se preservan esas asignaciones. Convertir ejes, origen en los pies y escala usando el zombie normal como referencia; comprobar el MDL compilado, no solo Blender. Crear texturas indexadas de 8 bits y transparencia compatible sin perder UV.

Crear bodygroups por regiones con variantes intacta/herida/ausente y muñones cuando proceda. Verificar límites del entero de bodygroups, tamaños del MDL, materiales y secuencias. Seleccionar actividades GoldSrc para idle, walk, run, ataques, flinches y muertes. Registrar IDs originales y cuadros de eventos de golpe en el manifiesto. No instalar antes de completar los clips requeridos.

### 2. Hitboxes y registro anatómico

Definir hitboxes por hueso ajustadas a David en el QC. Separar hitboxes de disparo de la caja física de desplazamiento: conservar inicialmente el hull navegable del zombie y ajustar altura/origen solo si las dimensiones medidas lo requieren. La silueta no implica cambiar automáticamente el ancho de puertas ni el alcance de golpe.

GoldSrc no elimina hitboxes al ocultar bodygroups. Por tanto, implementar una resolución de impactos específica de David con máscara de partes activas y cajas por hueso en la pose de servidor. Integrarla en el TraceLine encadenado existente y en el TraceAttack del zombie. Si el hit nativo cae en una parte ausente, resolver la continuación real del segmento contra mundo, partes restantes y otras entidades; no descartar ciegamente el impacto ni permitir atravesar paredes.

Para armas personalizadas de daño directo, pasar explícitamente el contexto de impacto desde el trazado o Touch responsable. No reutilizar un hit antiguo solo porque ocurrió en el mismo frame. Contexto identificado por víctima/serial, ataque y segmento/pellet; consumible y sin contaminación entre explosiones y disparos.

Mantener multiplicadores de hitgroup del mod, no aplicarlos dos veces. Si basta el TraceAttack nativo para calcularlos, delegar y observar el daño resultante. Las pruebas de partes ausentes y proyectiles son requisito de esta fase.

### 3. Integración en el servidor activo

Añadir `source/david_zombie.inc` y configuración/manifiesto propios, integrados desde `ricochet_proxy.cpp` en precache, spawn, think, daño, muerte y restore.

Clonar tabla virtual por variante/instancia como hacen las clases independientes existentes; no alterar una tabla compartida que afecte minions u otras clases. Encadenar correctamente los wrappers de sangre, gibs, voces y drops ya presentes. Conservar el CZombie nativo para salud, daño, combate y navegación donde el ABI verificado lo permita; añadir únicamente decisiones de actividad, carrera y daño regional. No sustituir toda la IA por la del Quake zombie.

Registrar estado de regiones, máscara de amputación, daño acumulado regional, actividad y temporizadores. No reservar entvars privados usados por el donor sin una auditoría previa. Preferir una extensión de guardado con nombre propio para los datos nuevos y un registro de ejecución protegido por edict+serial.

### 4. Física de partes y desaparición

Extender el simulador de `humanoid_gibs.inc` con un descriptor por pieza: modelo/body, origen anatómico, orientación, impulso, gravedad, coeficientes de rebote y vida. Reutilizar una sola simulación por resto, con piezas decorativas no sólidas y sin daño de contacto.

Los desprendimientos locales de David duran exactamente 1,0 s desde la separación: comenzar fade a los 0,75 s y retirar a 1,0 s. No esperar al primer impacto ni al reposo. Cabeza incluida. Mantener sin cambios la duración de 25 s de los gibs generales de los demás monstruos.

Calcular límites y apoyo en el suelo desde el modelo concreto: `anatomicalRest` actualmente presupone la estructura/body de hgibs y no se puede usar sin generalizarlo. Nacimiento desde la transformación real del hueso, sin geometría de otra pose ni teletransporte al centro. Impulso del disparo más velocidad del zombie, con límite, sangre y sonidos existentes. Límite global de restos y degradación de efectos para ráfagas/escopetas, sin sacrificar el estado lógico de amputación.

### 5. Guardado, oleadas y compatibilidad

Persistir heridas, partes ausentes y muerte; reinstalar los hooks/modelo correcto al restaurar. Recalcular los enemigos alrededor de cada jugador y reconstruir las reservas de carrera después de cargar, sin conservar reservas de entidades desaparecidas. Descartar restos transitorios como ya hace el mod, sin regenerar extremidades. Guardar referencias de entidad con serial y evitar aplicar estado a edicts reciclados.

Cubrir spawns de mapa, oleadas y restauración. Mantener voces del zombie y alianza con HECU. Revisar radar, conteo de vivos, drops/corazones, minions, ralentizaciones y efectos térmicos. Los cadáveres antiguos de mapas requieren una decisión separada; este reemplazo afecta primero a zombies vivos normales.

## Pruebas de aceptación

1. Modelo: frente/espalda, pies en suelo, ojos, UV, paletas, transparencia, costuras, rig, todos los ciclos y ausencia de deslizamiento evidente.
2. Balance: comparar salud y daño por dificultad contra el zombie normal de referencia. Misma arma, distancia, hitgroup y resistencias; comprobar golpe simple y doble, caminando y corriendo.
3. Carrera: con el entorno del jugador despejado puede correr un David; con cualquier otro enemigo hostil dentro del rango medio, los David que se aproximan caminan. Probar enemigos de distintos tipos y en todas las direcciones alrededor del jugador; aliados no bloquean la carrera. Otro David cerca del atacante pero lejos del jugador no invalida por sí mismo la condición. Paredes y plantas separadas no bloquean indebidamente. Varios candidatos simultáneos obtienen una sola reserva por jugador; al llegar el primero, el resto no se precipita corriendo. Probar entrada/salida de los radios, muerte del corredor, cambio de objetivo y carga de partida. No debe oscilar cada frame ni correr incapacitado.
4. Anatomía: impactos en cada región durante varias animaciones, estados heridos correctos y una única pieza desprendida. Disparar por un brazo ausente alcanza lo que realmente haya detrás. Un disparo próximo a la cabeza que no la toca no decapita.
5. Muerte: cabeza separada implica muerte única y atribución correcta. Sobre-daño activa el gibbing general una sola vez. Probar varias balas/pellets en el mismo frame, cadáveres, explosiones, Link Gun y fuego.
6. Restos: rebote y sangre como los gibs existentes; retirada a 1 s independientemente de velocidad/impacto; sin empujar jugador/monstruos, ni duplicar física.
7. Persistencia: guardar/cargar un David herido, amputado y persiguiendo; repetir con oleadas, reinicio de mapa y reutilización de edicts. No reaparecen miembros.
8. Regresión y rendimiento: mismos comportamientos de Quake/rápidos/venenosos/CZERO/HECU/minions; combates con 10–40 Davids y armas de múltiples impactos, presupuesto de entidades y coste de trazados medidos.

Agregar pruebas focalizadas de la decisión de carrera, máscara y enrutado de hitboxes, daño único, decapitación, prioridad del gibbing y serialización. Ejecutar build/validaciones del proxy y comprobaciones del MDL; después reiniciar completamente GoldSrc y hacer las pruebas visuales/de combate. Una compilación o lanzamiento de 20 s no demuestra estas condiciones.

## Orden de entrega y recuperación

A. Assets completos y mapa de variantes/clips; B. modelo/hitboxes con balance preservado; C. caminar/correr contextual; D. heridas, amputación y decapitación; E. física/fade y compatibilidad de muertes; F. guardado, carga y aceptación completa.

Mantener interruptor de desarrollo para desactivar David y volver al modelo/comportamiento normal, con copias de los recursos y DLL afectados. Instalar únicamente los assets y DLL necesarios; evitar el pipeline general que sobrescribe modelos personalizados. El repositorio tiene cambios previos ajenos: no incluirlos ni descartarlos durante la implementación.

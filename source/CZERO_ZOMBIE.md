# Zombi infiltrador y objetivo de bomba

Entidad `monster_czero_zombie`: exactamente uno adicional por oleada, en un CT libre al azar. Totales 41/41/81/81. Aliado de los zombies, enemigo del jugador y excluido del radar. Conserva 100 HP, hitboxes de la primera carpeta y compatibilidad con daño, congelación, empuje, carbonización, gibs y corazones del mod.

## Infiltración

Lleva el knife con la skin Leviathan. Sigue la ruta del spawn hasta la plataforma central; después elige una de las dos puertas. Evita entradas bloqueadas o con más de tres enemigos en 128 unidades. Si ninguna resulta accesible, busca la escalera norte, trepa mediante física real, se agacha para pasar por la ventana y entra a la planta alta. Puede romper los cristales que obstruyan el acceso.

`tools/build_czero_navigation.py` obtiene los suelos y escalones del BSP real. La búsqueda de camino penaliza aglomeraciones y comprueba la posición actual de las hojas de las puertas giratorias. No cambia la geometría de la mansión ni teletransporta al NPC. Regenerar `source/czero_navigation.h` y recompilar la DLL cuando se cambie la geometría.

## Dos granadas

Dispone de una smoke y una flash por vida, guardadas con flags persistentes. No lanza humo al salir del spawn: busca al menos cuatro zombies agrupados en un radio de 144 unidades cerca o dentro de la mansión, a menos de 768 unidades del NPC. Después puede lanzar su única flash al ver a un jugador. La flash comprueba línea de visión, distancia y orientación antes de aplicar `ScreenFade`. El humo sigue durando 18 segundos y oculta la visión sin colisión sólida.

## Plantado, defensa y desactivación

El objetivo son las posiciones originales de rehenes: (-304,-272,56) por las puertas o (-624,-176,192) por la escalera. Planta durante 3 segundos. Una vez plantada la bomba equipa inmediatamente una Glock; antes conserva el hacha. La Glock tiene cargadores de 20, recarga de 1,8 segundos y reserva ilimitada. La defensa alterna cobertura real y posiciones de asomado, tomando como referencia el último jugador visto, dentro de 256 unidades de la bomba. Es una adaptación para el ABI de Cremation; no carga la DLL ni los bots binarios de CSZ dentro del servidor Ravenholm.

La bomba tarda 35 segundos. Mantiene la oleada pendiente aunque se elimine a todos los enemigos. Todos los jugadores tienen kit: primero hay que matar a su defensor y después mantener USAR a menos de 88 unidades, con visión de la bomba y sin moverse, durante 5 segundos. Soltar USAR, alejarse o morir cancela el progreso. La explosión produce Game Over y mata al jugador incluso con invulnerabilidad; hay que reiniciar la partida.

## Modelos y persistencia

Los modelos de knife y Glock conservan el rig original de 34 huesos y 20 hitboxes. Las marchas mezclan piernas originales y torso del arma correspondiente; las poses CS de nueve blends se reducen a la pose central para el renderer HL. Las pistas se insertan antes de las texturas y tienen muestras de guarda. Hay variantes carbonizadas para ambos cuerpos.

El servidor sigue usando la asignación, TraceAttack e interfaces nativas de Ravenholm. Fase, inventario, bomba y temporizadores se serializan con entvars y la extensión versionada `CremationTactical1`; los campos de usuario omitidos por el guardado estándar tienen un bloque explícito. Las cachés de rutas se reconstruyen tras cargar.

## Validación

`check_czero_tactical.py`: recorrido físico por las dos puertas y la escalera, plantado en rehenes, Glock, granadas, desactivación y explosión; también los cuatro artefactos DMC y la Link Gun exponencial con enlaces. `check_czero_runtime.py`: 23 pruebas nativas de daño y efectos. `check_tactical_save.py`: guardado y restauración en procesos distintos. `check_mansion_runtime.py`: las cuatro oleadas y sus 244 enemigos. Los modelos se reproducen con `install_czero_zombie.py` e `install_dmc_powerups.py`.

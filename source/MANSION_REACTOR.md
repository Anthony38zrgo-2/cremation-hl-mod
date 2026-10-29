# Mansion: cuatro frentes y dos islas de recompensas

Estado anterior guardado en el commit `e87955f` antes de modificar geometría.
El usuario confirmó un único pozo conectando las zonas marcadas y la pileta,
conservando un puente central para la ruta de los enemigos. La ampliación
posterior sustituye ese único acceso por cuatro puentes simétricos.

## Geometría

- Suelo exterior Z=24; fondo Z=-424: profundidad de 448 unidades.
- Se retiraron los obstáculos marcados y la pileta original mediante corte
  de brushes. El vacío bajo el puente conecta las partes norte y sur.
- Arena de 5120 unidades por lado, centrada en la mansión (-352,-256).
  Plataforma central de 1664 por lado y cuatro recintos de aparición de 2560
  por 768, centrados a 2048 unidades del centro al norte, este, sur y oeste.
  Cada recinto tiene muros de 24 unidades de grosor y 192 de altura, con una
  abertura de 320 unidades centrada en la entrada del puente. Los diez puntos
  por frente se separan 400 unidades lateralmente y 256 entre filas; las rutas
  convergen hacia una franja interior antes de atravesar la abertura.
- Cuatro puentes iguales de 832 unidades de largo y 256 de ancho. Las rutas
  pasan por el patio y conservan como destino la puerta principal original.
- Acceso sur reconstruido con cuatro escalones de 8 unidades y caras visibles
  en los remates de cimentación. Corrige el rectángulo que dejaba ver el líquido
  bajo el suelo; se retiran los restos de escalera recortados del BSP original.
- Grafo nativo reducido de 403 a 81 nodos, con muestreo por cobertura y límite
  de 96. El exceso anterior rebasaba las 128 conexiones iniciales por nodo y
  activaba las cinco nubes de diagnóstico de `CTestHull::ShowBadNode`.
  El grafo corregido se genera con 882 conexiones, sin ocultar efectos nativos.
- Casas pequeñas enfrentadas en las diagonales noreste y suroeste, simétricas
  respecto al centro de la mansión. Conservan el borde de 32 unidades y tejado
  a Z=168. Isla NE: X=960..1216, Y=1088..1280; isla SO: X=-1920..-1664,
  Y=-1792..-1600.
- Fondo con líquido nativo `!RADIO`, franjas `SCROLLTOXIC` y luces verdes
  compiladas. Texturas utilizadas incluidas en el BSP.
- Las siete franjas de las bases son `func_illusionary` sin colisión. Conservan
  el brillo y dejan caer las cajas de colisión hasta el fondo, sin la antigua
  repisa sólida a Z=-384 que las detenía antes del umbral letal Z=-399.
- Plataforma NE en (448,544,28); destino sobre el tejado (1088,1184,205).
  Vuelo balístico de 1,35 segundos adaptado a `sv_gravity` y
  a la gravedad del jugador. Frenado horizontal al tocar el tejado.
- Plataforma SO en (-1152,-1056,28), destino (-1792,-1696,205). Ambas están
  en las esquinas diagonales de la plataforma central, alineadas con las casas.

El fondo mata inmediatamente a jugadores y monstruos, sin excepción por
armadura, vida alta o godmode. Ambas islas y los cuatro puentes conservan suelo físico.
No se teletransporta al jugador durante el salto.

## Recompensas

Las estaciones de las casas NE y SO entregan exclusivamente Flak Cannon,
Link Gun, Shock Rifle o Rocket Launcher. La antigua estación de powerups
usa ahora la misma selección de armas. No entrega granadas ni powerups.

Las representaciones flotan, giran y tienen contorno y partículas cuadradas
doradas. Recogida por proximidad: concede el arma si falta y munición del
tipo correcto. Una reserva llena no consume la recompensa. Reaparece con una
nueva selección a los 10 segundos. Las estaciones y temporizadores usan campos
serializables del motor; hace falta aceptación interactiva de guardar/cargar.
La retirada general de armas tiradas continúa activa: estas estaciones usan
representaciones propias y entregan el arma al inventario.

La estación del fondo es un portal azul animado, en (1040,1184,72).
Entrar en él devuelve al patio trasero de la mansión, en (-784,-272,61),
mirando hacia la casa. Comprueba casco humano y suelo antes de transportar;
si la salida está ocupada busca un punto cercano del mismo patio. Detiene el
impulso de entrada y aplica dos segundos de cooldown. Es un regreso de un solo
sentido, sin telefrag.

La casa suroeste conserva su salto y portal de regreso, pero su estación usa
la misma selección de cuatro armas y espera diez segundos tras la recogida.
Su portal reflejado en (-1744,-1696,72) devuelve al mismo patio trasero seguro.

El jugador empieza con revólver, Ricochet y motosierra, además del traje y su
equipo de movimiento. Hay ocho cajas .357 (cuatro por planta) y cuatro granadas
en la planta superior. Cada granada concede una unidad. Los suministros no se
duplican al activar el mapa y lo recogido no se repone al cargar una partida.

## Fuentes y compilación

La fuente original disponible era el BSP, no un MAP del autor. Se reconstruyó
con HalfLife.UnifiedSdk.MapDecompiler (Tree), se editaron brushes convexos con
`tools/map_brushes.py` y se recompiló con SDHLT 1.3.1 (CSG/BSP/VIS/RAD).
La reconstrucción genera advertencias heredadas de brushes finos y volúmenes
ambiguos; no es una recuperación perfecta de los brushes originales.
El BSP conserva la mansión y la casa pequeña y amplía el contorno exterior;
requiere revisión visual de
los detalles del mapa y combate en una sesión del usuario.

- Fuente editable: `assets-source/mansion-reactor/cs_mansion_reactor.map`.
- Datos geométricos: `assets-source/mansion-reactor/reactor-layout.json`.
- `python tools/build_mansion_reactor.py`: construye, compila e instala el mapa.
- `tools/build_mansion_cross.py`: generador de la ampliación de cuatro frentes;
  el constructor general lo selecciona cuando `four_fronts` está activo.
- `powershell -ExecutionPolicy Bypass -File tools/build_mansion.ps1`: DLL y lanzador.
- `python tools/check_reactor_runtime.py --client`: pruebas reales del cliente.
- `python tools/check_mansion_runtime.py --routes`: 20 recorridos de cuatro frentes.
- `python tools/check_mansion_runtime.py`: cuatro oleadas completas.
- `python tools/render_mansion_grid.py`: plano de referencia actualizado.

Herramientas externas locales (no se instala software en Windows):
https://github.com/twhl-community/HalfLife.UnifiedSdk.MapDecompiler
https://github.com/seedee/SDHLT

Las variables `CREMATION_REACTOR_VALIDATE` y `CREMATION_REACTOR_PREVIEW` solo se
establecen en procesos de prueba. No se activan desde el lanzador normal.
La prueba real confirmó el arco hasta Z=324 y aterrizajes sobre ambos tejados,
cerca de (1090,1186,203) y (-1794,-1698,203), además de recogidas,
reaparición, portales y daño letal. Las rutas de salida de los 40 puntos se
comprueban con casco humano y suelo continuo; el ensayo nativo recorre los
cuatro frentes con 20 actores de los cinco tipos hasta la puerta.

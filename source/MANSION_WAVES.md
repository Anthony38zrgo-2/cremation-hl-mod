# Mansion y oleadas

La geometría instalada incluye el pozo radiactivo, cuatro puentes simétricos
y dos casas pequeñas con salto y portal descritos en
[MANSION_REACTOR.md](MANSION_REACTOR.md). La casa noreste ofrece armas del mod
con munición; la suroeste ofrece salud, batería y salto largo de Half-Life.

Mapa predeterminado: `cs_mansion`. Skybox activa: `city`, restaurada directamente
desde `valve_addon_b921d.rar`, con sus seis caras originales sin cambios.
El instalador general mantiene esta selección.

El lanzador abre el menú principal. Allí `Wave 1`–`Wave 4` cargan las copias
`cs_mansion_wave1`–`cs_mansion_wave4` del mismo BSP y el controlador empieza
directamente en la oleada indicada. Las acciones usan el prefijo `engine ` de
GameUI para enviar el comando `map` al motor. New Game, Load Game, Find Servers,
Create Server y Change Game están retirados del menú. Al reinstalar el mapa
se regeneran las cuatro copias.
El menú usa `HALF DEAD.jpg` como fondo y reproduce en bucle `Introduction (Attract Mode)`;
al entrar a un mapa vuelve al tema de partida. `cremation_music`
y `cremation_music_volume` controlan reproducción y nivel.

Skybox alternativa conservada, actualmente desactivada: `cremation_mountains`, mezcla
generada con el editor de imágenes entre el cielo City original y la referencia
de montañas rocosas con niebla roja. Las seis caras TGA de 256×256 se proyectan
desde un único panorama; sus 24 bordes coinciden exactamente. El panorama y el
prompt se conservan en `assets-source/skybox-mountains/`, y la instalación se
reproduce con `python tools/install_mountain_sky.py`. Las seis caras `city` del
archivo `valve_addon_b921d.rar` se conservan intactas. El mapa procede de `cs_mansion.7z`.

El cliente GoldSrc instalado limita las caras del cielo a 256×256: rechaza
512×512 con `LoadTGA: texture too large` y carga `desert` como reemplazo.
La exportación usa TGA RGB de 24 bits sin compresión, a 256×256.

Revisión seamless: el editor de imágenes sustituyó los tonos marrones/grises
de las montañas por rojo sangre oscuro y suavizó el cierre horizontal y el
cenit del panorama. La exportación comprueba también la discontinuidad del
panorama fuente y la variación del cenit: comprobar solo los bordes del cubo
no detectaba la costura interna ni el efecto de estrella al mirar arriba.
Los originales extraídos están en `assets-source/mansion/`; no se ejecutó
contenido de los archivos. Las texturas originales necesarias se recopilan
desde los WAD locales de Half-Life en `mansion_textures.wad`.
La versión anterior al reactor conservaba los 14 bloques BSP de geometría;
la versión actual se recompila desde la fuente reconstruida para modificar
físicamente el patio, el pozo, las colisiones y la iluminación.

Los diez `info_player_deathmatch` originales (TT) pasan a ser inicios del
jugador, conservando posiciones y orientaciones. Se conservan también los
inicios multijugador TT en esos mismos puntos para compatibilidad con Ravenholm;
los inicios de jugador no llevan nombre de destino. El servidor entrega el
equipamiento directamente al inventario al entrar, sin armas colocadas en el
suelo; también elimina las armas que sueltan enemigos o jugadores. Los diez
marcadores CT antiguos se sustituyen por 40 marcadores: diez en cada frente
cardinal de la arena ampliada. Se eliminan las
entidades de rehenes y cámaras de Counter-Strike.

| Oleada | Normales* | Johnny fijo | Quake | Rápidos | Venenosos | Cultistas | Zombi knife | Total |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 30 | 10 | 0 | 0 | 0 | 0 | 1 | 41 |
| 2 | 13 | 15 | 12 | 0 | 0 | 0 | 1 | 41 |
| 3 | 20 | 0 | 20 | 20 | 20 | 0 | 1 | 81 |
| 4 | 10 | 0 | 10 | 10 | 10 | 40 | 1 | 81 |

Los Johnnys sustituyen plazas del grupo principal, no se suman a la oleada.
La primera oleada fija diez y la segunda quince; la selección aleatoria global
de Johnny queda desactivada para el resto de zombis base de esas dos oleadas,
por lo que sus cantidades son exactas. En las oleadas 3 y 4 sigue aplicándose
`cremation_johnny_chance` a los zombis normales. En las filas 3 y 4, un
Johnny seleccionado aleatoriamente ocupa una de las plazas `Normales*` y no
aumenta el total.

Cada oleada añade un [zombi con knife Leviathan](CZERO_ZOMBIE.md), con el
modelo de la carpeta 0 del RAR. Aparece en un punto CT libre al azar, cuenta
como enemigo y conserva compatibilidad con los efectos de Cremation.

La primera oleada comienza cinco segundos después de entrar un jugador vivo.
Se intenta generar un enemigo cada 0,375 segundos, alternando norte, este,
sur y oeste y rotando los diez puntos de cada frente. Cada frente recibe
10 enemigos en las dos primeras oleadas y 20 en las dos últimas. En la cuarta
oleada, los 40 cultistas salen desde puntos CT normales elegidos al azar; 20
inician una ruta alrededor del perímetro y 18 siguen las entradas de puerta
hacia el jugador. Los otros dos conservan la incursión por la puerta trasera.
La selección evita puntos ocupados. La composición mantiene 40 cultistas y
dos o tres de cada tipo de zombi; como los CT de los cultistas se eligen al
azar, su reparto por frente cambia entre partidas.
Un punto ocupado retrasa la aparición; no se superponen monstruos ni se omiten
puntos. La siguiente oleada empieza ocho segundos después de eliminar todos
los enemigos y agotar las apariciones pendientes de la anterior. La cuarta
termina con un mensaje de finalización; no se repite indefinidamente.

El HUD usa `TE_TEXTMESSAGE`, canal 4, centrado horizontalmente en y=0,86,
con actualización cada 0,5 segundos. Muestra oleada, enemigos realmente vivos
y aviso de refuerzos mientras quedan apariciones pendientes. Los aliados del
jugador y los cadáveres no cuentan como enemigos vivos.

Las rutas CT–puerta principal se calculan sobre el BSP, con casco humano,
comprobación de suelo, escalones, altura adicional del zombi Quake y margen
lateral. El movimiento utiliza `WalkMove` con colisiones, sin teletransportes
ni secuencias cinematográficas. La detección de objetivos visibles cede el
combate a la IA de cada enemigo; al perder el contacto puede continuar la ruta.
Los enemigos mantienen la alianza de facción zombi/cultista.

El controlador `mansion_wave_state` conserva oleada, contador de apariciones,
cursor CT, temporizadores y finalización en campos nativos serializables.
Cada enemigo conserva su siguiente marcador en `target`. Una prueba completa
interactiva de guardar/cargar sigue siendo necesaria para aceptar ese flujo.

## Herramientas

- `python tools/install_mansion.py`: reinstala mapa, WAD, cielo, rutas y mapa predeterminado.
- `python tools/install_menu.py`: regenera el fondo del menú y la pista en bucle desde `assets-source/menu/`, además de las copias para iniciar cada oleada.
- `powershell -ExecutionPolicy Bypass -File tools/build_mansion.ps1`: compila solo servidor, pruebas y lanzador; no ejecuta el pipeline general de armas.
- `python tools/check_mansion_runtime.py`: prueba acotada de las cuatro oleadas con muertes por daño nativo.
- `python tools/check_mansion_runtime.py --routes`: 20 recorridos físicos desde los cuatro frentes, con los cinco tipos de enemigos.

Las pruebas de motor usan exclusivamente su proceso de servidor dedicado y la
variable `CREMATION_MANSION_VALIDATE`; el juego normal no activa ese modo.
Evidencia en `diagnostics/mansion-*.json` y `diagnostics/mansion-*-server.log`.
La revisión visual del cielo, HUD y combate requiere una sesión interactiva.
Reiniciar completamente Half-Life e iniciar una partida nueva para usar el mapa.

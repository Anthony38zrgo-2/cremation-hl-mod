# Mansion y oleadas

La geometría instalada incluye el pozo radiactivo, cuatro puentes simétricos
y dos casas pequeñas con salto y portal descritos en
[MANSION_REACTOR.md](MANSION_REACTOR.md). La casa noreste ofrece armas del mod
con munición; la suroeste ofrece salud, batería y salto largo de Half-Life.

Mapa predeterminado: `cs_mansion`. Skybox activa: `city`, restaurada directamente
desde `valve_addon_b921d.rar`, con sus seis caras originales sin cambios.
El instalador general mantiene esta selección.

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

| Oleada | Normales | Quake | Rápidos | Venenosos | Cultistas | Zombi knife | Total |
|---|---:|---:|---:|---:|---:|---:|---:|
| 1 | 40 | 0 | 0 | 0 | 0 | 1 | 41 |
| 2 | 20 | 20 | 0 | 0 | 0 | 1 | 41 |
| 3 | 20 | 20 | 20 | 20 | 0 | 1 | 81 |
| 4 | 10 | 10 | 10 | 10 | 40 | 1 | 81 |

Cada oleada añade un [zombi con knife Leviathan](CZERO_ZOMBIE.md), con el
modelo de la carpeta 0 del RAR. Aparece en un punto CT libre al azar, cuenta
como enemigo y conserva compatibilidad con los efectos de Cremation.

La primera oleada comienza cinco segundos después de entrar un jugador vivo.
Se intenta generar un enemigo cada 0,375 segundos, alternando norte, este,
sur y oeste y rotando los diez puntos de cada frente. Cada frente recibe
10 enemigos en las dos primeras oleadas y 20 en las dos últimas. Los tipos
también se reparten: en la cuarta, diez cultistas por frente y dos o tres de
cada tipo de zombi, rotando el sobrante para mantener iguales los totales.
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
- `powershell -ExecutionPolicy Bypass -File tools/build_mansion.ps1`: compila solo servidor, pruebas y lanzador; no ejecuta el pipeline general de armas.
- `python tools/check_mansion_runtime.py`: prueba acotada de las cuatro oleadas con muertes por daño nativo.
- `python tools/check_mansion_runtime.py --routes`: 20 recorridos físicos desde los cuatro frentes, con los cinco tipos de enemigos.

Las pruebas de motor usan exclusivamente su proceso de servidor dedicado y la
variable `CREMATION_MANSION_VALIDATE`; el juego normal no activa ese modo.
Evidencia en `diagnostics/mansion-*.json` y `diagnostics/mansion-*-server.log`.
La revisión visual del cielo, HUD y combate requiere una sesión interactiva.
Reiniciar completamente Half-Life e iniciar una partida nueva para usar el mapa.

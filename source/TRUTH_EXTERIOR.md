# Exterior de Truth para Mansion

Recursos originales de `czero/maps/de_truth_cz.bsp` y `czero/czde_truth.wad`. La textura del suelo bajo los CT, comprobada en el BSP de Truth a Z=-96, es `jpn_walk43`. `truth_textures.wad` es una copia íntegra del WAD original; las texturas quedan embebidas al compilar el mapa.

Materiales aplicados:

| Superficie | Textura de Truth |
| --- | --- |
| Suelo exterior de la mansión | `-0out_grss1`, césped original de Mansion |
| Suelo de los spawns | `jpn_walk43` |
| Suelo de ambos pabellones | `woods_plank_org`, igual que los puentes |
| Fachada exterior y perímetro | `brk_bld_yl_1` |
| Tejados | `rf_tile_1`, con remates `rf_tile_2` |
| Chimeneas y paredes verticales del pozo | `WWD003` |
| Puertas y puentes | `woods_plank_org` |
| Muros de los spawns y casas pequeñas | `outter_wall3` |
| Vegetación sobre el perímetro | `{bam_trees03`, transparente |

El perímetro de ladrillo llega a Z=280, 64 unidades por encima de los muros de los spawns (Z=216). Encima lleva la pantalla de árboles original de Truth, sin colisión. Otra capa está 128 unidades detrás y 64 más abajo, con el patrón desplazado 256 unidades para no repetir los huecos. El cerramiento del cielo se amplía 192 unidades hacia fuera para alojarla; el área jugable conserva sus límites. Conserva el cielo `city` y el pozo radioactivo con sus luces y muerte instantánea.

Se añaden un arce rojo, un arce anaranjado, cinco `bush_japan_bush01` y dos `rock_japan_rocks2`, sin eliminar el jeep y los tres árboles de Vostok. Los modelos se copian byte a byte; `assets-source/truth-exterior/props.json` registra sus posiciones y hashes. El árbol rojo original tiene una sola pose; se conservan las secuencias originales de los demás modelos. Las rocas y los troncos tienen colisiones estrechas; los arbustos son permeables para evitar atascos.

Las dos islas son ahora pabellones abiertos: se eliminan los bloques de las casitas originales (paredes, puertas y techo intermedio). Se conservan postes cobrizos, aleros y tejados inclinados con remates elevados; el suelo es la misma madera clara de los puentes. Las plataformas siguen en las diagonales; sus destinos están en Z=257 para aterrizar sobre el tejado. No cambian los puntos de armas, powerups o portales.

El interior de la mansión conserva geometría, mobiliario, materiales y luces, con la excepción explícita de las puertas cobrizas solicitadas. `preserve_mansion_interior.py` conserva las muestras originales de iluminación de 1.116 superficies interiores; 58 requieren trasladar su rejilla de muestras porque el compilador divide los polígonos de otra forma. La referencia está comprimida en `assets-source/truth-exterior/interior-lightmaps.json.gz`; no depende del backup para recompilaciones posteriores. `material-changes.json` permite auditar cada sustitución.

Generación: `python Cremation/tools/build_mansion_cross.py`. Validaciones: `verify_mansion.py`, `check_truth_exterior.py`, `check_reactor_runtime.py --client` y `check_mansion_runtime.py --routes`. Las capturas reales están en `diagnostics/truth-exterior.png`. Backup previo: `backups/before-truth-exterior/`.

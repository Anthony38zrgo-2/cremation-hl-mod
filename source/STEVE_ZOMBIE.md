# Steve como variante de David

Instalado en `models/cremation/hotd2/steve/` y `dlls/cremation.dll`.

## Comportamiento

Los `monster_zombie` normales se distribuyen entre David y Steve. `cremation_steve_chance` vale `0.5` por defecto: probabilidad independiente del 50 % por aparición, no una cuota exacta por oleada. Valor `0`: David; valor `1`: Steve. La selección afecta a nuevas apariciones. `cremation_david` sigue siendo el interruptor de toda la familia.

Steve conserva la IA nativa, navegación, carrera condicionada por enemigos alrededor del jugador, hitbox general y opciones de daño localizado de David. La postura tumbada por salud continúa desactivada.

La salud inicial de Steve es la salud configurada del David de esa dificultad multiplicada por 1.10, tanto en `health` como en `max_health`. El daño de sus eventos de ataque se multiplica por 1.10 antes de la armadura y resistencias del objetivo. Con los valores actuales: David 50 de vida y ataques de 10/25; Steve 55 y ataques de 11/27.5. Ravenholm trunca a enteros el daño final del jugador sin armadura: se observan 11/27 de salud perdida. Se conserva esa regla nativa.

David y Steve usan la misma selección aleatoria uniforme. Con ambos brazos disponibles, cada uno de los golpes derecho, izquierdo y doble tiene 1/3 de probabilidad. Si falta un brazo, solo se sortean los golpes que puede ejecutar; el evento de contacto se consume una sola vez.

El bloque guardado `CremationDavid1` pasa a versión 3 y añade `variant`. La identidad también se reconoce por el modelo guardado, incluida la versión carbonizada. Cargar una partida conserva la salud existente sin volver a aplicar el aumento del 10 %.

## Modelo y animaciones

Modelo original HOTD2 PC, kind 8, recurso `pol/znsam2b.bin`, 810 triángulos, esqueleto de 16 nodos y pieza flexible de abdomen 8011. Las 27 texturas originales se convierten en 33 materiales de GoldSrc, horneando el color original y la repetición reflejada. Los shorts son opacos hasta las rodillas; las tiras rasgadas conservan el recorte alpha. Se incluyen `steve.mdl`, `steve_charred.mdl` y 13 modelos regionales de partes.

El banco compartido `zom.bin` proporciona espera 0x3CF, caminar 0x3FE, correr 0x3C8 y tres golpes: derecho 0x3EA, izquierdo 0x3E9 y doble descendente 0x3C9. Son los registros originales de Steve kind 8, compatibles con los 16 nodos de David. Se conservan los 26 slots de secuencia. La revisión y las pruebas de selección natural de ataques están en `source/HOTD2_MELEE_FIX.md`.

Los índices de huesos, jerarquía y bases de bodygroups se comparan con David mediante un `static_assert`; las cajas anatómicas y los modelos desprendidos usan los recursos específicos de Steve.

## Conversión y comprobaciones

- `python tools/convert_hotd2_steve.py`: genera e instala únicamente los modelos de Steve y sus constantes.
- `python tools/verify_steve_assets.py`: decodifica el MDL instalado; 76 poses coinciden con los SMD, altura de 72 unidades, 810 triángulos y materiales completos.
- `tools/build_david_zombie.ps1`: compila las pruebas compartidas y el servidor; `-Install` instala el DLL.
- `python tools/check_david_runtime.py`: servidor HLDS aislado; compara estadísticas, valores de daño antes del redondeo, consumo único del evento, preferencia de ataques, serialización y regresiones de David.
- `python tools/check_david_mansion.py`: completa las cuatro oleadas con aparición de David y Steve.

Vista del MDL realmente compilado: `assets-source/hotd2-steve/Steve_compiled_preview.png`. La inspección del render no sustituye la revisión visual del combate en una sesión interactiva.

Copia del DLL anterior a esta incorporación: `backups/before-hotd2-steve/cremation.dll`.

## Revision del cabello

La textura 8 de cabello usa ARGB4444 con 14 niveles alpha intermedios. El primer port aplicaba un umbral de 128 y eliminaba 285 texeles de mechones tenues. Se sustituyo por cobertura alpha mediante tramado 4x4 sobre una imagen de 256x256, manteniendo los texeles totalmente opacos y transparentes. GoldSrc conserva su mascara binaria; la cobertura aproxima la transparencia original con un error maximo de 0.0292. Se corrigio ademas el orden vertical del mosaico reflejado del material opaco de cabello, que era el unico material con ese modo de repeticion.

Se comprobaron las banderas de transparencia del MDL instalado y las 76 poses compiladas. La geometria de la cabeza y sus animaciones permanecen identicas. La frente y la herida superior estan dibujadas opacas en la textura 22 original; la recuperacion de alpha afecta a los mechones externos, no convierte esa zona en cabello. Evidencia: `assets-source/hotd2-steve/hair-alpha-validation.json` y `Steve_hair_comparison.png`.

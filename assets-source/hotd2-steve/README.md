# Recursos fuente de Steve / HOTD2 PC

Extraídos de `D:/games/House_of_the_dead_2_ISO`. Geometría: `DATA.CAB/F13303_pol_znsam2b.bin`. Texturas: `DATA.CAB/F13796_znsam2b.bin`. Esqueleto kind 8 y asignaciones de la piel del abdomen obtenidos de `HOD2.EXE` mediante las herramientas locales de referencia hotd2-re, licencia MIT adjunta.

`Steve_extracted.json` conserva las 37 piezas/variantes, UV, normales, jerarquía y piel original. `textures/` conserva 27 PNG fuente; el canal RGB completo de los shorts se conserva aunque su alpha sea cero. El material original indica opacidad y por ello el exportador ignora ese alpha.

`zom.bin` y su tabla son el banco de movimientos que comparte la familia David/Steve. `hzom.bin` conserva los movimientos de arrastre compatibles. Los hashes de todos los archivos de entrada están en `source-sha256.json`.

Conversión reproducible: `tools/convert_hotd2_steve.py`. No ejecutar el pipeline general de assets. Detalles de integración y balance: `source/STEVE_ZOMBIE.md`.

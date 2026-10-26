# Fuente de David

Recursos originales recuperados de la copia local de HOTD2 PC en `D:/games/House_of_the_dead_2_ISO`:

- `David_extracted.json`: mallas del banco znsam2, UV, normales, materiales, jerarquía y asignaciones originales del abdomen.
- `textures/`: las 28 texturas originales decodificadas, con transparencia.
- `zom.bin` y `motion_table.json`: movimientos originales y tabla de IDs/duraciones extraída del ejecutable.
- `goldsrc/`: SMD, QC, BMP indexados, MDL y registros reproducibles del compilador.
- `conversion-report.json`: escala, referencias originales, secuencias, hitboxes y hashes instalados.
- `compiled-readback.json` y `compiled-preview/`: comprobación y texturas leídas de los MDL finales.

La extracción inicial utilizó los decodificadores MIT de GitFist/hotd2-re, commit `7c48c8e1eafee7d275fb166b287ee0028d72ce1d`; `LICENSE-tools-MIT.txt` conserva su licencia. Los assets pertenecen al juego original. El exportador de Cremation es `tools/convert_hotd2_david.py` y todos sus datos de entrada están en este directorio; no necesita volver a consultar el ejecutable para reconstruir los MDL.

La conversión conserva 16 huesos con una influencia por vértice, incluidos los nodos originales que controlan el abdomen. StudioMDL requiere ajustes explícitos de loops, actividades y máscaras de textura. La orientación final se verifica leyendo el archivo compilado.

Funcionamiento e instrucciones: `source/DAVID_ZOMBIE.md`.

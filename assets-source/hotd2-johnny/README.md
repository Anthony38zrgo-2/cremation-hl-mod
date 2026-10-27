# Johnny — HOTD2 PC

Recovered from the local ISO using its executable resource tables. `Johnny_extracted.json` stores the original 16-node hierarchy, all 44 models in the geometry bank, the 15 rigid body parts, the executable-defined flexible attachment, original UVs/material words, 28 texture descriptors, and 33 decoded clips from `zom.bin` and `ono.bin`, including shared standing deaths.

The source geometry bank resolves to model IDs 7850–7892 plus 585; Johnny kind 0x13 selects 15 rigid parts and deformation model 7852. The assembled rigid parts contain 860 triangles, plus 14 in the skin attachment.

Alpha audit: `Johnny_alpha_audit.json` reports every pixel format, alpha histogram, texture use, and material TSP alpha decision; `Johnny_alpha_audit.png` shows decoded color and alpha masks. ARGB1555 masks retain binary transparency, ARGB4444 retains all decoded alpha levels, and RGB565 is opaque. Materials whose source pass is opaque ignore alpha even when their image contains it.

Raw local source files and SHA-256 values are retained under `source/`; the ISO binaries were not modified.

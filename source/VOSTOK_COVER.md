# Coberturas de Vostok

Se importaron sin modificar los binarios originales de Condition Zero:

- `czero/models/props/tree_noleaves2.mdl`: es el mismo árbol utilizado en el spawn y en A de `de_vostok_cz`; secuencia `idle`, 301 cuadros a 30 FPS. Tres ejemplares, orientaciones 0/90/120 y multiplicadores 1/1/1.3.
- `czero/models/props/jeep-russian.mdl`: jeep de A, secuencia `idle`, dos cuadros a 1 FPS, multiplicador 10 como en Vostok. Se conserva su animación original; no se inventó una animación de conducción.

Copias independientes en `models/cremation/vostok/`. El manifiesto `assets-source/vostok-cover/installed.json` registra coordenadas, recursos, hashes y cajas de colisión. El jeep está en el patio oeste; los árboles cubren los patios sudoeste, norte y este. No ocupan las rutas cardinales, puertas, salida del portal o plataformas de salto.

`tools/vostok_cover.py` añade los elementos al generador reproducible `tools/build_mansion_cross.py`. Las colisiones son brushes BSP sólidos invisibles: troncos estrechos, chasis y cabina del jeep. Bloquean balas y movimiento; las ramas no crean una gran caja invisible. Son coberturas permanentes. `source/vostok_cover.inc` reproduce las secuencias originales mediante entvars nativas y conserva orientación, fase y velocidad en los guardados. No se necesitan DLL de CSZ para dibujarlas.

Validación: `python Cremation/tools/check_vostok_cover.py` compara los recursos byte a byte, abre el cliente real, comprueba los cuatro modelos animados y las trazas de disparos y hulls, y captura cuatro vistas en `diagnostics/vostok-cover.png`. `verify_mansion.py` comprueba la geometría y las rutas existentes. `check_mansion_runtime.py --routes` recorre las cuatro entradas con los cinco tipos de enemigo.

# Zombi de Quake independiente — estado actual

Rapidcore contiene 10 `monster_quake_zombie`, 10 zombis normales morados,
10 fast y 10 poison. Estos últimos,
los HECU y el esqueleto del Necromancer conservan sus modelos e IA anteriores.
El modelo normal `models/zombie.mdl` procede de `purple_zombie.rar`, junto
con su headcrab normal. El cadáver quemado conserva su malla y animaciones.
Los primeros 20 puntos alternan Quake (IDs impares) y normal (IDs pares),
sin moverlos. Los normales conservan la IA nativa de Ravenholm.

## Modelo, escala y colisiones

El modelo actual procede de `Quake/rerelease/id1/pak0.pak`: `zombie.md5mesh`,
`zombie.md5anim` y su textura. Conserva las 198 poses originales y 1262 triángulos.
La escala es 2,0 respecto de Quake: un 20% menor que la versión anterior a 2,5.
Altura medida en la pose inicial: aproximadamente 103,34 unidades.

La conversión mantiene el esqueleto MD5 original para los hitboxes y la mano del
lanzamiento. Las regiones de piel se ajustan al límite de 128 huesos de GoldSrc.
El error medio de piel es aproximadamente 0,058 unidades en las poses de origen.
Las secuencias mantienen la interpolación; solo se congela la pausa en el suelo.

Se invierte el winding de los triángulos antes de StudioMDL. Quake MD5 y el
compilador usan convenciones opuestas: omitir esta inversión dejaba el modelo
con caras vistas desde dentro, ocultadas por el backface culling de GoldSrc.
La verificación relee el MDL compilado y comprueba la misma orientación que
el modelo funcional de SGClassic, además de 24 poses contra el MD5.

Los 15 hitboxes explícitos siguen huesos anatómicos: cabeza, pecho, pelvis,
brazos, manos, piernas y pies. Sus límites se recalculan en el espacio de cada
hueso al convertir y escalar. No son las antiguas cajas automáticas de la piel.
La colisión con el mapa mide 32 × 32 × 104 y el punto de visión está a 80.
Las comprobaciones adicionales de altura evitan atravesar techos bajos.
Se conservan tres correcciones de patrulla, registradas en
`quake-zombie-patrols.json`; los puntos iniciales de los monstruos no se mueven.

## Combate e integración con Cremation

La entidad tiene Think, animaciones, ataques y respuesta al daño propios.
Utiliza la construcción y serialización de CZombie y una tabla virtual clonada,
sin cambiar las tablas de los zombis nativos. Conserva la facción 7 de los
zombis y cultistas; es hostil al jugador y sus aliados minion/sentry.

Lanza restos físicos con 10 de daño y duración de 2,5 segundos. La salida usa
el attachment `hand_R` del modelo; la trayectoria compensa gravedad y altura.
Los impactos ordinarios conservan la regla de Quake: 60 de daño en un fotograma
lo destruyen, 25 lo derriban, y el daño menor se regenera. Permanece cinco segundos
en el suelo y comprueba el espacio antes de levantarse.

El plasma primario de Link Gun y el daño de energía, electricidad y fuego se
acumulan persistentemente, para que la regeneración de Quake no los anule.
Esta acumulación usa `armorvalue`, un entvar guardado por Ravenholm. El rayo
secundario respeta NEVERGIB: selecciona la nueva secuencia `die_fall` y permite
el mismo efecto verde/esqueleto de Link Gun que los otros humanoides.

Las muertes con desmembramiento llaman al mismo `humanGibSpawn` que los demás
monstruos: seis piezas de `models/hgibs.mdl`, física, sangre, sonidos y duración
compartidos. Los restos pequeños de Quake se conservan solo como proyectiles.
La entidad también instala el hook compartido de sangre y define sangre roja.

## Construcción y validación

`tools/convert_quake_zombie_remaster.py` convierte el modelo instalado de Quake.
`tools/verify_quake_zombie_remaster.py` comprueba el MDL compilado, incluyendo
orientación, tamaño, poses y hitboxes. `tools/build_quake_zombie.ps1 -EngineCheck`
instala los recursos privados, recompila solo la DLL de servidor y ejecuta las
pruebas C++, ABI, mapa y motor real. No ejecuta el pipeline general de armas.

La prueba optativa de GoldSrc comprueba impacto del proyectil, trazas de cabeza
y aire sobre el modelo, caída, recuperación, gibs compartidos, daño continuo de
Link Gun y coincidencia del efecto de esqueleto. Solo se activa mediante
`CREMATION_QZ_VALIDATE=1` en su proceso de prueba.

La revisión visual de todas las caras durante el combate y una ronda interactiva
de guardar/cargar requieren aceptación en juego; las pruebas automáticas no las
sustituyen. Reiniciar completamente Half-Life tras actualizar modelos y DLL.

Los verificadores generales antiguos tienen fallos previos documentados:
`verify_mod.py` exige un equipamiento con escopeta que ya no coincide con el mapa;
`test_ricochet.cpp:95` falla en `discs.empty()`, reproducido con el código anterior.
Las pruebas específicas de este zombi se mantienen separadas de esos fallos.

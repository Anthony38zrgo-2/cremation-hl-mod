# Link Gun (native Nailgun carrier)

## Primario

Conserva daño 9, coste 1 y trail verde. Velocidad 750 u/s frente a 1000;
7,5 disparos/s frente a 10 (intervalo 1/7,5 s). Ambas reducciones son del 25 %.

## Secundario

Mantener Attack2 adquiere un enemigo visible a 768 u como máximo. Se prioriza
el impacto directo de la mira; si no lo hay, se elige dentro de un cono de
10 grados, priorizando cercanía a la mira. No se adquieren humanos pasivos ni
aliados nativos del jugador. Conserva el mismo objetivo aunque se mueva la mira.
No cambia a otro objetivo al morir el primero; requiere soltar Attack2.

El enlace visual verde se renueva cada 0,1 s, con chispas e iluminación verde
sobre el punto de contacto. Se usa una unidad de munición por bloque de 0,5 s
(2/s); se paga al empezar cada bloque y solo con conexión efectiva. El daño
empieza en 12/s y aumenta 6/s por segundo efectivo hasta 48/s. Rango o paredes
suspenden daño, partículas en el enemigo, consumo y aumento; se conserva el lock.
Soltar el botón, cambiar de arma, morir o agotar munición detiene el enlace.

## Muerte por enlace

El último tick usa ENERGYBEAM | NEVERGIB para evitar la fragmentación ordinaria.
Al morir se oculta el cadáver original y se crea una representación no sólida
con el scientist.mdl aportado por el usuario, en models/cremation/link_skeleton.mdl.
Usa la secuencia diesimple (31), transparencia y glow verde, y se desvanece
hasta desaparecer tras 1 segundo. El modelo original del enemigo no se sustituye
mientras siga vivo. Se escalan los efectos según la altura del objetivo.
Los FX transitorios no se reanudan al restaurar guardados.

La fuente del modelo se conserva en assets-source/linkgun_skeleton/ y el
instalador de armas puede copiarla de nuevo. Procedencia, parámetros y hash en
linkgun-feature.json. DLL compilada e instalada; sin ejecución de pruebas ni
validación visual/interactiva de este cambio.

## Corrección de visibilidad y forma del haz

Se comprueba la muerte tanto al volver de TakeDamage como al siguiente tick
nativo (hasta 0,35 s desde el último daño del enlace). Se guarda posición,
orientación y altura antes de dañar para soportar eliminación/reutilización
del edict. El esqueleto borra EF_NODRAW, tiene bounding box explícito y origen
3 unidades elevado. Es visible con alpha 210 durante 0,75 s y se desvanece en
los últimos 0,25 s. Se registra LinkSkeletonFX en startup-diagnostics.txt.

El haz se dibuja como una curva Bézier cúbica de 18 segmentos: sale siguiendo
la mira y se curva hacia la posición del enemigo. Sobre esa curva viajan tres
ondas con desplazamientos transversal y vertical, ancladas en ambos extremos.
La ondulación se calcula geométricamente, sin depender de ruido en una línea
recta. Conserva color verde y partículas en el objetivo.
DLL recompilada e instalada; revisión visual en juego pendiente.

Duración actual del esqueleto: 6 s, visible durante 5,75 s y fade final de 0,25 s.

Esqueleto Link Gun: el brillo verde y la transparencia desaparecen durante el primer segundo; después queda el modelo opaco, sin aura verde, hasta el desvanecimiento final de 5,75 a 6 s. DLL recompilada e instalada.

Aspecto actual del enlace: estilo Gluon/Egon con sprites/xbeam1.spr original, envoltura verde aditiva y núcleo verde más brillante, grosor/brillo pulsantes y textura desplazándose. Conserva la curva hacia el enemigo con ondulación suavizada. Impacto sin chispas ni partículas: solo iluminación verde. DLL compilada e instalada; revisión visual pendiente.

Alineación del haz Link Gun: corregido el vector lateral invertido. Se usa right=(sin(yaw),-cos(yaw),0), con up derivado de la orientación de vista; el origen queda 18 unidades al frente, 6 a la derecha y 6 debajo del ojo en ejes de vista. DLL recompilada e instalada; alineación visual pendiente de comprobación.

Proyectil primario Link Gun: núcleo opaco con envoltura verde aditiva translúcida (GlowShell, RGB 35/255/65, intensidad/expansión 22). Conserva la estela verde. DLL recompilada e instalada; revisión visual pendiente.

Modelo del proyectil primario Link Gun sustituido por elipsoide verde luminoso (radios 8/3/3 unidades), alineado con su dirección de vuelo. Mantiene envoltura verde y trail. Generación reproducible en tools/prepare_linkgun_projectile.py; modelo instalado en models/cremation/nail.mdl. Revisión visual pendiente.

Proyectil primario con aspecto de plasma: cuerpo ovalado verde animado de bordes
irregulares, núcleo blanco amarillento y halo verde difuso. Tres sprites aditivos
con 16 fotogramas y pulsación de tamaño, con luz dinámica verde junto a la bola.
La cola se redujo a 0,1 s, anchura 2 y brillo 85. Se conservan velocidad 750 u/s,
daño 9 y cadencia 7,5/s. Los sprites secundarios se eliminan con el proyectil y
no se reanudan en guardados. Generador reproducible:
tools/prepare_link_plasma_sprites.py. Procedencia/hash en link-plasma-assets.json.
DLL compilada e instalada; revisión visual en juego pendiente.

Esqueleto: selecciona una muerte equivalente a la del enemigo por actividad/nombre de secuencia (hacia delante/atrás, cabeza, abdomen/pecho o simple). Sincroniza progreso, posición y orientación con el cadáver nativo, incluida selección de muerte diferida. Si no se identifica equivalencia, usa muerte simple tras 0,3 s. No trasplanta pistas de huesos entre rigs diferentes. Mantiene 6 s de duración y aura verde solo durante el primer segundo. DLL compilada e instalada; revisión en juego pendiente.

Fluidez del esqueleto: se conserva la selección de muerte del enemigo y su duración, pero se deja de copiar su frame de IA por tick. La reproducción avanza por tiempo continuo y activa interpolación del cliente mediante framerate ajustado al modelo. Al terminar conserva la pose final. DLL recompilada e instalada; comprobación visual pendiente.

Enlace secundario inspirado en la referencia: núcleo blanco verdoso de poca
ondulación, envoltura verde difusa y dos filamentos verdes entrelazados con ondas
que avanzan hacia el objetivo. Curva base orientada desde la boca al enemigo.
Halos aditivos animados en la boca y en el contacto, con centro luminoso y luz
verde local. No genera chispas. Los halos se eliminan al soltar/cambiar de arma,
morir, agotar munición o perder alcance; no alteran daño ni consumo del enlace.
Sprites generados por tools/prepare_link_plasma_sprites.py. DLL compilada e
instalada; comparación visual con la referencia pendiente en juego.

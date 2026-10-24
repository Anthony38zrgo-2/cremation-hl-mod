
## Correccion vigente: la patrulla no controla el combate
Se eliminan todas las aiscripted_sequence y sus trigger_auto de patrulla. Incluso
con interrupcion permitida, la cadena repetible podia recuperar al actor y el
estado cinematografico impedia su reaccion normal al jugador. Los 80 destinos
son ahora info_target pasivos, sin posesion, animacion cinematica ni override.
La patrulla usa pasos WalkMove con colision solo mientras no haya contacto:
cede definitivamente a la IA nativa al recibir dano, detectar un jugador vivo
visible a <=2048u o tenerlo a <384u. No altera relaciones ni punteros privados
de enemigo. Se conserva la distribucion inicial; hace falta cargar una partida
nueva, pues los guardados anteriores contienen las antiguas secuencias.
Se anaden pruebas de visibilidad bloqueada/libre, proximidad, dano, cliente muerto
y cesion persistente. La verificacion BSP exige cero secuencias cinematograficas
y cero disparadores de patrulla. Combate visual pendiente de comprobacion manual.

# Correccion vigente: apariciones y patrullas fisicas

Los 40 zombis conservan sus posiciones originales. Se retiran las ordenes de
correr hasta la marca lejana del spawn, que saltaban la fase de movimiento al
no encontrar ACT_RUN y ejecutaban TASK_PLANT_ON_SCRIPT sobre el jugador.
La medida anterior de "40 desplazados" era incorrecta como evidencia de marcha:
incluia teletransportes. Se reprodujo: 40 saltos y 40 dentro del radio del spawn.

Cada actor tiene ahora una patrulla local de ida y vuelta de 64-128 unidades,
con WALK, animacion valida, repeticion y sin movimiento de raiz cinematica.
Se comprueban el casco, el suelo y la separacion respecto a otros actores. Se
mantiene la IA de combate nativa y la distribucion inicial por todo Rapidcore.
No se fuerzan apariciones ni destinos de patrulla en el spawn del jugador.

Las pruebas reales miden pasos fotograma a fotograma y fallan ante teletransportes
o zombis en el radio inicial de 256 del jugador. La marcha se cuenta acumulando
los pasos fisicos, para no confundir una patrulla de regreso con falta de movimiento.
La aceptacion visual y de combate sigue a cargo del usuario.

Los apartados inferiores describen el historial y quedan sustituidos donde
contradigan esta correccion.

# Integración TFC y horda — estado vigente



## Escopeta simple de TFC



Reemplaza weapon_shotgun conservando su identidad de inventario. Modelo local

v_tfc_shotgun.mdl y p_smallshotgun.mdl, con audio shotgn2.wav. Flak mantiene sus

copias privadas de modelos y su propia lógica.



Valores comprobados en la DLL local tfc.dll con pefile y Capstone: constructor

exportado tf_weapon_shotgun RVA 0x6f840, tabla virtual RVA 0xa6ccc, disparo

RVA 0x6f080: 6 proyectiles hitscan de 4 daño, cono X/Y 0.04, alcance 2048,

intervalo 0.5 segundos. Spawn RVA 0x6f5d0 define cargador 8 y recarga por cartucho

0.25 segundos; Reload RVA 0x6f460 prepara 0.1 segundos antes de insertar.

El secundario es vacío en la DLL de TFC y aquí tampoco dispara doble.

No aplica el multiplicador extra de hitgroup de Flak. Ravenholm conserva la

mitigación de sus enemigos. Comparte reserva nativa de buckshot con Flak.



Es una adaptación sobre el motor de Ravenholm; no carga la lógica multijugador

ni las clases completas de TFC. La capacidad de reserva sigue el inventario

compartido de Cremation. La fuente binaria TFC no se modifica ni se distribuye.

SHA-256 de referencia tfc.dll: 9426e7052b79b8024cb7a33d5fe46493d3f0c51cbb71920b80619078bcf3993b



## Sentry adaptada de nivel 1



B / cremation_build: una sentry por jugador, 130 metal, construcción de 4 s.

El jugador empieza con 200 metal y se inmoviliza mientras construye. La posición

requiere suelo plano, espacio libre y visibilidad; no se descuentan recursos en

colocaciones rechazadas. Usa sentry1.mdl y base.mdl de TFC, con 150 HP iniciales,

100 balas, rango 1024 y giro progresivo. Selecciona monstruos enemigos visibles;

no dispara a jugadores ni a otras sentries. Emplea una entidad nativa monster_sentry

como portador de daño; su IA hostil al jugador se sustituye por la selección propia.



J / cremation_repair: cerca de la sentry, repara con metal y repone sus balas

usando cartuchos de la reserva del jugador. N / cremation_dismantle: desmonta

cerca y devuelve 65 metal. Seis baterías identificadas como metal dan 50 unidades

hasta el límite 200; las baterías normales del mapa conservan su uso original.



Los enlaces de teclas se añaden solo si no existen otros enlaces en config.cfg o

default.cfg. `exec engineer.cfg` aplica B/N/J explícitamente si se desea.

No incluye dispensadores, mejoras a niveles 2/3 ni la clase completa de ingeniero.

Las sentries y los estados de cargador propios se reinician al cargar un mapa;

no se serializan como una clase TFC persistente. El bloqueo de construcción no

se guarda para evitar que una carga de partida deje al jugador congelado.



## Zombis



40: 20 monster_zombie, 10 monster_fastzombie y 10 monster_poisionzombie

(la ortografía poisionzombie es el export real del mod). Los archivos zombie01 y

poison_zombie01/02/03 son grupos de animación IDSQ, no enemigos adicionales.

198 nodos; cada zombi tiene una aiscripted_sequence automática hacia el spawn,

sin teletransporte y con interrupción de combate permitida. Los inicios continúan

a más de 1000 unidades y sin visión directa del jugador.



Se detectó ausencia de ACT_WALK en zombie_fast.mdl, pese a que esa ruta de la IA

lo solicita. install_horde_assets.py cambia únicamente la actividad ACT_RUN de

su animación de carrera a ACT_WALK, conservando todos los offsets, geometría,

nombres y datos de animación originales. El archivo fuente de Ravenholm queda

intacto. La prueba de integridad exige longitud idéntica y un solo byte distinto.



Incidencia confirmada: el experimento previo de añadir un descriptor al final del

MDL produjo acceso inválido en hl_ravenholm.dll+0x6dd0 (DispatchAnimEvents). Se

retiró. Una posible causa es el cache/recorte del modelo tras texturas; no se

confirmó con código del motor. La corrección en sitio evita cambiar esa estructura.

Hay además un guardia limitado al rápido: una secuencia inválida se sustituye por

idle antes de entregar el modelo a su lector de eventos.



## Validación



Pruebas simuladas: fragmentos diagonales, supresión por impacto directo, guiado

por puntería de ambos discos, clip/recarga/daño de TFC, colocación/construcción,

recursos, giro/disparo, reparación, desmontaje y recogidas de metal.

Prueba nativa: DLL x86, exportaciones y ABI Ravenholm. Verificación BSP/recursos.

Servidor GoldSrc real: sobrevivió 20 segundos y registró 40/40 zombis desplazados

más de 32 unidades (20 normales, 10 rápidos, 10 venenosos) a los cinco segundos.

La apariencia, manejo, navegación completa y combate jugable requieren prueba manual.



Manual local usado como referencia del ingeniero: tfc/manual/TFCclasses.htm.


La recuperacion de rutas bloqueadas usa WalkMove con colision tras 3 segundos sin avance, sin SetOrigin, y cede al combate nativo si el jugador se acerca. La comprobacion de servidor de 20 segundos confirmo 40/40 zombis en movimiento, cero teletransportes y cero zombis dentro de 256 unidades del spawn.

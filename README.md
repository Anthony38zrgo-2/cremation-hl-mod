# Cremation

Mod para Half-Life / GoldSrc en Windows. Incluye Rapidcore con 20 zombis de
Half-Life2Classic_Ravenholm y un equipo inicial con escopeta, Ricochet, Flak Cannon,
Pipe Wrench de Opposing Force, Nailgun, granada de mano y RPG.

## Instalar y ejecutar

Se necesita Half-Life instalado. Coloca este repositorio en la carpeta
`Half-Life/Cremation` y copia `launcher/Cremation.exe` a `Half-Life/Cremation.exe`,
junto a `hl.exe`. Ejecuta ese lanzador con Half-Life cerrado.
También puedes iniciar `hl.exe -game Cremation -console +exec cremation.cfg`.
No incluye el motor de Half-Life.

El primario de Ricochet ralentiza con efecto azul y su secundario lanza tres
 discos explosivos con empuje. La Flak Cannon lanza 12 perdigones, con
50% más de daño y 15% más dispersión por eje; rebotan dos veces y desaparecen al
tercer contacto con superficies o al impactar un enemigo. Su secundario lanza
una granada cuyo daño aumenta con la elevación del apuntado.

Ricochet usa la Panthergun azul. La Pipe Wrench usa los modelos y sonidos de la chainsaw, con HUD de Opposing Force, con los ataques de la palanca de Half-Life. La
Nailgun usa modelos de TFC/Deathmatch y munición de 9 mm: el primario dispara un
clavo con estela verde; el secundario dispara un Super Nail con estela roja y cuesta
tres unidades de munición.

## Código y compilación

Clona con `git clone --recurse-submodules` o ejecuta
`git submodule update --init --recursive` después de clonar.
El SDK oficial de Valve está fijado como submódulo en `source/hlsdk`.

La compilación actual requiere Windows, Visual Studio 2022 Community con MSVC
x86, .NET Framework C#, Python con Pillow y pefile, y las carpetas de recursos
`valve`, `ricochet` y `Half-Life2Classic_Ravenholm` junto a `Cremation`.
Ejecuta `powershell -ExecutionPolicy Bypass -File tools/build_ricochet.ps1`.
El script compila las pruebas, la DLL y el lanzador, instala recursos y verifica
el mod. El lanzador compilado se escribe en la carpeta padre, junto a `hl.exe`.

Los binarios y recursos instalados se incluyen para usar el mod sin compilar.
La configuración personal, guardados, copias de seguridad y resultados temporales
no se versionan. Consulta `.AGENTS`, `source/RICOCHET_ANALYSIS.md` y
`source/BOUNCE_SHOTGUN.md` para herramientas, incidencias y detalles técnicos.

## Procedencia y validación

Mapa y recursos base: Half-Life de Valve. Zombis y DLL base:
Half-Life2Classic_Ravenholm. La lógica de Ricochet proviene de Ricochet de Valve;
el arma usa ahora la Panthergun azul suministrada por el usuario.
Son recursos de terceros y conservan sus condiciones originales; este repositorio
no les asigna una licencia nueva. La licencia del SDK está en `source/hlsdk/LICENSE`.

Se comprobaron lógica de armas, carga nativa de la DLL, integridad de recursos y
arranque del servidor en Rapidcore. La comprobación gráfica y de combate sigue
pendiente de revisión manual. Los fallos anteriores de exportaciones GoldSrc,
textura externa del modelo, selección de Ricochet y FOV están documentados en `.AGENTS`.

Ricochet tiene tres cargas y regenera una cada 2 segundos. El primario de Flak Cannon consume un cartucho de la reserva compartida de escopeta, sin recarga, cada 0,75 segundos. Su secundario usa dos cartuchos de la misma reserva. Si los doce perdigones de una descarga alcanzan a un enemigo, lo mata y desmiembra. Hay seis cajas de cartuchos adicionales repartidas por Rapidcore.

El balance se edita sin compilar en `config/weapons/ricochet.json` y `config/weapons/flak_cannon.json`. Reinicia el mapa para aplicarlo. Consulta [la guía de configuración](config/weapons/README.md) para parámetros y rangos.

El modelo de primera persona de Flak Cannon es una copia exacta de la escopeta
actual instalada en `models/v_shotgun.mdl`, con sus manos y animaciones. El origen
persistente est� en `assets-source/additional_weapons/current_shotgun/`.
Ricochet usa ahora la Panthergun azul de `panthergun2.zip` y la Pipe Wrench usa la
chainsaw de `d3_chainsaw.zip`, incluidos sus modelos y sonidos.
El primario de Ricochet no sigue la mirada: refleja cinco veces y desaparece en el
sexto contacto con superficies. Su estela tiene anchura 9 (antes 3). El secundario
conserva el seguimiento de la punter�a. La Nailgun conserva su nombre seleccionable
al terminar el spawn nativo de MP5.

Balance vigente: Ricochet regenera cada 1,5 s. Flak hace 9,375 por perdigón; el secundario describe un arco lento, daño nominal 56,25–112,5 según elevación, con radio 48. Se elimina el instakill por doce impactos: el gibbing depende del daño letal. La configuración vigente está en `config/weapons`.


Actualizacion TFC/horda: 40 zombis (20 normales,10 rapidos,10 venenosos) avanzan
hacia el spawn. Escopeta normal: modelo y mecanicas de la simple de TFC.
B construye sentry nivel1,N desmonta,J repara/reabastece;200 metal inicial,
130 por construccion,4s. Recoge las baterias de metal. Ricochet guiado con la
punteria, sin laser,25% mas lento. Flak fragmenta diagonalmente en superficies,
pero no al alcanzar un enemigo. Ver [guia TFC y horda](source/TFC_AND_HORDE.md).

Correccion de aparicion: los40 zombis mantienen sus posiciones iniciales y realizan patrullas locales fisicas. Se retiran las marcas que los concentraban en el spawn. Inicia una partida nueva para cargar el mapa corregido.

## Modelos y recursos de armas

Se instalaron los modelos, sonidos y sprites HUD del paquete `unreal_pack.zip`;
`models/v_shotgun.mdl` conserva la escopeta del pack y la Flak Cannon usa su modelo
de escopeta actual en `models/cremation/v_bounce_shotgun.mdl`, con sus manos y animaciones. La granada usa ahora los cuatro MDL de
`graneda_tnt_by_ben.rar`. La Panthergun azul se asigna a Ricochet y la chainsaw a Pipe Wrench en rutas propias.
La Nailgun incluye modelos, sonidos y sprites HUD de TFC/Deathmatch. Los hashes y
copias anteriores quedan registrados en `additional-weapons-assets.json` y
`file-manifest.json`; los recursos del pack siguen documentados en
`unreal-pack-assets.json`.

El equipo inicial de Rapidcore conserva las armas actuales y añade Pipe Wrench,
Nailgun, granada y RPG, además de una caja de munición para la Nailgun. Estos
cambios se compilaron; la comprobación visual y de combate en una sesión del juego
sigue pendiente.

Actualizaci�n comprobada: regresiones de trayectoria, rebotes, coste 1/3 de Nailgun y nombre seleccionable; arranque de GoldSrc durante 8 s con Nailgun y Pipe Wrench presentes. La revisi�n visual interactiva queda pendiente.


Corrección de transferencia de sonidos: se instalaron los cinco nombres WAV heredados que solicita el DLL de Ravenholm. Como sus archivos originales no están en los paquetes disponibles, cada ruta usa una copia de un efecto compatible ya instalado; el servidor conserva los nombres e índices de sonido originales. El origen y los hashes quedan en `additional-weapons-assets.json` y `file-manifest.json`.

Corrección de carga (24 octubre 2026): las seis rutas de precarga de la chainsaw usan cadenas permanentes. GoldSrc conserva estos punteros para enviar los recursos al cliente; las cadenas temporales generaban nombres de sonido corruptos y bloqueaban la entrada. Se retiraron los filtros de sonido de la corrección anterior. La carga real con hl.exe completó los recursos y registró la entrada del jugador en Rapidcore; evidencia en diagnostics/qconsole-static-sound-fix.log y diagnostics/sound-transfer-fix.json.

Ricochet actualizada: modelo adelantado 6 unidades con las mismas animaciones. El secundario mantiene una variante roja opaca y una envoltura roja aditiva hasta disparar el primario; el modo persiste al guardar el arma durante la sesión. Cuatro cargas regenerables. Trails: primario anchura 12/vida 1 s; secundario anchura 10/vida 0,9 s. El secundario conserva guía, ralentiza a cada enemigo tocado y refleja hasta cinco veces; la explosión ocurre al sexto contacto. Variantes reproducibles con tools/prepare_ricochet_model.py. DLL compilada e instalada; aspecto y combate pendientes de revisión en juego.

Corrección de cierre y selección Flak: el modelo rojo mantiene todas las estructuras de estudio antes de texturedataindex, ya que GoldSrc descarta la cola de píxeles tras subir texturas. La Flak publica su índice real de buckshot al registrar la escopeta, antes de intentar seleccionarla desde el HUD; también migra weapon_crossbow al restaurar partidas. Validado con cliente real: cambio rojo/azul, selección desde slot3 y ambos disparos de Flak, sin cierre durante 28 s. Pruebas test_ricochet pasadas. Evidencias: diagnostics/ricochet-crash-flak-fix.json.

Corrección de modos Ricochet: el primario no guiado ralentiza a cada enemigo que toca y conserva hasta cinco rebotes (desaparece al sexto contacto). El secundario conserva la guía hacia la mira y explota al primer contacto, sin rebotar ni aplicar ralentización. Los aspectos azul/rojo y las cuatro cargas se conservan.

Ricochet: el primario atraviesa los enemigos sin desviar su trayectoria ni consumir rebotes. Un barrido de cada segmento aplica ralentización a todos los enemigos cruzados antes del primer obstáculo sólido; cada proyectil afecta una vez a cada enemigo. Las paredes conservan los cinco rebotes. Ambos modos emiten cinco chispas aditivas junto al disco cada 0,1 s, limitadas al PVS, con RGB idéntico a sus trails: azul (60,145,255) y naranja (255,150,40). El secundario conserva guía y explosión al contacto. DLL compilada e instalada; revisión visual pendiente.

Flak Cannon: el proyectil secundario lleva trail naranja RGB (255,150,40), anchura 10 y vida 0,9 s, acompañado por cinco chispas naranjas cada 0,1 s durante su vuelo, limitadas al PVS. DLL compilada e instalada; revisión visual pendiente.

La Nailgun usa en primera persona v_hgun.mdl del pack Unreal. Animaciones adaptadas al modelo: despliegue 4, disparos 5 y reposo 0. Conserva lógica, munición y trails verde/rojo de Nailgun. Fuente en assets-source/additional_weapons/unreal_hive_hand/.

Ricochet: el modo primario azul incorpora la misma envoltura aditiva del secundario, en celeste (3,65,90). La superficie del arma permanece opaca. Al disparar el primario se cambia del modo rojo a este modelo celeste; el secundario conserva la envoltura roja. Generación reproducible en tools/prepare_ricochet_model.py.

Nailgun: conserva la geometría y texturas de Hive Hand Unreal; sus secuencias se adaptan de la ametralladora Unreal (models/v_9mmar.mdl). Retarget de movimientos Main/Minigun/Muzz al rig Zero/Base/MuzM, con traslación de Camera añadida a Base; conserva la pose de referencia Hive Hand. Se mantienen índices de despliegue 4, disparo 5 y reposo 0. Fuente y generador reproducibles en tools/prepare_nailgun_animation.py. Revisión visual en juego pendiente.

Corrección de animación Nailgun: el donante es ahora la MP5 original de Half-Life (valve/models/v_9mmar.mdl), persistido como v_mp5_motion.mdl. Se adapta movimiento global Bone01 y del conjunto arma/manos Bone04 al rig de Hive Hand; se conservan geometría y texturas Hive Hand, y tiempos/fotogramas originales de las secuencias MP5. Sustituye la adaptación anterior de la ametralladora Unreal. Revisión visual pendiente.

Nailgun: corregida la orientación del retarget MP5 mediante transformación de rotaciones entre los ejes de referencia de ambos esqueletos. El retroceso angular se aplica en el eje de elevación en vez de copiar el Z local de MP5 como giro lateral de Hive Hand. Modelo regenerado e instalado; revisión visual pendiente.

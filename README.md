# Cremation

Mod para Half-Life / GoldSrc en Windows. Incluye Rapidcore con 20 zombis de
Half-Life2Classic_Ravenholm, HEV, escopeta original, Ricochet y Flak Cannon.

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
Half-Life2Classic_Ravenholm. Modelos y recursos del arma Ricochet: Ricochet de Valve.
Son recursos de terceros y conservan sus condiciones originales; este repositorio
no les asigna una licencia nueva. La licencia del SDK está en `source/hlsdk/LICENSE`.

Se comprobaron lógica de armas, carga nativa de la DLL, integridad de recursos y
arranque del servidor en Rapidcore. La comprobación gráfica y de combate sigue
pendiente de revisión manual. Los fallos anteriores de exportaciones GoldSrc,
textura externa del modelo, selección de Ricochet y FOV están documentados en `.AGENTS`.

Ricochet tiene tres cargas y regenera una cada 2 segundos. El primario de Flak Cannon consume un cartucho de la reserva compartida de escopeta, sin recarga, cada 0,75 segundos. Su secundario usa dos cartuchos de la misma reserva. Si los doce perdigones de una descarga alcanzan a un enemigo, lo mata y desmiembra. Hay seis cajas de cartuchos adicionales repartidas por Rapidcore.

El balance se edita sin compilar en `config/weapons/ricochet.json` y `config/weapons/flak_cannon.json`. Reinicia el mapa para aplicarlo. Consulta [la guía de configuración](config/weapons/README.md) para parámetros y rangos.

Balance vigente: Ricochet regenera cada 1,5 s. Flak hace 9,375 por perdigón; el secundario describe un arco lento, daño nominal 56,25–112,5 según elevación, con radio 48. Se elimina el instakill por doce impactos: el gibbing depende del daño letal. La configuración vigente está en `config/weapons`.


Actualizacion TFC/horda: 40 zombis (20 normales,10 rapidos,10 venenosos) avanzan
hacia el spawn. Escopeta normal: modelo y mecanicas de la simple de TFC.
B construye sentry nivel1,N desmonta,J repara/reabastece;200 metal inicial,
130 por construccion,4s. Recoge las baterias de metal. Ricochet guiado con la
punteria, sin laser,25% mas lento. Flak fragmenta diagonalmente en superficies,
pero no al alcanzar un enemigo. Ver [guia TFC y horda](source/TFC_AND_HORDE.md).

Correccion de aparicion: los40 zombis mantienen sus posiciones iniciales y realizan patrullas locales fisicas. Se retiran las marcas que los concentraban en el spawn. Inicia una partida nueva para cargar el mapa corregido.

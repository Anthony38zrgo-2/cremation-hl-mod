# HECU: voces de cultistas de Blood

## Recursos y alcance

Se instalaron las 50 entradas SFX vocales disponibles de las familias CULT y TCLT
(cultista de escopeta y fanático de Tommy gun): alerta, búsqueda, dolor, muerte,
victoria/burla y fuego. Se incluyen las variantes que el azar del Blood original
no selecciona normalmente. No se incluyen sus sonidos de armas/TNT.
Cada HECU mantiene una familia vocal: escopeta usa CULT; los demás usan TCLT.
Todas las variantes de cada evento pueden seleccionarse sin repetición inmediata.

| Evento | Cultista | Fanático | Activación en Cremation |
|---|---|---|---|
| Alerta | 1003–1007 | 4003–4007 | Nuevo enemigo, aviso de peligro, carga/ataque |
| Búsqueda | 1008–1012 | 4008–4012 | Patrulla, enemigo perdido, conversación nativa |
| Dolor | 1013–1017 | 4013–4017 | Daño recibido sin fuego |
| Muerte | 1018–1020 | 4018–4020 | Muerte, incluida muerte por daño propio del mod |
| Victoria | 1021–1024 | 4021–4024 | El enemigo que seguía el HECU muere |
| Fuego | 1031–1033 | 4031–4033 | Daño BURN/SLOWBURN y muerte por fuego |

La selección de alertas y búsquedas también usa los eventos nativos HG_ALERT,
HG_MONST/HG_MONSTER, HG_GREN, HG_THROW, HG_CHARGE, HG_TAUNT, HG_COVER, HG_CHECK,
HG_QUEST, HG_IDLE, HG_CLEAR y HG_ANSWER. Se añadieron 17 grupos de frases a
sound/sentences.txt para registrar esos eventos antes ausentes. El hook sustituye
las voces nativas, conserva el tipo de voz por HECU y prioriza dolor/muerte frente
a conversación. La muerte interrumpe cualquier otra voz. Hay límites temporales
por personaje y un límite de conversación ambiental entre personajes.

Se conserva la alianza/facción HECU-zombis y la lógica de combate de Ravenholm.
La extensión observa el EHANDLE nativo del enemigo mediante metadatos de guardado,
y envuelve TakeDamage en el mismo slot ABI ya usado por el mod para aplicar daño;
llama al método nativo y respeta su resultado. Los gritos de fuego se disparan por
el tipo real de daño, no por una probabilidad durante combate normal.

## Investigación

Fuentes primarias consultadas:

- [NBlood ai.cpp](https://github.com/NBlood/NBlood/blob/master/source/blood/src/ai.cpp): activación con/sin objetivo, dolor y fuego.
- [NBlood aicult.cpp](https://github.com/NBlood/NBlood/blob/master/source/blood/src/aicult.cpp): burla cuando el objetivo ha muerto.
- [NBlood actor.cpp](https://github.com/NBlood/NBlood/blob/master/source/blood/src/actor.cpp): voces de muerte.
- [NBlood resource.h](https://github.com/NBlood/NBlood/blob/master/source/blood/src/resource.h) y resource.cpp: directorio RFF y descifrado.
- [NBlood sound.cpp](https://github.com/NBlood/NBlood/blob/master/source/blood/src/sound.cpp): tasas de muestreo SFX.

No se inventaron traducciones de las frases en el lenguaje de los cultistas:
la correspondencia se basa en los eventos que realmente las reproducen.

Los SFX 4023/4024 de esta instalación apuntan a RAW 2008/2002 ausentes en ambos
sounds.rff (Fresh Supply y DOS). Se usan las variantes presentes 2008Z/2002Z de
la misma frase, con el pitch de la familia TCLT. Esta resolución está documentada
por clip en blood-cultist-voices.json.

## Reproducción de recursos y estado

Origen: D:/SteamLibrary/steamapps/common/Blood/sounds.rff.
Extracción reproducible: tools/install_blood_cultist_voices.py.
RAW/SFX extraídos en assets-source/blood_cultist/; WAV en sound/blood/cultist/.
Se conservan el pitch base de cada SFX y las voces originales; se convierte PCM8
sin cabecera a WAV mono PCM16/22050 Hz. Las rutas precargadas tienen vida estática.
Procedencia, duración, ID, variante y hashes: blood-cultist-voices.json.

DLL compilada e instalada con las 534 exportaciones de Ravenholm. Sin ejecución
de pruebas ni sesión de escucha/combate realizada en este cambio. Reiniciar el
juego para recargar sound/sentences.txt y las voces. Cambios locales posteriores
al commit publicado 253cd0e; esta feature todavía no tiene commit propio.

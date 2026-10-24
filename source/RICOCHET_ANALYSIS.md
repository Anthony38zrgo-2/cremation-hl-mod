# Actualización vigente: guiado de puntería

Velocidad 750 u/s (25% menos que 1000). Ambos disparos siguen continuamente el
punto hacia el que apunta el dueño, como un RPG sin entidad/sprite de láser rojo.
No elige automáticamente un enemigo. Giro limitado por guidance_turn_rate en
radianes/segundo, incluso tras un rebote; mantiene la velocidad configurada.
El algoritmo de giro resuelve también cambios de dirección de 180 grados.
Regeneración: tres cargas, una cada 1,5 s. El resto inferior es análisis histórico.

# Ricochet en Cremation

Referencia primaria: [código oficial de Valve](https://github.com/ValveSoftware/halflife/tree/master/ricochet).
La copia consultada está en `hlsdk/ricochet`. Los modelos y sonidos proceden de las
instalaciones locales de Ricochet y Half-Life; `../ricochet-assets.json` registra cada copia.
Revisión del SDK consultada: `b1b5cf5892918535619b2937bb927e46cb097ba1`.

## Comportamiento confirmado en el original

- `dlls/discwar.h`: velocidad de disco 1000, empuje 1200, Freeze dura 7 segundos
  y fija la velocidad máxima del jugador en 50 frente a 320 normalmente.
- `dlls/player.cpp`, `CBasePlayer::Freeze`: brillo azul (`kRenderFxGlowShell`,
  componente azul 200 y amplitud 25), velocidad limitada y temporizador de 7 segundos.
  El original aplica esto a jugadores, no a los monstruos de Half-Life.
- `dlls/wpn_shared/disc_weapon_disc.cpp`, `DiscTouch`: Freeze se aplica si el jugador
  no estaba congelado. Otro contacto puede empujarlo; la decapitación es otra rama.
- `DiscThink`: antes del primer rebote, Freeze busca enemigos en un cono con producto
  escalar mayor de 0.6 y corrige la dirección sumando 0.25 de la dirección al objetivo.
- `FireDisc`: Triple Shot añade dos discos al central, con yaw -7° y +7°; todos salen
  simultáneamente. **Triple Shot no incorpora una explosión radial por sí mismo.**
- El secundario original usa discos decapitadores. Ni esa decapitación ni el sistema
  de puntuación de las arenas se incorporan a Cremation.

## Adaptación implementada

El primario lanza un disco de Freeze a 1000 unidades/s. La corrección suave mantiene
el cono y el factor 0.25, restringidos además a un enemigo visible dentro de 1024
unidades. Se elimina al alcanzar a un enemigo o tras tres rebotes; no consume munición.

En los zombis, limitar `pev->maxspeed` no basta: su IA desplaza la entidad mediante
`WalkMove` y `MoveToOrigin`. Se escala la distancia de esas llamadas al 25%, además
del `framerate` que usa la animación y sus eventos. No se suspende la IA. El efecto
dura 7 segundos y un nuevo impacto renueva el plazo. La representación combina un
brillo azul con transparencia 175/255. Se restauran los valores previos al expirar,
morir, guardar o desactivar el servidor.

El secundario conserva el patrón simultáneo de tres discos a 0°/-7°/+7°, añadiendo
la explosión solicitada. Cada disco detona en su primer impacto y alcanza enemigos
con línea de visión dentro de 210 unidades. Daño máximo 4 por disco, con caída por
distancia: una ráfaga completa tiene un techo bruto de 12. La onda aplica hasta 850
unidades/s horizontales y 190 verticales; el acumulado horizontal se limita a 1200.
El enemigo pasa temporalmente a física de caída para que su navegación no anule
el empuje. Sus desplazamientos de IA se bloquean durante ese intervalo, no su
pensamiento. El arma no aplica daño o empuje a su propio jugador.

Los valores están en `ricochet_balance.h`. Hay un límite de 24 proyectiles activos.
Se trazan segmentos completos por paso para evitar que los discos atraviesen
objetivos debido a la velocidad; las explosiones comprueban oclusión por paredes.

## Integración

`cremation.dll` carga `hl_ravenholm.dll`, una copia exacta de la DLL local de Ravenholm.
Conserva sus 524 exportaciones y reenvía sus entidades y funciones de guardado mediante
saltos Win32 que conservan la convención de llamada. Añade `weapon_ricochet` y
`cremation_disc`. La tabla de nombres mantiene completos los símbolos decorados de
las funciones heredadas para que el guardado pueda recuperarlos.

El inventario y HUD reutilizan el identificador de pistola (2), con nombre
`weapon_ricochet`, icono propio y grupo 2 (slot 1 de base cero). El comando de selección
conserva ese nombre; el proxy lo restituye después de que `CGlock::Spawn` lo cambie
a `weapon_9mmhandgun`, y migra ese nombre al restaurar guardados antiguos.
Se filtran los ataques y la recarga de pistola
antes de que el servidor los procese. `cl_lw 0` desactiva la predicción heredada de
balas; las animaciones de disco se envían desde el servidor. Los modelos originales
de Ricochet están copiados bajo los nombres de modelos que espera ese inventario.

La escopeta y la lógica de los zombis siguen en Ravenholm. El BSP sólo contiene como
armas recogibles `weapon_ricochet` y `weapon_shotgun`, con HEV y cartuchos. Sus otros
14 bloques, incluido el mapa visible, se mantienen idénticos a Rapidcore original.

Los discos en vuelo y los estados temporales expiran al cargar guardados. El guardado
usa aspecto y tipo de movimiento normales; el estado transitorio se restituye al
continuar la sesión actual. No se garantiza la persistencia de un proyectil en vuelo.

## Validación y límites

- `test_ricochet.cpp`: motor simulado, lanzamiento primario, impacto, ralentización,
  animación sin acumulación, recuperación, transparencia, patrón triple, daño bajo,
  empuje limitado, guardado normalizado, HUD, selección, filtrado de pistola,
  cadencia, límite de proyectiles y conservación del ataque de escopeta.
- `test_native.cpp`: carga la DLL Win32 real sin arrancar el juego, obtiene la API
  140, resuelve el constructor heredado de zombi y comprueba su interfaz base.
- `verify_mod.py`: posiciones de los 20 zombis, equipo, bloques BSP, exportaciones,
  arquitectura Win32 y hashes de los recursos respecto de sus fuentes.

La sensación, animaciones finales, apariencia azul, empuje en el mapa y compatibilidad
completa de guardados requieren la prueba manual del usuario. Estas pruebas no
equivalen a una sesión jugable validada.

## Corrección del cierre al iniciar

Windows registró una violación de acceso en `hw.dll+0x1e13f0` durante el arranque.
Ese punto pertenece al lector de nombres exportados del motor. El lector convierte
las direcciones del array y de las cadenas usando exclusivamente la sección `.rdata`;
la primera compilación situaba los nombres añadidos en una sección aparte. Por eso
la prueba nativa de Windows pasaba aunque GoldSrc fallase al examinar el archivo.

La compilación ahora reserva espacio en `.rdata` y mantiene allí el array y todas
las cadenas contiguas. `verify_mod.py` reproduce ese lector antiguo para evitar la
regresión. `check_server_startup.py` confirmó además, con el motor real sin interfaz
gráfica, las fases `worldspawn` y `ServerActivate rapidcore`, y ocho segundos de
ejecución sin cierre. La prueba gráfica y de manejo sigue siendo manual.

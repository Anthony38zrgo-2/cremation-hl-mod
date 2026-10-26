# HUD de bomba CS Zero y artefactos DMC

Se conservan el cliente Ravenholm, sus exports, el radar y los indicadores de salud/armas. El proxy añade una capa en `HUD_Redraw`, tanto en el export directo como en la tabla `F`. Se limpia en `HUD_Reset` y `HUD_VidInit`.

## Recursos originales

`tools/install_tactical_hud.py` copia sin modificar los sprites a `sprites/cremation/hud/`. Registra procedencia y hashes en `assets-source/tactical-hud/installed.json`.

- CS Zero hereda `cstrike/sprites/640hud7.spr`: iconos `bombticking`, `bombticking1`, `c4`, `defuser`, `stopwatch` y dígitos, con los recortes de `cstrike/sprites/hud.txt`.
- DMC: `icnquad.spr`, `icninvin.spr`, `icninvis.spr` y su `640hud7.spr`. Los iconos se apilan a la izquierda, respetando el espacio del radar. Los segundos usan los dígitos originales de DMC y los iconos parpadean en los últimos tres segundos.
- DMC no incluye un icono dedicado de estado para el Biosuit: se usa su propio `suit_full`, procedente del mismo HUD, en verde. Los otros tres artefactos conservan sus iconos dedicados originales.

## Presentación

La bomba plantada muestra aviso temporal, icono animado y temporizador inferior central. El contador pasa a rojo en los últimos diez segundos. Todos los jugadores vivos muestran su kit de desactivación. Solo el jugador que está desactivando ve la barra de progreso CS; al soltar USAR, moverse o perder la acción se cancela. La desactivación y la explosión muestran confirmaciones de seis segundos. La antigua impresión numérica en el centro se sustituye por esta UI.

Los artefactos muestran su icono, color y tiempo restante. Desaparecen al expirar, al morir, al cambiar de mapa o cuando el servidor deja de enviar el estado. `hud_draw 0` también los oculta. Los relojes se reconstruyen desde la autoridad del servidor al cargar una partida.

## Protocolo

Mensaje independiente `TacticalHUD`, versión 1, tamaño fijo 17 bytes por jugador a 5 Hz:

| Campo | Tamaño |
| --- | --- |
| Versión y estado de bomba | 2 bytes |
| Tiempo de bomba en décimas y progreso sobre 1000 | 4 bytes |
| Flags de interacción y kit | 2 bytes |
| Cuatro duraciones de artefactos en décimas | 8 bytes |
| Jugador vivo | 1 byte |

Estados: 0 sin bomba, 1 defensor vivo, 2 puede desactivar, 3 desactivando, 4 desactivada, 5 explosión. Los flags indican acción propia, bloqueo por defensor y proximidad. El cliente interpola la presentación, no controla el resultado. Descarta mensajes incompatibles y oculta datos tras 1,2 segundos sin actualización.

## Validación

- `check_tactical_hud.py`: cliente real, recepción y dibujo de los seis estados, cancelación, expiración y muerte; capturas en `diagnostics/tactical-hud.png`.
- `check_tactical_save.py`: crea un guardado y lo carga en un proceso nuevo; verifica también la entrega de bomba y artefactos restaurados al HUD.
- Compilación e instalación: `build_mansion.ps1` y `build_client_effects.ps1`.

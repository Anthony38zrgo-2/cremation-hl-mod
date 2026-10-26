# Artefactos originales de Deathmatch Classic

La zona de powerups ofrece únicamente los cuatro artefactos de DMC. Los modelos, sus texturas externas y los sonidos se copian sin modificación desde la instalación de Steam. Procedencia y hashes: `assets-source/dmc-powerups/installed.json`. Instalador reproducible: `tools/install_dmc_powerups.py`.

La adaptación se basa en `source/hlsdk/dmc/dlls/quake_items.cpp` y conserva los tiempos originales: efectos durante 30 segundos; reaparición de Quad y traje en 60 segundos, e invulnerabilidad e invisibilidad en 300. El siguiente artefacto se elige al azar. Giran y flotan usando los modelos originales, con su animación y apariencia correspondiente.

- Quad Damage: multiplica por cuatro el daño de las armas contra monstruos y jugadores.
- Invulnerabilidad: impide el daño ordinario.
- Invisibilidad: representación translúcida y exclusión de la adquisición de objetivos por los monstruos; restaura el estado anterior al terminar.
- Traje protector: protección contra ácido, radiación, quemaduras ambientales y ahogamiento.

El fondo del reactor y la bomba siguen siendo objetivos letales, también con artefactos activos. No se incluyen botiquines, baterías ni módulos de salto largo en este punto. El jugador recibe el salto largo nativo y kit de desactivación desde el inicio y al reaparecer.

Los efectos usan entidades de estado serializadas; no dependen de offsets privados del jugador Ravenholm. El hook de daño conserva su tabla virtual original para no alterar RTTI. La precarga de las extensiones también se ejecuta al restaurar el mundo; las partidas guardadas pueden cargarse en un proceso nuevo sin perder los modelos personalizados.

# Link Gun

El haz secundario conserva 21 DPS iniciales y duplica el daño cada 1,5 segundos de conexión, con máximo de 336 DPS a los 6 segundos. Mantiene el impacto primario de 15,75. Enlaza hasta cinco enemigos en total, conectando vecinos a no más de 128 unidades y con línea de visión; no atraviesa paredes ni incluye aliados. Cada víctima recibe un único impacto por tick y conserva sus efectos y muerte nativos. El haz puede continuar con otro objetivo al morir el principal.

Pruebas: `check_czero_tactical.py`, `check_reactor_runtime.py --client`, `check_tactical_save.py`. Revisión visual: `capture_dmc_preview.py` genera `diagnostics/dmc-powerups.png`.

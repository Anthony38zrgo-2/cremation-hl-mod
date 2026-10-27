# Alianza entre enemigos y bloqueo de fuego amigo

La regla común está en `enemy_combat_policy.inc` y se aplica después de adaptar cada NPC, tanto al aparecer como al restaurarlo. La IA trata a los NPC enemigos como aliados (`R_AL`), incluso cuando sus clases originales pertenecen a facciones diferentes. Antes de cada `Think` se eliminan objetivos enemigos que pertenecen a esa alianza, incluido el `m_hEnemy` nativo leído mediante su metadato de guardado.

El daño se rechaza antes de ejecutar represalias, dolor, defensa de Johnny, mutilación y efectos posteriores. Se cubren el punto común `weaponBloodDamage` y las entradas propias de David/Steve/Johnny, Quake, CZ y cultistas. El `TraceAttack` de David también lo rechaza antes de consumir la defensa de Johnny.

La atribución distingue atacante e inflictor. El atacante explícito conserva su crédito; cuando es el mundo o falta, se recorre la cadena de propietarios del proyectil. Así una granada de un enemigo no daña a otro enemigo aunque el ataque llegue con el mundo como atacante. El jugador, sus invocaciones y sus proyectiles conservan el daño. El daño ambiental y las autolesiones programadas siguen disponibles.

La política modifica únicamente la relación en el slot 63 de las tablas existentes. No añade una copia exterior a las tablas de daño: los adaptadores actuales buscan sus originales por identidad de tabla. El retorno nativo hacia jugadores y aliados usa el `CBaseMonster::IRelationship` del donante comprobado por firma; una DLL donante distinta genera un diagnóstico de ABI.

## Comprobación

`python tools/check_enemy_combat.py` abre un HLDS oculto e independiente. Comprueba:

- David, Steve, Johnny, Quake, rápido, venenoso, cultista HECU, CZ y headcrab.
- Las 72 relaciones dirigidas entre tipos distintos y 360 impactos de bala, melee, explosión, ácido y fuego.
- Protección de la defensa de Johnny ante impactos amigos.
- Propietarios de proyectiles anidados y prioridad del crédito explícito del jugador.
- Daño del jugador hacia los nueve tipos y daño de enemigos hacia jugador e invocación.
- Conservación de daño ambiental y autolesiones programadas.
- Limpieza de objetivos aliados guardados y reinstalación idempotente.

`python tools/check_hotd2_deaths.py` comprueba además que la política no rompe la continuidad y las poses finales de las muertes. Evidencia: `validation-enemy-combat-diagnostics.txt` y `validation-hotd2-deaths-diagnostics.txt` en la raíz del mod. La prueba verifica el comportamiento en el servidor real; la aceptación visual de una partida sigue siendo una comprobación distinta.

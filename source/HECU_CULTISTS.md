# Blood Cultist HECU en Cremation

Se agregan 12 monster_human_grunt a Rapidcore, conservando los 40 zombis.
El ZIP aportado se conserva en assets-source/bloodcultisthecu.zip.
install_hecu_assets.py instala los cinco MDL y ocho sonidos exclusivamente en Cremation.
No se modifican Valve, Ravenholm ni otros mods.

Cuatro cabezas suministradas por el modelo y seis equipamientos, con dos soldados
por equipamiento: subfusil; escopeta; subfusil + granadas; escopeta + granadas;
subfusil + lanzagranadas; escopeta + lanzagranadas. Son 12 soldados, no 12 armas
ni 12 modelos independientes. Armas, disparos, granadas, IA son
los nativos de Ravenholm. El proxy elige la cabeza despues del Spawn y asigna
a los HECU la clasificacion 7 de los zombis, evitando hostilidad mutua.
Ambos siguen siendo enemigos del jugador. Se reaplica al restaurar partidas;
para evitar enemigos ya adquiridos se recomienda reiniciar el mapa.
El grupo visible de arma se conserva para que corresponda al equipamiento.

Las posiciones verifican casco humano, suelo, separacion de 144 unidades de otros
inicios de monstruos y mas de 1050 unidades del jugador, sin visibilidad inicial.

Iniciar Cremation.exe y comenzar una partida nueva para cargar las entidades.
Los guardados antiguos conservan el mapa anterior.

Validacion: compilacion y pruebas de armas/ABI, verify_mod.py y servidor GoldSrc
real durante 20 segundos con 12 HECU vivos al aparecer, seis equipamientos y
cuatro cabezas. Pendiente comprobacion visual de animaciones y combate.

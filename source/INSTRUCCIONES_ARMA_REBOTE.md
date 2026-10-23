# Instrucciones: arma independiente de escopeta con rebotes y granada de daño angular

Estado: especificación de implementación. Este documento no cambia el juego.

Actualización posterior del usuario, ya incorporada a la implementación: el arma
modificada debe convivir con Ricochet, sus perdigones hacen un 50% más de daño que
la referencia y la dispersión aumenta ligeramente (valor adoptado: +15% por eje).
Su secundario no debe activar el zoom del portador de ballesta. Estos ajustes
prevalecen sobre el balance de referencia descrito en la especificación inicial.

## 1. Objetivo y decisiones confirmadas

Crear un arma independiente que reutilice el modelo de la escopeta actual de
Cremation. Su disparo primario convierte la descarga secundaria de la escopeta
original en proyectiles físicos con rebotes. Su secundario lanza una granada de
contacto, parecida a la del subfusil, cuyo daño aumenta al apuntar más hacia arriba.

Aclaración expresa del usuario sobre los proyectiles primarios:

- Rebotan dos veces contra superficies.
- Desaparecen en el tercer contacto con una superficie.
- Si alcanzan a un enemigo, hacen el daño normal de **un perdigón** de escopeta y
  desaparecen inmediatamente, sin rebotar en él ni atravesarlo.

La altura relevante del secundario es **la elevación del apuntado al disparar**.
No usar la altura de la plataforma, la altura máxima de vuelo ni el tiempo que
permanece la granada en el aire para aumentar el daño.

Nombre provisional de desarrollo: `weapon_bounce_shotgun` / «Escopeta de rebote».
El nombre comercial definitivo no ha sido indicado por el usuario.

## 2. Alcance e independencia

1. Leer primero `.AGENTS` en la raíz de Half-Life y
   `Cremation/source/RICOCHET_ANALYSIS.md`.
2. Trabajar exclusivamente en Cremation, su código y su lanzador. Conservar los
   directorios `valve`, `ricochet` y `Half-Life2Classic_Ravenholm` como fuentes.
3. Crear una entidad, identidad de inventario y entrada HUD diferentes de la
   escopeta normal y de Ricochet. Reutilizar un modelo no convierte ambas armas
   en el mismo objeto de inventario.
4. El jugador debe poder poseer y seleccionar por separado la escopeta normal,
   Ricochet y el arma nueva. No sustituir silenciosamente una de las dos actuales.
   Añadir la nueva al equipo de prueba inicial sin alterar el HEV, el inicio ni
   las posiciones de los 20 zombis.
5. Mantener Ricochet en su grupo 2. Propuesta para la nueva arma: grupo 3 de
   escopetas, en una posición distinta de la escopeta normal.
6. Usar copias o alias propios de los modelos de escopeta para vista, tercera
   persona y mundo. No sobrescribir `v_shotgun.mdl` o los modelos de Ricochet para
   cambiar el comportamiento de otro objeto.
7. No añadir Freeze Shot, seguimiento automático de blancos, ráfaga Triple Shot
   ni explosiones al primario de esta arma. Sólo se reutiliza la mecánica de rebote.

### Ruta viable con el runtime actual

La DLL de Ravenholm es binaria y la implementación actual usa una extensión Win32.
Antes de programar, auditar los identificadores y portadores de inventario que
realmente acepta el cliente heredado.

- No reutilizar el ID de la escopeta normal ni el portador de pistola de Ricochet.
- Puede reservarse un portador nativo actualmente sin uso en Cremation, con HUD,
  selección, modelos y ataques propios. En el SDK de referencia, la ballesta tiene
  ID 6 y la escopeta ID 7; el 6 es **un candidato**, no una compatibilidad ya probada.
- Si se adopta ese camino, documentar el portador interno, comprobar que no colisiona
  con otra arma y filtrar completamente sus ataques, munición visible y animaciones
  heredadas. La identidad visible y la entidad del mapa deben ser las del arma nueva.
- No limitarse a cambiar el modelo o a reutilizar el ID 7: eso no cumple independencia.
- Si el cliente requiere cambios para aceptar la identidad elegida, implementarlos
  y verificarlos. No declarar terminada una integración que sólo crea una entidad
  de mundo pero no permite llevarla y seleccionarla.

## 3. Referencia exacta de la descarga primaria

Consultar el SDK local, revisión `b1b5cf5892918535619b2937bb927e46cb097ba1`:

- `Cremation/source/hlsdk/dlls/shotgun.cpp`, `CShotgun::SecondaryAttack`.
- `Cremation/source/hlsdk/dlls/combat.cpp`, `CBaseEntity::FireBulletsPlayer`.
- `Cremation/source/hlsdk/dlls/weapons.h`, constantes de dispersión e inventario.

Referencia confirmada del Half-Life original **para un jugador**:

- 12 perdigones por descarga secundaria.
- `VECTOR_CONE_10DEGREES = (0.08716, 0.08716, 0.08716)`.
- Coste de referencia: dos cartuchos y cadencia de 1.5 segundos.
- La rama multijugador es distinta: ocho perdigones y un patrón más ancho y plano.
  No elegir esa rama porque Rapidcore proceda de deathmatch: Cremation es de un jugador.

El código del SDK confirma esos valores del original, pero no demuestra por sí
solo que la DLL modificada de Ravenholm conserve exactamente todos ellos. Comparar
con la escopeta efectiva de Cremation antes de fijar la referencia definitiva.
Si hay una diferencia confirmada, documentarla y conservar la descarga secundaria
de la escopeta que el usuario tiene en este juego como criterio visual final.

### Generación de direcciones

Para cada perdigón, calcular los mismos desplazamientos aleatorios y la misma
base de apuntado que la descarga de referencia. Conservar la distribución de
`FireBulletsPlayer`, no repartir doce discos uniformemente en un círculo ni en
una línea horizontal.

El SDK suma dos muestras en [-0.5, 0.5] para X y otras dos para Y, utilizando la
semilla compartida del disparo y el índice del perdigón. Reproducir también el
orden de semillas. La dirección resultante es:

```text
dir = normalize(forward + right * (x * spread.x) + up * (y * spread.y))
```

Normalizar conserva la dirección geométrica de la traza original y permite que
todos los proyectiles tengan la misma velocidad física. Capturar las direcciones
al disparar: el movimiento posterior de la cámara no debe modificar la descarga.

## 4. Contrato de proyectil primario

Cada perdigón pasa a ser una entidad de proyectil visible y autoritativa del servidor.
No debe seguir existiendo una traza hitscan que haga daño instantáneo por debajo.

Estado mínimo independiente por proyectil:

```text
owner + serial del propietario
arma de origen
posición y velocidad
surface_contacts = 0
damage_snapshot
born_time / expires_at
consumed = false
```

### Movimiento y contactos

1. Vuelo recto sin gravedad. Velocidad inicial propuesta: 1000 unidades/s, como
   Ricochet; es un valor de balance inicial, no una nueva exigencia del usuario.
2. Hacer trazas continuas del segmento completo recorrido en cada paso y resolver
   la primera colisión. No mover primero la entidad y comprobar sólo el punto final.
3. En el primer contacto con pared, suelo o techo: incrementar el contador a 1,
   reflejar la velocidad y continuar.
4. En el segundo contacto con superficie: incrementar a 2, volver a reflejar y
   continuar el vuelo. **Debe verse el recorrido posterior al segundo rebote.**
5. En el tercer contacto con superficie: retirar el proyectil sin generar un tercer
   rebote, explosión ni daño radial.
6. Al contactar con un enemigo vivo en cualquier tramo, antes o después de un
   rebote: consumir el proyectil, aplicar una sola vez el daño de un perdigón y retirarlo.
   Ese contacto no debe procesarse después como choque con pared.
7. No permitir daño repetido a varios enemigos, penetración ni rebote en un enemigo.

Reflexión contra una normal unitaria `n`:

```text
v_after = v_before - 2 * dot(v_before, n) * n
```

Conservar el módulo de la velocidad para esta primera versión. Desplazar el punto
de salida una pequeña distancia hacia fuera de la superficie y consumir el tiempo
restante del paso. Un solo choque no debe contarse varias veces por solapamiento.
Poner un límite de iteraciones por paso para esquinas; al agotarlo, retirar el
proyectil de forma segura, sin atravesar geometría.

Distinguir sólidos reales de agua, triggers, decoraciones no sólidas y otros
proyectiles. Estos últimos no deben consumir rebotes. Para objetos rompibles o
cadáveres, fijar una regla explícita antes de implementar; propuesta conservadora:
si reciben daño, aplicar el impacto una vez y consumir el perdigón, sin penetración.

### Daño de perdigón

- Leer el daño de buckshot efectivo para la dificultad activa, no el daño total
  de una descarga ni el de una granada. El `skill.cfg` actual declara 5 de base
  por perdigón en las tres dificultades.
- Reproducir el tipo de daño de bala y los modificadores nativos de hitgroup y del
  enemigo. Comparar con un perdigón original en condiciones controladas.
- El helper de daño actual de Ricochet usa `DMG_BLAST`; no reutilizarlo literalmente
  para perdigones. Añadir una ruta con tipo de daño explícito sin cambiar la
  semántica de los discos explosivos de Ricochet.
- No restar salud directamente ni asumir que 5 de salud final equivale siempre
  al daño nativo: pueden intervenir resistencia, hitgroup u otras reglas del donante.
- Resolver el daño antes de liberar la entidad si se usa como inflictor, y marcar
  el impacto como consumido para impedir una segunda aplicación.

### Representación y límites

Usar proyectiles pequeños y legibles, con estela corta. Puede reutilizarse el disco
de Ricochet reducido de escala; no es necesario que cada perdigón tenga el tamaño
del disco del arma Ricochet. Evitar el azul de congelación para no comunicar un
efecto que esta arma no tiene.

Definir una vida máxima finita y un presupuesto de entidades. Propuesta inicial:
cuatro segundos y 96 perdigones activos para esta arma. No modificar sin revisión
el límite de 24 discos de Ricochet. Si no cabe una descarga completa, no consumir
munición ni producir una descarga parcial silenciosa.

## 5. Contrato del secundario: granada de daño por elevación

Referencia local:

- `Cremation/source/hlsdk/dlls/mp5.cpp`, `CMP5::SecondaryAttack`.
- `Cremation/source/hlsdk/dlls/ggrenade.cpp`, `CGrenade::ShootContact` y explosión.

El original dispara una granada por contacto a 800 unidades/s con gravedad 0.5.
No se añade la velocidad del jugador. Usar ese arco como punto de partida para
que se comporte como el lanzagranadas del subfusil, no como un disco sin gravedad.

1. Crear una granada por activación del secundario.
2. Lanzarla desde la boca del arma, comprobando que no aparece dentro de una pared.
3. Capturar el ángulo y el daño al disparar, antes del retroceso de ese disparo.
4. Detonar al primer contacto sólido, también si es un enemigo. Este modo no usa
   los dos rebotes del primario.
5. Aplicar una única explosión con sonido, efecto visible, daño radial y empuje pequeño.
6. No sumar un daño directo adicional al enemigo alcanzado que eleve el total por
   encima del comportamiento máximo del lanzagranadas original.
7. Añadir vida máxima de seguridad para granadas que no encuentren un sólido.

### Función de daño propuesta, completa y comprobable

Sea `Dmax` el daño efectivo de la granada M203 del subfusil en la dificultad activa.
El `skill.cfg` actual declara 100 en las tres dificultades. Leer el valor del
runtime/configuración, no fijar 100 permanentemente en el código.

En GoldSrc el pitch negativo apunta hacia arriba:

```text
elevation_deg = clamp(-aim_pitch_at_fire, 0, 90)
t = elevation_deg / 90
Dmin = 0.25 * Dmax
Dshot = Dmin + (Dmax - Dmin) * t
Dshot = clamp(Dshot, Dmin, Dmax)
```

El mínimo del 25% y la curva lineal son **propuestas de balance** para eliminar
ambigüedad en la primera implementación; el requisito confirmado es que el daño
aumente al apuntar hacia arriba y nunca supere el del lanzagranadas.

Ejemplo con `Dmax = 100`, antes de la caída radial y resistencias:

| Elevación al lanzar | Daño base de explosión |
|---|---:|
| Horizontal o hacia abajo | 25 |
| 30° arriba | 50 |
| 60° arriba | 75 |
| 90° arriba | 100 |

El daño se fija en el instante del disparo. Mirar hacia arriba después de lanzar,
saltar, caer desde una plataforma o alargar el vuelo no debe aumentarlo.
El autoaim tampoco debe incrementar por su cuenta el ángulo que determina el daño.

Usar la misma semántica nativa de daño radial del M203, incluidas oclusión y reglas
de los enemigos. Para aislar la variación solicitada al daño, propuesta inicial:
radio fijo igual al radio del M203 a `Dmax`; en el SDK de referencia es `2.5 * Dmax`.
Verificar ese comportamiento en Ravenholm antes de fijarlo como hecho del donante.
El daño recibido cae con la distancia al centro y nunca excede el máximo del M203
en una comparación equivalente. No hacer daño a través de una pared sólida.

### Empuje pequeño, independiente del daño angular

La granada debe producir un empujón perceptible y breve, claramente menor que el
de los discos explosivos de Ricochet. No multiplicarlo por el ángulo ni por `Dshot`.

Propuesta inicial de componente radial:

```text
falloff = clamp(1 - distance_to_explosion / radius, 0, 1)
horizontal_impulse = 140 * falloff
vertical_impulse = 35 * falloff
```

Estos valores son iniciales de balance, no magnitudes exigidas por el usuario.
Mantener iguales el criterio de oclusión y el de objetivo para daño y empuje.

Si se usa el daño nativo, medir su impulso implícito y evitar sumarlo dos veces
al explícito. El resultado total debe seguir siendo pequeño incluso con `Dmax`.
No alterar globalmente la gravedad o el movimiento de los zombis. Si se necesita
un estado temporal para que la IA no anule el impulso, limitarlo a ese impacto
y restaurarlo al aterrizar, morir o expirar.

No se ha definido todavía si una explosión propia puede afectar al jugador.
Mantener como punto de partida la regla actual de Cremation y documentarla; no
añadir salto por explosión ni otro mecanismo como objetivo implícito.

## 6. Cadencia, munición y presentación

- Reutilizar el modelo y las animaciones compatibles de la escopeta, con HUD que
  permita distinguir el arma nueva de la normal.
- Referencias iniciales de cadencia: primario 1.5 segundos, secundario 1 segundo.
  Ambos modos deben respetar el bloqueo cruzado para evitar disparos simultáneos.
- La economía de munición no fue especificada. Propuesta mínima: el primario gasta
  dos cartuchos y el secundario una `ARgrenade`, con existencias iniciales suficientes
  para probar ambos. No crear un nuevo sistema de munición como parte de este objetivo.
- Compartir la reserva de cartuchos es compatible con dos armas independientes,
  pero su cargador, estado de recarga, selección y temporizadores deben ser propios.
- Confirmar capacidad de cargador y animaciones en la escopeta donante; no asumir
  que el modelo de Ravenholm tiene los mismos índices y capacidad que el SDK base.
- Evitar doble disparo por predicción: mantener la estrategia servidor/cliente de
  Cremation y revisar la compatibilidad con `cl_lw 0`.

## 7. Orden de implementación y archivos

1. **Congelar la referencia.** Registrar hashes del mapa, DLL y recursos actuales.
   Respaldar los archivos que se van a sustituir, sin copiar ni modificar los donantes.
2. **Auditar inventario y referencia.** Confirmar ID libre, ruta de selección/HUD,
   descarga secundaria de escopeta y daño/radio del M203 efectivos.
3. **Separar parámetros.** Crear `Cremation/source/bounce_shotgun_balance.h` para
   dispersión, proyectiles, cadencias, contactos, curva de daño y empuje. No meter
   estos valores en el balance de Ricochet.
4. **Aislar la lógica nueva.** Crear un módulo propio, por ejemplo
   `Cremation/source/bounce_shotgun.cpp`, y una interfaz pequeña para integrarlo con
   `ricochet_proxy.cpp`. Si se extrae código compartido de trazas/daño/efectos,
   ejecutar las regresiones de Ricochet antes y después de esa extracción.
5. **Implementar el primario.** Patrón determinista, doce entidades o cantidad
   confirmada del donante, movimiento continuo, dos rebotes, tercer choque y daño
   unitario al enemigo. Verificarlo antes de incorporar la granada.
6. **Implementar la granada.** Arco, captura del ángulo, explosión única, daño con
   techo M203, oclusión y empuje pequeño sin doble acumulación.
7. **Integrar recursos.** Extender `install_ricochet_assets.py` o crear un instalador
   específico con procedencia y hashes. Mantener los recursos de la nueva arma
   separados de los de las dos existentes.
8. **Integrar mapa/equipo.** Añadir la entidad nueva al inicio en `build_mod.py`;
   preservar el resto del contrato de Rapidcore y sus 20 zombis.
9. **Actualizar verificaciones.** `verify_mod.py` debe aceptar y exigir las tres
   armas independientes, sin permitir que desaparezca la escopeta normal o Ricochet.
10. **Compilar e instalar.** Usar `build_ricochet.ps1` ampliado o un sucesor que
    conserve sus verificaciones actuales. Mantener Win32/x86 y el tratamiento
    especial de las exportaciones dentro de `.rdata`.
11. **Documentar el resultado real.** Valores definitivos, portador interno si lo
    hay, límites, comandos de reproducción y pruebas pendientes.

No modificar los 14 bloques no relacionados con entidades del BSP para añadir
esta arma. No recompilar la geometría del mapa para resolver un problema de arma.

## 8. Pruebas automáticas necesarias

### Primario

- Misma semilla y mismo apuntado: direcciones iguales a las de la descarga
  secundaria de referencia, con tolerancia numérica explícita.
- Descarga completa con la cantidad correcta de proyectiles; sin daño instantáneo.
- Primer y segundo contactos: reflexión correcta, contador 1/2 y velocidad conservada.
- Tercer contacto: eliminación, sin tercer rebote y sin explosión.
- Enemigo antes de rebotar, tras un rebote y tras dos: un daño de perdigón, una
  única aplicación y desaparición inmediata en los tres casos.
- Comparación de daño de cuerpo/cabeza con el perdigón nativo y la misma dificultad.
- Esquinas, proximidad a la pared, pasos largos y velocidad alta: sin atravesar
  sólidos, recontar el mismo choque ni entrar en bucle.
- Sin seguimiento de blancos, congelación, penetración o daño radial involuntarios.
- Límite de entidades, vida máxima y falta de presupuesto sin descargas parciales.

### Secundario

- Una granada, velocidad inicial y gravedad según la referencia; detona una sola vez.
- Daños para -30°/0°/30°/60°/90° de elevación coherentes con la tabla y la saturación.
- Mantener daño idéntico al mover la cámara después del disparo.
- Mismo ángulo desde alturas distintas: mismo `Dshot`.
- Mismo ángulo y distinta duración de vuelo: mismo `Dshot`.
- Nunca superar `Dmax`; no sumar daño directo y radial accidentalmente.
- Caída radial y bloqueo por paredes correctos.
- Empuje total pequeño y sin crecimiento por ángulo, daño, pasos de simulación
  o repetición del callback de contacto.

### Integración y regresiones

- Las tres armas pueden coexistir y seleccionarse; modelos e iconos correctos.
- Ricochet conserva su grupo 2, Freeze Shot y Triple Shot actuales.
- La escopeta normal conserva sus ataques, recarga y daño originales.
- Inventarios, cargadores y temporizadores no se pisan al alternar armas.
- Fallecimiento, cambio de mapa y carga de guardado no dejan proyectiles huérfanos
  ni modificaciones permanentes de movimiento/aspecto.
- Todas las exportaciones y cadenas contiguas siguen dentro de `.rdata`.
- Pasan la carga nativa y `check_server_startup.py` con el motor real.

## 9. Aceptación manual y entrega

El usuario comprobará manualmente el juego. No controlar la interfaz sin nueva
autorización ni confundir una prueba de servidor con validación gráfica.

Entregar el mod compilado/instalado, el lanzador válido, fuentes, procedencia de
recursos y resultados de pruebas. Indicar expresamente qué comprobaciones faltan.

Criterios jugables de aceptación:

1. El arma se distingue de la escopeta normal aunque comparta su modelo.
2. Su primario conserva visualmente la descarga secundaria de referencia, pero
   los proyectiles viajan y permiten ver dos rebotes completos.
3. El tercer choque los elimina; un enemigo los consume en cualquier tramo y
   recibe el daño de un perdigón, sin ser atravesado.
4. El secundario describe un arco de granada, explota al contacto y da un pequeño
   empujón. Apuntar más alto al lanzar aumenta el daño, hasta el del M203 original.
5. Cambiar la mirada después del disparo o lanzar desde una plataforma elevada
   no altera ese daño ya capturado.
6. No reaparece el cierre de arranque de la DLL ni cambian las otras dos armas.

Este documento fija el objetivo y el plan. No es evidencia de que la nueva arma
ya exista o haya sido probada.

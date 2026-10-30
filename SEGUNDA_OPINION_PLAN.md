# Segunda opinion: analisis de la planificacion del port a Featureful

Fecha: 2026-10-29. Complementa ANALISIS_REWRITE_VS_ORIGINAL.md (seccion 5,
"Camino recomendado"): se auditan secuencia, presupuesto y gates de ese plan.
Rama activa: featureful-merge (juego original restaurado y verificado).

## 1. Auditoria de la planificacion original (que fallo)

6 fallas concretas:

1. El plan ignoro su propio contrato. El mapa de migracion
   (source/FEATUREFUL_MIGRATION_MAP.md, 86 items con prioridad P0/P1/P2)
   se genero ANTES del rewrite y ya definia el P0 real: director de oleadas,
   David como subclase real, politica de combate, HUD tactico, radar, saves.
   El rewrite cumplio una fraccion y declaro el resto "siguiente iteracion",
   sin pedir re-scope ni marcar la brecha como bloqueante.
2. No existio definicion de "terminado". "Compila + smoke PASS + JSONs
   validos" se trato como paridad. Falto una matriz de comportamientos y,
   para cada uno, el test que lo verifica, por sistema.
3. Secuencia por costo de implementacion, no por fidelidad. Se avanzo por lo
   barato (templates JSON, DefaultModel), dejando lo que define al mod
   (director con rutas, variantes con reglas) para despues. El resultado fue
   predecible: jugable vanilla con skins nuevas.
4. Tamano subestimado unas 4 veces. La propuesta decia "trabajo de semanas".
   Numeros reales: unos 745 KB de .inc custom + 90 KB de proxy + 43 KB de
   bounce suman ~880 KB de codigo bespoke a portear (solo mansion_waves.inc
   pesa 106 KB y czero_navigation.h generada 152 KB). Un port con paridad es
   un proyecto de meses y debe planificarse como tal.
5. Falto el gate de paridad por paso. El repositorio ya tenia 36 harnesses
   de verificacion (tools/check_*.py y tools/verify_*.py) que validan el mod
   original en el motor real: exactamente el mecanismo que nunca se adopto
   como condicion para promocionar binarios al juego.
6. Sin criterio de por que portar. El plan no articulo que beneficio
   justificaba pagar el port ni definio un go/no-go.

## 2. Las tres rutas posibles

| Ruta | Costo | Riesgo de paridad | Notas |
|---|---|---|---|
| A. Mantener el original (estado actual) | ~0 | Bajo (verificado por harnesses) | Queda atado al donor ABI cerrado (hl_ravenholm.dll) |
| B. Port completo al fork (plan original) | Meses; ~880 KB | Alto si se deja incompleto | Requiere disciplina fuerte |
| C. Port incremental por peldano | Paso a paso | Bajo | Recomendada si se decide portear |

Nota olvidada en el plan original: la Fase 3 (capa proxy featureful_ai.inc
sobre el donor) ya captura varios beneficios practicos de Featureful (olvido
de enemigo, vagar tras combate, cadaver non-solid, no ensanarse con
moribundos) a cerca del 5 por ciento del costo de un port completo. Buena
parte del valor util de Featureful ya estaba disponible sin portearlo.

## 3. Plan corregido (si se decide portear)

Correcciones sobre la seccion 5 del analisis previo:

1. Gate 0: matriz de paridad. Registrar en un solo documento los 25-30
   comportamientos que definen a Cremation (fuente: source/*.md y los
   36 harnesses) y con que test queda atado cada uno. Sin esa matriz, un
   paso no se considera portado.
2. Secuencia por riesgo creciente, no por tamano:
   a. Alianza y facciones + voces + carbonizados (classify y soundscripts;
      barato, sin tocar el director).
   b. Familia David con modelos + variantes + reglas de velocidad y dano
      (sin anatomia: ya descartada).
   c. Director de oleadas como extraccion literal de mansion_waves.inc sobre
      los waypoints intactos del mapa + HUD minimo de oleada.
   d. Reglas completas de Quake, CZ y Tpose.
   e. Armas bespoke por uso real (ricochet, bounce, link, necromancer,
      deagle, awp, winchester) como clases derivadas de CConfigurableWeapon.
   f. HUD, radar, journal y saves versionados.
3. Regla de promocion: cada peldano se promociona (reemplazar dlls/hl.dll
   del juego principal) SOLO si su harness pasa y existe aceptacion jugada.
   El juego principal SIEMPRE queda en la rama estable jugable. Un sistema
   en progreso a la vez, nunca dos.
4. Presupuesto real: director ~40 por ciento del esfuerzo total, armas con
   fisica ~25, resto ~35. Horizonte: meses, no semanas.

## 4. Recomendacion final

- Ahora: Ruta A. Mantener el juego original restaurado como base jugable y
  de comparacion (ya hecho). Conservar los beneficios ya capturados por la
  capa proxy de la Fase 3.
- Si el motivo es estructural (independizarse del donor ABI cerrado, adoptar
  features nuevas de Featureful, migrar a Xash3D o Linux), entonces Ruta C
  con las correcciones anteriores, empezando por el peldano mas barato
  (alianza y voces) y reevaluando tras el primer peldano. Un peldano se
  define como: codigo en el fork + harness en PASS + prueba jugada + commit.
  Nunca se promociona un peldano sin su gate.

En cualquier ruta: el mapa de migracion, el fork y los harnesses ya existen.
Lo que fallo no fue la planificacion base sino ejecutarla sin contratos
verificables ni gates; lo que falta es corregir el metodo, no rehacer el
plan desde cero.

# Configuración de armas

Edita `ricochet.json` o `flak_cannon.json` con Half-Life cerrado o antes de recargar
el mapa. Guarda el archivo y arranca una partida nueva con Cremation.exe; también
puedes ejecutar `map rapidcore` en la consola (reinicia el mapa y la partida).
**No hace falta compilar.** Los cambios no se aplican en mitad de un mapa.

Los archivos son objetos JSON planos: nombres de claves entre comillas, punto
como separador decimal y sin comentarios ni comas finales. `true` y `false` se
escriben sin comillas. Ejemplo de Ricochet (puedes cambiar solo estas claves):

```json
{
  "max_charges": 3,
  "recharge_seconds": 1.5
}
```

Ejemplo de Flak Cannon:

```json
{
  "primary_ammo_cost": 1,
  "secondary_ammo_cost": 2,
  "spread_multiplier": 4.0,
  "pellets_per_shot": 12,
  "gib_on_lethal_overkill": true
}
```

Un archivo válido aplica todos sus valores juntos; las claves omitidas conservan
su valor previo (o el predeterminado al arrancar). Un archivo inválido se rechaza
entero, conservando la última configuración válida de esa arma. Se rechazan claves
desconocidas, duplicadas, tipos incorrectos, valores fuera de rango y números no
finitos. La otra arma puede cargar independientemente. Los avisos aparecen en la
consola y en `Cremation/startup-diagnostics.txt` como `WeaponConfig`; `WeaponBalance`
resume los valores aplicados. Borrar un archivo no restaura valores: reinicia el
juego para volver a los valores incorporados.

Tiempos en segundos, velocidades/distancias en unidades GoldSrc. `slow_speed_factor`
0.25 significa 25% de velocidad. `blast_damage_per_disc` es por disco (el secundario
lanza tres). `spread_multiplier` multiplica el radio tangencial base 0.08716: 4.0
produce unos 38.4 grados de apertura total; un número mayor abre más el círculo.
`surface_contacts_until_removal`: 3 significa dos rebotes y desaparición al tercer
contacto; el impacto en enemigo siempre consume el perdigón. El gibbing requiere daño letal que supere la vida restante; una descarga completa no fuerza una muerte. Se mantienen las resistencias nativas.

El daño de Flak usa `skill.cfg`: `sk_plr_buckshot1..3` por `pellet_damage_multiplier`,
El máximo del secundario es el daño nominal de una descarga completa por `secondary_damage_multiplier`; el mínimo usa `secondary_min_damage_fraction` (0.5). El daño aumenta linealmente con el ángulo, entre 0 y 90 grados. Un impacto directo recibe ese daño; la explosión tiene caída lineal dentro de `secondary_radius` (48). `secondary_speed` (450), `secondary_gravity` (0.8) y `secondary_upward_boost` (120) controlan el arco.

Modelos, sonidos, identificadores de inventario/HUD, lógica de ataques, muestreo
circular y reparto del mapa permanecen en código/recursos. Estos JSON configuran
el balance de las dos armas existentes; no crean nuevas clases de armas.
El instalador de recursos no sobrescribe estos archivos.

## Ajustes nuevos

- Ricochet disc_speed=750 y guidance_turn_rate=6 rad/s; control por punteria,
  sin seleccion automatica de enemigos ni laser.
- Flak secondary_damage_multiplier=0.75 reduce ambos extremos un25%.
- secondary_shrapnel_enabled activa el anillo diagonal sobre superficies;
  no fragmenta al impactar un enemigo. secondary_shrapnel_damage_multiplier
  multiplica el dano de explosion capturado dividido por pellets_per_shot.
- Escopeta TFC/sentry: constantes propias en source/tfc_gameplay.inc. Los dos
  JSON siguen controlando Ricochet y Flak.

## Rangos admitidos

### ricochet.json

| Clave | Minimo | Maximo |
|---|---:|---:|
| `guidance_turn_rate` | 0.1 | 30 |
| `max_charges` | 1 | 30 |
| `recharge_seconds` | 0.1 | 60 |
| `disc_speed` | 100 | 4000 |
| `slow_seconds` | 0 | 60 |
| `slow_speed_factor` | 0.05 | 1 |
| `primary_cooldown` | 0.05 | 10 |
| `secondary_cooldown` | 0.05 | 10 |
| `secondary_spread_degrees` | 0 | 45 |
| `blast_radius` | 1 | 1000 |
| `blast_damage_per_disc` | 0 | 1000 |
| `blast_push` | 0 | 3000 |
| `blast_lift` | 0 | 1500 |
| `push_seconds` | 0.05 | 10 |
| `max_horizontal_push` | 0 | 4000 |
| `disc_lifetime` | 0.1 | 30 |
| `max_projectiles` | 3 | 128 |

### flak_cannon.json

| Clave | Minimo | Maximo |
|---|---:|---:|
| `secondary_shrapnel_enabled` | 0 | 1 |
| `secondary_shrapnel_damage_multiplier` | 0 | 10 |
| `secondary_damage_multiplier` | 0.1 | 20 |
| `spread_multiplier` | 0.1 | 10 |
| `pellet_damage_multiplier` | 0.1 | 20 |
| `pellets_per_shot` | 1 | 32 |
| `pellet_speed` | 100 | 4000 |
| `surface_contacts_until_removal` | 1 | 10 |
| `pellet_lifetime` | 0.1 | 15 |
| `primary_cooldown` | 0.05 | 10 |
| `primary_ammo_cost` | 1 | 125 |
| `secondary_ammo_cost` | 1 | 125 |
| `deploy_seconds` | 0 | 5 |
| `secondary_speed` | 100 | 3000 |
| `secondary_gravity` | 0.05 | 3 |
| `secondary_lifetime` | 0.1 | 30 |
| `secondary_cooldown` | 0.05 | 10 |
| `secondary_min_damage_fraction` | 0 | 1 |
| `secondary_push` | 0 | 1500 |
| `secondary_lift` | 0 | 1000 |
| `secondary_push_seconds` | 0.05 | 10 |
| `gib_on_lethal_overkill` | 0 | 1 |
| `secondary_radius` | 8 | 256 |
| `secondary_upward_boost` | 0 | 800 |


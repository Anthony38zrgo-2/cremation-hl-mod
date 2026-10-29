# Cremation / GoldSrc — parche experimental de continuidad de animaciones (DLL x86)

**Estado:** parche binario aplicado y verificado estáticamente; **no probado dentro de Half-Life**. Su uso en el juego es una prueba de integración, no una garantía de que todos los tirones hayan desaparecido.

**Fecha:** 2026-10-28. **Origen:** DLL suministradas por el usuario en `cremation_dlls.zip`.

## 1. Archivos modificados

| DLL | Estado | Observación |
|---|---|---|
| `Mod_Cremation/dlls/cremation.dll` | **Modificada** | DLL activa de lógica del servidor. |
| `Mod_Cremation/build_variants/cremation_validation.dll` | **Modificada** | Compilación de validación; no se carga automáticamente como servidor activo. |
| `Mod_Cremation/dlls/hl.dll` | Sin cambios | DLL donante; idéntica a `hl_ravenholm.dll` en el original. |
| `Mod_Cremation/dlls/hl_ravenholm.dll` | Sin cambios | DLL donante nativa. |
| `Mod_Cremation/cl_dlls/client.dll` | Sin cambios | Proxy de cliente. |
| `Mod_Cremation/cl_dlls/client_ravenholm.dll` | Sin cambios | DLL donante de cliente. |

Se conservan las DLL modificadas **con el mismo tamaño de archivo** que sus originales. El motor GoldSrc y las DLL de Valve **no se modifican** ni se incluyen en este paquete.

## 2. Parches efectuados

### P0 — Quake Zombie, transición redundante de estados no ofensivos

- Producción, `cremation.dll`: hook RVA `0x3D5AE`, trampolín RVA `0x7C780`.
- Validación, `cremation_validation.dll`: hook RVA `0x4E7AE`, trampolín RVA `0x91A70`.

El gancho intercepta una asignación de secuencia. **Solo** suprime la operación si concurren las tres condiciones:

1. El estado solicitado es **0 o 1**, asociados al tramo no ofensivo de este selector.
2. La secuencia solicitada ya coincide con `pev->sequence`.
3. El estado solicitado ya coincide con el estado almacenado por la entidad en `+0x204`.

Si alguna condición falla, ejecuta la escritura original y continúa con todos los reinicios previstos. No se suprimen reinicios de ataques, dolor, muerte ni otros estados. Este es un refuerzo defensivo: **no consta todavía que esa triple condición ocurra frecuentemente durante una partida**.

### P1 — Fast Zombie y Randy, fallback a secuencia 0

- Producción: RVA `0x21837`, trampolín RVA `0x7C7B0`.
- Validación: RVA `0x2C307`, trampolín RVA `0x91AA0`.

Se conserva el comportamiento indispensable de sustituir un índice inválido de secuencia por `sequence = 0` (si el modelo es válido y tiene al menos una secuencia). En lugar de forzar **siempre** `frame=0` y `animtime=ahora` en cada fallback, el parche conserva el fotograma y el tiempo previos **solo si** `frame` es finito y está en el intervalo `[0, 255]` y `animtime != 0`. Para valores fuera de rango, NaN, negativos o sin tiempo inicializado, se ejecuta la restauración original. Esto puede permitir que una animación idle siga avanzando cuando otra parte del código insista en solicitar actividades no válidas.

**Limitación:** no corrige la causa que produce el índice inválido; para arreglarla de raíz son necesarios los modelos `.mdl` y/o el código fuente que resuelve actividades. Tampoco asegura que el contador privado de animación del donante y el proxy estén sincronizados.

### P1 — CZero, eliminación de dos escrituras redundantes

- Producción: RVA `0x11B3E` y `0x13DD8`.
- Validación: RVA `0x12A0E` y `0x14AF8`.

Dentro de dos acciones se escribía `frame=0` y, sin leer `frame` ni llamar a ninguna otra función entre ambas escrituras, se volvía a escribir `frame=0`. Se elimina **únicamente la primera escritura duplicada**, manteniendo la última, por lo que las acciones (incluidos ataques reiterados) **siguen reiniciando correctamente sus animaciones**. Es una limpieza de instrucciones redundantes con el mismo resultado final, no una corrección de la frecuencia con que la IA invoca dichas acciones.

## 3. Qué **no** se ha alterado

- No se eliminan los `frame=0` de otros zombis sin conocer el evento o la transición que los ejecuta.
- No se modifica el avance manual del contador `+0x200` del Quake Zombie: cambiar su relación con `frame`, `animtime` o `StudioFrameAdvance()` sin código fuente o trazas del motor podría provocar desincronización.
- No se modifica `hl_ravenholm.dll` ni el motor gráfico.
- No se modifica la tabla de actividades ni las secuencias dentro de los modelos `.mdl`.
- No se promete una limpieza exhaustiva de memoria, concurrencia, IA ni interoperabilidad del proxy: una DLL compilada no permite demostrar sin más que todos esos patrones sean redundantes o inseguros.

## 4. Instalación

1. **Cierra Half-Life** y haz copia completa de la carpeta `Mod_Cremation` del juego. Comprueba la compatibilidad con los hashes de abajo antes de sobrescribir nada.
2. Copia el contenido de `Mod_Cremation/dlls/` del ZIP a la carpeta `dlls/` del mod, reemplazando únicamente las DLL de ese directorio. Las DLL de cliente incluidas son copias idénticas para facilitar restauración y no requieren reemplazo.
3. La compilación de validación queda en `Mod_Cremation/build_variants/`, **no** renombres esa DLL a `hl.dll` ni la instales como principal sin configurar adecuadamente el proxy.
4. Ejecuta una partida de prueba fuera de tu partida principal. Prueba zombis quietos, desplazándose, atacando varias veces, recibiendo daño y muriendo. Vigila bloqueos, modelos inmóviles, y `idle` que nunca termina.
5. Si algún comportamiento empeora, restaura las DLL activas desde `BACKUP_ORIGINALS/` o desde tu copia de seguridad previa.

### Reversión

- `BACKUP_ORIGINALS/cremation.dll` -> restaura a `<mod>/dlls/cremation.dll`.
- `BACKUP_ORIGINALS/cremation_validation.dll` -> restaura a `<mod>/build_variants/cremation_validation.dll`.

## 5. Comprobaciones realizadas

- Identidad SHA-256 de originales validada antes de parchear (no hay búsqueda heurística arriesgada sobre versiones distintas).
- Firmas de instrucciones originales comprobadas antes de modificar bytes.
- Se modificó exclusivamente `.text` y el tamaño virtual de `.text` (dentro de su propio espacio reservado), además del campo checksum PE.
- Trampolines x86 desensamblados; saltos relativos y retorno al flujo original revisados.
- Tamaño total de cada DLL conservado.
- 548 exportaciones preservadas en cada DLL.
- Tabla de importaciones, tabla de exportaciones y sección `.reloc` conservadas.
- Checksum PE recalculado y contrastado.
- El verificador compara el archivo modificado con una reconstrucción determinista a partir del original.
- **Sin pruebas runtime:** no se puede certificar estabilidad, FPS, animaciones reales ni compatibilidad de todos los modelos desde este entorno.

## 6. Hashes

| Archivo | SHA-256 |
|---|---|
| Original producción | `8335a06a4e3dc91cd5360e5be5fb48678f8cf72f45b81274dcf6b5bd5c9b97f3` |
| Parche producción | `8f29251a4c550fa885530bc26f75936b0b806100119f003c4b96b9413986de77` |
| Original validación | `f4b085ae715b12eda94b5651ad5eed4dadb56d7caa93573c9e7df8cc95f834e6` |
| Parche validación | `950e71f34086f815a8eb8fc7bf4ce4ab8c1ed75aca2cc61478a262c7fc7e9397` |

## 7. Scripts

Para reproducir la modificación sobre los binarios **exactos**, sin dependencias adicionales:

```bash
python tools/apply_anim_patches.py BACKUP_ORIGINALS/cremation.dll Mod_Cremation/dlls/cremation.dll --profile production
python tools/apply_anim_patches.py BACKUP_ORIGINALS/cremation_validation.dll Mod_Cremation/build_variants/cremation_validation.dll --profile validation
```

Verificar las DLL instalables:

```bash
python tools/verify_anim_patches.py BACKUP_ORIGINALS/cremation.dll Mod_Cremation/dlls/cremation.dll --profile production
python tools/verify_anim_patches.py BACKUP_ORIGINALS/cremation_validation.dll Mod_Cremation/build_variants/cremation_validation.dll --profile validation
```

Los scripts detienen la operación si el hash de origen no coincide. **No los uses con nuevas compilaciones** sin analizar las diferencias de código y actualizar conscientemente las direcciones.

# Portar armas de Unreal Tournament a GoldSrc

- Extraer con UModel las mallas de primera persona, tercera persona y suelo, sus texturas y todas las poses. Consultar la clase UnrealScript para identificar materiales y secuencias. Referencia: `tools/convert_ut99_pulsegun.py`.
- UT1 anima por vértices; GoldSrc requiere huesos. Convertir las poses a un rig de **máximo 128 huesos**, conservando UV y animaciones. Ajustar escala y ejes respecto de un viewmodel funcional. En este port, el frente de la vista está hacia **Y negativo**; comprobarlo antes de instalar. Al reflejar un eje, invertir también el orden de los triángulos y sus normales.
- Crear el attachment de boca en la **salida visible del cañón**, unido al hueso que la anima. Comprobar su posición en el MDL compilado; no usar el origen del modelo ni asumir que el vértice más adelantado pertenece a la boca.

## Problema principal: efectos separados del arma

**No corregirlo moviendo repetidamente el arma o usando offsets fijos desde la cámara.** El viewmodel y los efectos del mundo pueden tener proyecciones diferentes; un attachment en coordenadas del mundo no garantiza coincidencia visual.

- Obtener la boca desde la matriz de huesos utilizada al dibujar el viewmodel (`StudioDrawPoints`), sin depender exclusivamente de `cl_entity.attachment`, que se actualiza en pases `STUDIO_EVENTS`.
- Proyectar ese punto con las matrices reales de render del arma y reconstruirlo para la proyección del mundo. Usar ese mismo origen visual para núcleo, filamentos, brillo e inicio del proyectil primario. Implementación actual: `source/client_proxy.cpp`.
- Mantener objetivo, trayectoria física, colisiones y daño en el servidor. El cliente ancla el efecto visual. Revisar la transición entre el plasma inicial del cliente y el proyectil del servidor para evitar saltos o duplicados.
- Preservar el cliente original y sus exports; interceptar también la tabla `F`. Reiniciar referencias de efectos al cambiar de mapa o reinicializar vídeo.
- Compilar e instalar únicamente los modelos y DLL necesarios; el proxy cliente requiere `opengl32.lib` y `cl_dlls/client_ravenholm.dll`. Evitar el pipeline general de assets, que puede sobrescribir modelos personalizados.

## Validación pendiente hasta comprobarla en juego

Compilar no demuestra que el origen esté alineado. Comprobar ambos disparos al mover la cámara, mirar arriba/abajo, animar el arma y disparar cerca de paredes. Reiniciar completamente el juego tras cambiar modelos o DLL. Consultar `link-muzzle-diagnostics.txt` y `source/UT_PULSE_GUN_PORT.md`; la última corrección de proyección aún requiere aceptación visual. No declararla verificada solo por la compilación.

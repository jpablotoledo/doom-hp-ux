# Índice de documentación - cómo creció el proyecto

Todos los documentos de este repo, en el orden en que realmente se
escribieron, leídos como una línea de tiempo. Cada entrada enlaza ambas
versiones de idioma (el proyecto mantiene una copia en español y otra en
inglés de cada doc). Los prefijos numéricos en los archivos dentro de
`docs/` reflejan este mismo orden; `README.md`/`LEAME.md` quedan sin
prefijo en la raíz del repo (GitHub renderiza `README.md` de forma
especial).

| # | Fecha | Español | Inglés | Qué cubre |
|---|---|---|---|---|
| 1 | 2011-06-30 | [`LEAME.md`](../LEAME.md) | [`README.md`](../README.md) | Resumen del proyecto: hardware, pasos de build, estado actual. Se mantiene actualizado - es el único par que sigue manteniéndose activamente en vez de quedar como instantánea histórica. |
| 2 | 2011-06-30 | [`02-configuracion-maquina.md`](02-configuracion-maquina.md) | [`02-machine-setup.md`](02-machine-setup.md) | *Histórico.* Primera auditoría de la máquina y primer build exitoso - Doom corriendo en el B2000 sin ningún audio. |
| 3 | 2011-06-30 | [`03-cambios-codigo.md`](03-cambios-codigo.md) | [`03-source-changes.md`](03-source-changes.md) | *Histórico.* Los cambios de código necesarios para ese primer build: rutas X11, compatibilidad con HP make, ubicación del config file. |
| 4 | 2011-06-30 | [`04-agregar-sonido.md`](04-agregar-sonido.md) | [`04-add-sound.md`](04-add-sound.md) | *Histórico.* Primeros efectos de sonido funcionales (HP Alib / `simpleAudio`), solo SFX, todavía sin música. |
| 5 | 2026-07-10 | [`05-investigacion-musica.md`](05-investigacion-musica.md) | [`05-music-investigation.md`](05-music-investigation.md) | La historia completa de la música: investigación de arquitectura, el sintetizador de proceso externo abandonado, la reescritura en proceso OPL2/GENMIDI que resolvió la traba, y los seis bugs de síntesis encontrados y corregidos en una investigación posterior del "chirrido". El documento más grande y detallado del proyecto. |
| 6 | 2026-07-12 | [`06-investigacion-rendimiento-cpu.md`](06-investigacion-rendimiento-cpu.md) | [`06-cpu-performance-investigation.md`](06-cpu-performance-investigation.md) | Investigación de optimización de CPU/compilador (flags PA-RISC, MAX-2, PBO, `rtprio`) más una sección de resultados sobre lo que realmente se probó, incluyendo una advertencia fuerte sobre `rtprio` colgando la máquina. |
| 7 | 2026-07-14 | [`07-auditoria-commits.md`](07-auditoria-commits.md) | [`07-commit-audit.md`](07-commit-audit.md) | Retrospectiva, no un doc de fase de desarrollo: una auditoría commit por commit de toda la historia de audio/rendimiento, evaluando qué sigue siendo necesario y qué quedó superado o nunca llegó a producción. |

## Leído como historia de crecimiento

1. **2011, un solo día**: el proyecto arranca completamente silencioso
   (`-DDOOM_NO_SFX`), logra efectos de sonido vía la librería de audio
   Alib de HP, y consigue un WAD real más un par de fixes de build
   script/render - docs 1-4 (más el cambio de WAD y `i_video.c` de
   `3191a4e`, absorbido en las actualizaciones continuas del doc 1 en
   vez de tener uno propio).
2. **Un salto de 15 años en el historial de commits** (2011 → 2026) - el
   proyecto queda sin tocar.
3. **2026-07-10**: un primer intento de agregar música falla (proceso
   externo, corte audible), se encuentra la causa raíz (latencia de
   scheduling entre procesos en un solo núcleo), y se reconstruye como
   un sintetizador en proceso que sí funciona - doc 5.
4. **2026-07-12**: con la traba del motor ya resuelta en gran parte por
   flags de compilador, surge un problema distinto y hasta entonces
   enmascarado (un "chirrido" de audio) que se rastrea hasta seis bugs
   de síntesis separados - también absorbido en el doc 5 (sección 10), y
   el doc 6 cubre el lado de CPU de ese mismo trabajo.
5. **2026-07-14**: una pasada retrospectiva audita toda la historia para
   responder "¿todo esto era realmente necesario?" - doc 7.

## Notas sobre los documentos históricos

Los docs 2-4 describen el estado del proyecto en junio de 2011, antes de
que existieran audio, música o cualquiera del trabajo de
compilador/rendimiento. Se mantienen exactamente como se escribieron
originalmente en vez de reescribirse, para conservar fielmente
documentado el estado inicial del proyecto - cada uno lleva una nota al
inicio que apunta a dónde vive realmente la información actual. El doc 1
(`README.md`/`LEAME.md`) es la excepción: se actualiza para mantenerse
al día, ya que es la puerta de entrada del proyecto.

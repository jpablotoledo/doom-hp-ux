# Investigación: implementación de música para HP-UX

Informe de arquitectura sobre el estado actual de la música en el port y lo necesario
para implementarla, tanto a nivel de juego como de sistema operativo (HP-UX / HP
Visualize B2000). Es un documento de análisis, no describe una implementación
realizada.

## 1. Estado actual del código

La música **no está implementada**. Es un problema estructural, no específico de
HP-UX: la capa de bajo nivel nunca se completó en ningún target del `Makefile`,
ni siquiera en Linux.

- **`src/s_sound.c`**: capa de juego completa y correcta. `S_ChangeMusic()`
  localiza el lump `d_<nombre>` vía `W_GetNumForName`, lo cachea con
  `W_CacheLumpNum(..., PU_MUSIC)` y llama a `I_RegisterSong()` / `I_PlaySong()`.
  `S_Start()` calcula la música por nivel/episodio en cada cambio de mapa. Todo
  esto se ejecuta siempre, sin banderas de compilación.
- **`src/i_sound.c`, sección "MUSIC API"** (línea ~1372): todas las funciones
  (`I_InitMusic`, `I_PlaySong`, `I_RegisterSong`, `I_PauseSong`, `I_StopSong`,
  etc.) tienen su cuerpo real envuelto en `#ifdef MUSSERV`. **`MUSSERV` nunca se
  define** en `src/Makefile` (ni en el target HP-UX ni en ningún otro). Fuera de
  ese `#ifdef` las funciones son no-ops o simulan estado (`I_PlaySong` solo hace
  `musicdies = gametic + TICRATE*30`; `I_UnRegisterSong` está completamente
  vacía).
- El diseño `MUSSERV` original apuntaba a un **proceso externo** lanzado con
  `popen("algún-binario", "w")` y un protocolo de un carácter (`R` play/resume,
  `P` pause, `S` stop, `Q` quit, `N%d` + datos MUS para registrar canción, `V%i R`
  volumen). Ese binario externo nunca existió en el repo ni se documentó cuál era.
- A diferencia de SFX, **no existe ningún bloque `#elif defined(__hpux)` para
  música** en `i_sound.c`.

### Assets disponibles en el WAD

Verificado directamente contra `shareware/doom1.wad`: contiene **13 lumps `D_*`**
en formato MUS (E1M1–E1M9, INTER, INTRO, VICTOR, INTROA) más el lump
**`GENMIDI`** (11.908 bytes, banco de instrumentos FM estándar de DMX). Todos los
datos necesarios ya están en el WAD que se distribuye con el proyecto — no hace
falta descargar ni convertir nada externamente. El formato MUS es propietario de
id Software/DMX (similar a MIDI pero comprimido); el parser de cabecera MUS
(`struct musheader_s`) existe en el código pero está muerto dentro de `#ifdef
MUSSERV`.

## 2. Verificación en la máquina real (HP Visualize B2000, vía telnet)

- **Alib/Aserver es un canal PCM puro**: se revisaron `/opt/audio/include/Alib.h`
  y `Audio.h` en la propia máquina — cero símbolos relacionados con MIDI.
  `/opt/audio/bin/` solo contiene `Aserver`, `asecure`, `attributes`, `convert`,
  `send_sound`. No hay sintetizador MIDI ni externo instalado (`swlist`, `find`
  sobre el filesystem: negativos). **Esto descarta cualquier opción de pasarela
  MIDI** — la única vía viable es generar PCM en software y reproducirlo por el
  mismo canal que ya usan los SFX (`openAStream`/`write(audio_fd, ...)`).
- **Recursos disponibles**: `/` con ~103 MB libres, `/opt` ~3,2 GB libres, `/tmp`
  ~424 MB libres; CPU ociosa (load average 0,02) fuera de partida. Margen
  suficiente tanto para PCM pre-renderizado (pocos MB por pista) como para un
  proceso adicional de síntesis en tiempo real.
- El sistema corre **HP-UX B.11.00, PA-RISC 2.0 (9000/785, big-endian)**,
  compilador **HP cc A.11.01.00 en modo ANSI `-Aa`** (sin extensiones GNU, sin
  C99).

## 3. Hallazgo clave: ya hubo un primer intento completo, abandonado

Existe una rama `adding-music` (commit `974a77b "music with noise"`) que
implementa **una arquitectura de síntesis OPL2 en tiempo real de punta a punta**:

- `src/musserver_hpux.c` (1093 líneas): proceso separado con un emulador OPL2
  propio (9 canales FM, envolvente ADSR, 4 formas de onda, modulación de
  feedback), que parsea el lump `GENMIDI` real y datos MUS reales.
- `src/i_sound.c` modificado: usa `fork()` + dos pipes (en vez de `popen()`) para
  que el proceso de música escriba PCM a un pipe que Doom lee con un **ring
  buffer** y mezcla en tiempo real con el buffer de SFX antes de escribir al
  mismo `audio_fd` de Aserver — resolviendo el problema de "un stream único vs.
  varios streams simultáneos" a favor de un único stream mezclado.
- Ya resolvieron un problema real de portabilidad al compilador HP:
  `__attribute__((packed))` (rechazado por `cc -Aa`) fue reemplazado por lectura
  byte a byte de la cabecera MUS.
- El binario `musserver-hpux` sigue instalado en `/opt/doom-hpux/` en la máquina
  real, y el log de una sesión reciente (`doom.log`) muestra el mecanismo
  funcionando (pipes, throttling, mezcla, con estadísticas coherentes:
  `musmix: calls=700 throttled=278 read_ok=422 ... hp_queued=1217`) — pero el
  propio mensaje del commit indica que el resultado audible era **ruido**, no
  música limpia. Hay 16 archivos WAV de depuración en `test_audio/` (nombres como
  `..._ENDIANNESS_FIXED`, `..._fixed_retrigger`, `..._crossfade`,
  `poly_1chan`→`poly_9chan_all`) que documentan una sesión larga de debugging de
  polifonía, retrigger de notas y endianness que no llegó a cerrarse antes de
  reiniciar en la rama actual (`adding-music-try2`), la cual no contiene nada de
  ese código.

## 4. Análisis de validación: ¿el sintetizador ya funciona?

La música de Doom no es MIDI genérico: es **MUS**, un formato propio de id
Software/DMX muy parecido a MIDI pero comprimido. El lump `GENMIDI` tampoco es
un banco de sonidos MIDI — son literalmente **bytes de registros del chip
OPL2** (el sintetizador FM de las tarjetas AdLib/SoundBlaster de los 90). Por
eso "construir un reproductor MIDI" para validar antes de tocar el juego, en la
práctica, significa construir un emulador de OPL2 que consuma esos registros.

Ese reproductor de validación **ya se construyó dos veces** dentro del commit
`974a77b`, y los WAV resultantes quedaron en el historial de git
(`test_audio/`). Se extrajeron y se les corrió un análisis espectral (planitud
espectral: 0 = señal tonal/música, 1 = ruido blanco) para determinar
objetivamente, sin necesidad de escuchar, en qué etapa se rompe la señal:

| Archivo | Etapa | Planitud espectral | RMS | Clipping | Lectura |
|---|---|---|---|---|---|
| `1_piano_bass_notes.wav` | Emulador OPL2 casero, notas simples aisladas | 0.016 (muy tonal) | 11771 | 0% | Limpio |
| `9_d_intro_all_fixes.wav` | Emulador OPL2 casero, canción real completa | 0.846 (casi ruido puro) | 16748 | 0.08% | Ruido |
| `10_..._NUKED_real_emulator.wav` | Cambio a Nuked-OPL2 (emulador de ciclo exacto, verificado contra el chip YM3812 real) | 0.795 (ruido) | 18954 | 0.16% | Ruido (aún con bug) |
| `11_..._ENDIANNESS_FIXED.wav` | Nuked-OPL2 + fix de endianness, aislado (sin pasar por Doom) | 0.050 (tonal) | 3397 | 0% | Limpio |
| `14_..._after_ringbuffer_fix.wav` | Captura real desde el pipeline completo (Doom→pipe→ring buffer→mezcla→Aserver) | 0.172 (parcialmente tonal, dominado por 0-120 Hz) | 4550 | 0% | Degradado, no ruido puro pero sospechoso |

**Lectura:**

1. El emulador OPL2 casero (`src/opl2test_hpux.c`) reproducía notas aisladas
   simples correctamente, pero fallaba con canciones reales polifónicas
   (confirma el propio comentario en el código: *"dense broadband noise"*).
2. Al reemplazarlo por **Nuked-OPL2-Lite** (`src/opl2.c`/`opl2.h`, emulador de
   ciclo exacto verificado contra hardware real, licencia LGPL 2.1) alimentado
   directamente con los bytes de registro de `GENMIDI` — arquitectónicamente
   correcto, porque GENMIDI *son* registros OPL2 — y corregir un bug de
   endianness, el resultado **aislado es limpio y tonal** (archivo 11). El
   motor de síntesis está, en la práctica, resuelto.
3. La captura real desde el pipeline completo (archivo 14: proceso hijo → pipe
   → ring buffer → mezcla con SFX → `write()` a Aserver) muestra en cambio una
   señal degradada, dominada por frecuencias muy bajas (0-120 Hz) — un bug
   **distinto**, en la etapa de integración (lectura del pipe, formato/orden de
   muestras en el ring buffer, o la fórmula de mezcla "virtual-analog" en
   `I_SubmitSound()`), no en la síntesis en sí.

**Conclusión:** no hace falta construir un reproductor de prueba desde cero —
ya existe uno y ya demostró que la síntesis funciona (archivo 11). El problema
real y acotado está en el último tramo del pipeline (pipe → ring buffer →
mezcla → Aserver), la parte más nueva y frágil de `i_sound.c`. El paso de
validación más barato que sigue es aislar esa etapa (capturar lo que sale del
pipe *antes* de la mezcla) en vez de reconstruir el sintetizador.

## 5. Opciones consideradas

| Opción | Descripción | Riesgo | Trabajo reutilizable |
|---|---|---|---|
| **A. Retomar `adding-music`** | Depurar la etapa de integración (pipe/ring buffer/mezcla/Aserver) partiendo de que la síntesis OPL2 (Nuked-OPL2 + GENMIDI) ya está validada de forma aislada | Bajo-medio — el problema ya está acotado a una etapa concreta | ~2500 líneas + horas de debugging ya documentadas en `test_audio/`, incluida la síntesis funcional |
| **B. Pre-render offline a PCM** | Convertir las 13 pistas MUS a PCM/WAV fuera de HP-UX (reutilizando el propio Nuked-OPL2, que ya probó ser correcto) y reproducirlas por streaming simple, igual que los SFX | Bajo — sin síntesis en tiempo real, sin riesgo de CPU ni de bugs de emulación | El emulador Nuked-OPL2 ya validado (archivo 11) se reutilizaría igual, solo cambia cómo se reproduce el resultado |
| **C. Pasarela a sintetizador MIDI externo** | Descartada — no existe backend MIDI en la máquina (verificado) | — | — |

## 6. Sesión de depuración en vivo (2026-07-10): dos bugs reales encontrados y corregidos

Se decidió validar la síntesis directamente en el hardware real antes de tocar
el juego: se trajeron `src/opl2.c`, `opl2.h`, `opl2_stdint_hpux.h` y
`opl2test_nuked.c` desde el commit `974a77b` a la rama actual
(`adding-music-try2`), y se estableció un ciclo de trabajo repetible contra el
B2000 por telnet/FTP (sin acceso SSH/SCP disponible):

1. Editar el `.c` localmente.
2. Subir por FTP a `/tmp/opltest/` en el B2000.
3. Compilar y ejecutar con `cc -Ae +O2 ...` lanzado vía `at -f script now`
   (evita que `ccom` muera por `SIGHUP` al cerrar la sesión telnet — el mismo
   problema P3 ya documentado en `docs/machine-setup.md`).
4. Descargar el `.wav` resultante por FTP a `test_audio/loop/` en este repo.
5. Escuchar y/o analizar espectralmente, iterar.

Con este ciclo, escuchando directamente los WAV (no solo el análisis
espectral automático, que no detecta desplazamientos de tono), se encontraron
dos bugs reales que el intento anterior (`adding-music`) no había resuelto:

### Bug 1: tempo MUS incorrecto (70 Hz en vez de 140 Hz)

Al renderizar `D_E1M1` ("At Doom's Gate") aislado, el resultado no sonaba a
ruido pero tampoco se parecía a la música real: sonaba muy lento y grave,
como un vinilo reproducido a menos RPM. `opl2test_nuked.c` usaba
`MUS_TEMPO_HZ 70`, pero el formato MUS de Doom define sus eventos a una tasa
fija de **140 Hz** (no 70). Cada evento de música quedaba sonando el doble de
tiempo del debido. Corregido cambiando la constante a 140.

### Bug 2: estructura de `GENMIDI` mal interpretada

Con el tempo corregido, se probó un "barrido de instrumentos" nuevo
(`--instruments` agregado a `opl2test_nuked.c`, toca cada uno de los 175
instrumentos de `GENMIDI` en secuencia sobre una nota fija) para aislar el
problema de timbre del problema de canción completa. Resultado: todos los
instrumentos sonaban igual, como "pitidos" simples sin timbre reconocible, y
uno de ellos (instrumento 39, Synth Bass 2) sonaba a ruido/estática.

Se verificó contra el código fuente real de Chocolate Doom
(`src/i_oplmusic.c`, `LoadOperatorData`/`LoadInstrumentTable`) que la
estructura real de cada instrumento en el lump `GENMIDI` es:

```
8 bytes de cabecera "#OPL_II#"
175 × 36 bytes de genmidi_instr_t (128 melódicos + 47 de percusión):
    flags(u16 LE) + fine_tuning(u8) + fixed_note(u8) + 2 voces de 16 bytes
    cada voz = operador modulador(6) + feedback(1) + operador portador(6)
             + no usado(1) + base_note_offset(s16 LE)
    cada operador(6) = tremolo, attack, sustain, waveform, scale, level
175 × 32 bytes de nombres de instrumento (no usados para reproducir)
```

Total: 8 + 175×36 + 175×32 = 11.908 bytes — coincide exactamente con el
tamaño real del lump verificado en el WAD.

El código anterior asumía en cambio que cada instrumento ocupaba 68 bytes
consecutivos (36+32 fusionados) y que cada operador tenía solo 5 campos ya
combinados. Esto "casualmente" leía casi correctamente el instrumento 0 (el
offset inicial coincide), pero se desalineaba cada vez más para instrumentos
posteriores, hasta leer directamente bytes de la sección de nombres (texto
ASCII) como si fueran registros OPL2 — exactamente lo que producía el
"pitido sin timbre" (el operador modulador nunca se cargaba con datos
coherentes) y el "ruido de TV" puntual en instrumentos específicos donde la
lectura desalineada caía sobre valores de feedback/forma de onda extremos.
Además, los campos `scale` y `level` son dos bytes **separados** que hay que
combinar con OR para el registro 0x40 (KSL + nivel de salida), no un solo
byte ya combinado como asumía el código.

Corregido reescribiendo las macros de acceso y `opl_load_instrument()` en
`src/opl2test_nuked.c` con los offsets reales. Resultado: los instrumentos
suenan reconociblemente distintos (piano, órgano, guitarra, bajo) y `D_E1M1`
completo, con ambos fixes, es reconocible como música real de Doom
("suena como Doom" — confirmado escuchando el WAV).

Los WAV de cada iteración de esta sesión quedaron en `test_audio/loop/`
(archivos `01_...` a `06_e1m1_tempo_and_genmidi_fixed.wav`) como referencia.

## 7. Estado de la decisión

**Síntesis aislada: resuelta y validada en el hardware real.** Con los dos
fixes de la sección 6, `opl2test_nuked.c` reproduce música reconocible de
Doom sin pasar por el juego. Sigue pendiente extender la validación a más
instrumentos (percusión, índices 128-174) y más canciones antes de retomar
la integración con Doom (pipe → ring buffer → mezcla → Aserver, la etapa que
en el intento anterior (`adding-music`, archivo `14_..._after_ringbuffer_fix.wav`)
mostraba degradación) — que además debería beneficiarse de estos mismos dos
fixes, no solo del emulador en sí. Cuando se retome la integración, hay que
verificar si esa etapa tiene bugs propios además de los ya heredados y
corregidos aquí.

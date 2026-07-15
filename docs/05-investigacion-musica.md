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
datos necesarios ya están en el WAD que se distribuye con el proyecto - no hace
falta descargar ni convertir nada externamente. El formato MUS es propietario de
id Software/DMX (similar a MIDI pero comprimido); el parser de cabecera MUS
(`struct musheader_s`) existe en el código pero está muerto dentro de `#ifdef
MUSSERV`.

## 2. Verificación en la máquina real (HP Visualize B2000, vía telnet)

- **Alib/Aserver es un canal PCM puro**: se revisaron `/opt/audio/include/Alib.h`
  y `Audio.h` en la propia máquina - cero símbolos relacionados con MIDI.
  `/opt/audio/bin/` solo contiene `Aserver`, `asecure`, `attributes`, `convert`,
  `send_sound`. No hay sintetizador MIDI ni externo instalado (`swlist`, `find`
  sobre el filesystem: negativos). **Esto descarta cualquier opción de pasarela
  MIDI** - la única vía viable es generar PCM en software y reproducirlo por el
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
  mismo `audio_fd` de Aserver - resolviendo el problema de "un stream único vs.
  varios streams simultáneos" a favor de un único stream mezclado.
- Ya resolvieron un problema real de portabilidad al compilador HP:
  `__attribute__((packed))` (rechazado por `cc -Aa`) fue reemplazado por lectura
  byte a byte de la cabecera MUS.
- El binario `musserver-hpux` sigue instalado en `/opt/doom-hpux/` en la máquina
  real, y el log de una sesión reciente (`doom.log`) muestra el mecanismo
  funcionando (pipes, throttling, mezcla, con estadísticas coherentes:
  `musmix: calls=700 throttled=278 read_ok=422 ... hp_queued=1217`) - pero el
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
un banco de sonidos MIDI - son literalmente **bytes de registros del chip
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
   directamente con los bytes de registro de `GENMIDI` - arquitectónicamente
   correcto, porque GENMIDI *son* registros OPL2 - y corregir un bug de
   endianness, el resultado **aislado es limpio y tonal** (archivo 11). El
   motor de síntesis está, en la práctica, resuelto.
3. La captura real desde el pipeline completo (archivo 14: proceso hijo → pipe
   → ring buffer → mezcla con SFX → `write()` a Aserver) muestra en cambio una
   señal degradada, dominada por frecuencias muy bajas (0-120 Hz) - un bug
   **distinto**, en la etapa de integración (lectura del pipe, formato/orden de
   muestras en el ring buffer, o la fórmula de mezcla "virtual-analog" en
   `I_SubmitSound()`), no en la síntesis en sí.

**Conclusión:** no hace falta construir un reproductor de prueba desde cero -
ya existe uno y ya demostró que la síntesis funciona (archivo 11). El problema
real y acotado está en el último tramo del pipeline (pipe → ring buffer →
mezcla → Aserver), la parte más nueva y frágil de `i_sound.c`. El paso de
validación más barato que sigue es aislar esa etapa (capturar lo que sale del
pipe *antes* de la mezcla) en vez de reconstruir el sintetizador.

## 5. Opciones consideradas

| Opción | Descripción | Riesgo | Trabajo reutilizable |
|---|---|---|---|
| **A. Retomar `adding-music`** | Depurar la etapa de integración (pipe/ring buffer/mezcla/Aserver) partiendo de que la síntesis OPL2 (Nuked-OPL2 + GENMIDI) ya está validada de forma aislada | Bajo-medio - el problema ya está acotado a una etapa concreta | ~2500 líneas + horas de debugging ya documentadas en `test_audio/`, incluida la síntesis funcional |
| **B. Pre-render offline a PCM** | Convertir las 13 pistas MUS a PCM/WAV fuera de HP-UX (reutilizando el propio Nuked-OPL2, que ya probó ser correcto) y reproducirlas por streaming simple, igual que los SFX | Bajo - sin síntesis en tiempo real, sin riesgo de CPU ni de bugs de emulación | El emulador Nuked-OPL2 ya validado (archivo 11) se reutilizaría igual, solo cambia cómo se reproduce el resultado |
| **C. Pasarela a sintetizador MIDI externo** | Descartada - no existe backend MIDI en la máquina (verificado) | - | - |

## 6. Sesión de depuración en vivo (2026-07-10): dos bugs reales encontrados y corregidos

Se decidió validar la síntesis directamente en el hardware real antes de tocar
el juego: se trajeron `src/opl2.c`, `opl2.h`, `opl2_stdint_hpux.h` y
`opl2test_nuked.c` desde el commit `974a77b` a la rama actual
(`adding-music-try2`), y se estableció un ciclo de trabajo repetible contra el
B2000 por telnet/FTP (sin acceso SSH/SCP disponible):

1. Editar el `.c` localmente.
2. Subir por FTP a `/tmp/opltest/` en el B2000.
3. Compilar y ejecutar con `cc -Ae +O2 ...` lanzado vía `at -f script now`
   (evita que `ccom` muera por `SIGHUP` al cerrar la sesión telnet - el mismo
   problema P3 ya documentado en `docs/02-machine-setup.md`).
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

Total: 8 + 175×36 + 175×32 = 11.908 bytes - coincide exactamente con el
tamaño real del lump verificado en el WAD.

El código anterior asumía en cambio que cada instrumento ocupaba 68 bytes
consecutivos (36+32 fusionados) y que cada operador tenía solo 5 campos ya
combinados. Esto "casualmente" leía casi correctamente el instrumento 0 (el
offset inicial coincide), pero se desalineaba cada vez más para instrumentos
posteriores, hasta leer directamente bytes de la sección de nombres (texto
ASCII) como si fueran registros OPL2 - exactamente lo que producía el
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
("suena como Doom" - confirmado escuchando el WAV).

Los WAV de cada iteración de esta sesión quedaron en `test_audio/loop/`
(archivos `01_...` a `06_e1m1_tempo_and_genmidi_fixed.wav`) como referencia.

## 7. Extensión de la validación aislada: percusión y más canciones

Con los dos fixes de la sección 6 aplicados, se probó además:

- Barrido de percusión (índices GENMIDI 128-174, los 47 sonidos de batería/
  platillos/etc. mapeados por nota fija en canal MIDI 15): confirmado por
  oído ("suena muy bien").
- `D_E1M8` (la pista más larga/densa del shareware, buena prueba de estrés
  de polifonía): confirmado por oído, tonal y sin clipping.

Con esto, la síntesis aislada quedó completamente validada: tempo, estructura
GENMIDI, instrumentos melódicos, percusión y canciones completas.

## 8. Integración con Doom: de "funciona pero se corta" a resuelto

### 8.1 Primer intento: portar los fixes a `musserver_hpux.c` (arquitectura de proceso separado)

Se trajeron `musserver_hpux.c`, `i_sound.c` y el target de Makefile del commit
`974a77b` (arquitectura original de `adding-music`: Doom hace `fork()` +
pipes hacia un proceso `musserver-hpux` separado que sintetiza y envía PCM
de vuelta). Se le aplicaron los mismos dos fixes de la sección 6 (tempo
140Hz, estructura GENMIDI de 36 bytes) al sintetizador OPL2 casero de
`musserver_hpux.c` (nota: ese sintetizador casero, no Nuked-OPL2 - un
comentario del propio código explica que Nuked-OPL2 dentro del servidor en
vivo llegó a consumir ~84% de CPU, inviable junto al ~85% que ya usa el
render de Doom solo).

Compilado y probado en un directorio de pruebas separado (`/tmp/doombuild-test`
en el B2000, sin tocar `/opt/doom-hpux` de producción) vía el mismo ciclo
telnet/FTP/`at`. Resultado escuchado en vivo en la máquina: **ya es música
real**, pero el juego "tiende a pegarse" y la música se entrecorta.

### 8.2 Diagnóstico de la traba: buffer, no CPU del sintetizador

Perfilado de CPU en vivo (`ps -eo pid,pcpu,comm` muestreado cada segundo)
durante 20s de juego mostró: `musserver-hpux` se estabiliza en **menos del
1% de CPU**; `doom-hpux` sube solo él a ~18-19% acumulado en el mismo lapso
- consistente con que el render del juego ya era pesado antes de agregar
música. El log de `musserver` mostraba una tasa de descarte de frames
creciente y alta (`dropped` subiendo sin techo). Conclusión: Doom no
drenaba el pipe de música con la regularidad necesaria porque su propio
bucle de render, ya sobrecargado, no llamaba a `I_SubmitSound()` (que
drena el pipe) con la frecuencia asumida.

Se agrandó el ring buffer de música (`MUSIC_RING_SIZE` en `i_sound.c`) de
~185ms → ~3s → ~12s. Verificado con capturas reales de lo que se envía a
Aserver (`/tmp/aserver_capture.raw`, volcado por el propio `i_sound.c`
durante los primeros ~15s, convertido a WAV localmente respetando el
big-endian de PA-RISC): los cortes bajaron de 24,2% → 4,4% del audio, pero
agrandar más el buffer dejó de ayudar (techo). Aislando con `-warp 1 1`
(arranca directo en E1M1, sin cambios de canción, que es donde se
concentraba la mayoría de los descartes) el nivel de corte ya rozaba el
**silencio natural de la propia partitura** (7,0% medido en la referencia
limpia validada en la sección 6, contra 6,1-8,8% medido en vivo según la
corrida) - es decir, el pipeline ya estaba prácticamente al límite de lo
que un buffer más grande podía arreglar.

### 8.3 Timer real (SIGALRM/setitimer) para desacoplar el audio del render

Se activó un timer de sistema operativo real, análogo al mecanismo genérico
`SNDINTR` que ya existía en el código para otras plataformas (nunca
habilitado para HP-UX) pero adaptado para no pisar el trabajo de música ya
hecho: `I_HPStartAudioTimer()`/`I_HPStopAudioTimer()` en `i_sound.c`
instalan un `SIGALRM` periódico que llama a `I_UpdateSound()` +
`I_SubmitSound()` directamente, sin depender de que el bucle principal del
juego los invoque a tiempo. `d_main.c` bloquea `SIGALRM` con `sigprocmask`
alrededor de sus propias llamadas síncronas para que nunca se solapen con
el timer. Esto mejoró aún más los números del pipe (`dropped` bajó a 3-4,
prácticamente el mínimo posible) pero el usuario seguía escuchando cortes
en vivo.

### 8.4 Refactor final: sintetizador en proceso, sin fork/pipe (`hp_music.c`)

Ante la duda razonable de "¿esta máquina realmente no puede reproducir
música mientras corre el juego?", se reconsideró la arquitectura de fondo:
`musserver-hpux` corría como **proceso separado** - aunque su cómputo es
barato (<1% CPU), el *costo de comunicación entre procesos* (fork, pipes,
esperar a que el scheduler del SO le dé tiempo a un segundo proceso) en una
máquina de un solo núcleo puede pesar más que la síntesis en sí.

Se creó `src/hp_music.c` (+ `hp_music.h`): el mismo motor de síntesis OPL2 y
parser MUS de `musserver_hpux.c` (con los fixes de tempo/GENMIDI ya
incluidos), pero reestructurado de un modelo "push a un pipe" a un modelo
"generar N muestras bajo demanda" (`HPMusic_Generate(buf, n)`), llamado como
una función común directamente desde `I_SubmitSound()` - sin `fork()`, sin
`pipe()`, sin `popen()`, sin segundo proceso que el sistema operativo tenga
que planificar. `i_sound.c` se reescribió para que `I_InitMusic`/
`I_PlaySong`/`I_RegisterSong`/etc. llamen directamente a `HPMusic_*` en vez
de mandar comandos de texto por un pipe. El target `musserver_hp` y el flag
`-DMUSSERV=...` se eliminaron del `Makefile` (ya no se usan en HP-UX).

Nota de implementación: como `I_SubmitSound()` ahora puede ejecutarse
dentro de un manejador de señal real (`SIGALRM`), el volcado de diagnóstico
a `/tmp/aserver_capture.raw` se reescribió usando `open()`/`write()`/
`close()` en crudo en vez de `fopen()`/`fwrite()` - las funciones de stdio
no son seguras de usar dentro de una señal (`fprintf` ya no aparece en
ningún punto de la ruta caliente de audio, se verificó explícitamente).

Compilado y probado igual que los pasos anteriores: sin errores. La captura
real de Aserver con esta arquitectura dio **6,7% de silencio - prácticamente
igual al 7,0% intrínseco de la partitura**. A nivel de señal, el pipeline de
audio ya no tiene margen de mejora relevante.

### 8.5 Conclusión: la traba es preexistente al trabajo de música

El usuario reportó que, aun con esta arquitectura, seguía escuchando cortes
en vivo. Se hizo la prueba decisiva: correr el mismo nivel **sin música**
(`-nomusic`) y comparar. Resultado: **el juego se traba exactamente igual
sin música**, y la traba persiste incluso reduciendo la resolución en
pantalla. Esto confirma que la traba/corte percibido **no tiene relación
con el trabajo de música de esta sesión** - es una característica de
rendimiento preexistente del motor de Doom en este hardware específico (ya
se sabía por `docs/02-machine-setup.md` que el render solo, sin ningún audio,
corre al ~85% de CPU en esta máquina). Cualquier trabajo futuro sobre esa
traba es un problema de rendimiento general del motor/hardware, separado
del alcance de esta investigación.

## 9. Estado final

**Música funcionando correctamente, arquitectura definitiva: sintetizador
OPL2/GENMIDI en proceso (`hp_music.c`), sin proceso externo, alimentado por
un timer real independiente del render.** Validado por oído en el hardware
real: tempo correcto, instrumentos reconocibles, percusión, canciones
completas. La sensación de "traba" que persiste es un problema de
rendimiento general de Doom en esta máquina, no de la música - confirmado
reproduciendo sin música y viendo la misma traba.

Archivos clave de la implementación final:
- `src/hp_music.c` / `src/hp_music.h` - sintetizador OPL2/parser MUS en
  proceso.
- `src/i_sound.c` - Music API (`I_InitMusic` etc.) llamando a `HPMusic_*`;
  timer `SIGALRM`/`setitimer` (`I_HPStartAudioTimer`/`I_HPStopAudioTimer`).
- `src/d_main.c` - bloqueo de `SIGALRM` alrededor de las llamadas
  síncronas a `I_UpdateSound()`/`I_SubmitSound()` del bucle principal.
- `src/Makefile` - `hp_music.o` agregado al build; target `musserver_hp` y
  flag `-DMUSSERV` eliminados (ya no aplican).

Pendiente (fuera del alcance de esta investigación): investigar el
rendimiento general del motor en este hardware, independiente del audio.

## 10. Investigación del "chirrido" residual (sesión de optimización de CPU)

Tras el trabajo de optimización de CPU (`+O3 +DA2.0 +DS2.0 +Ofastaccess`,
ver `docs/06-investigacion-rendimiento-cpu.md`), con las trabas del motor ya
muy reducidas, el usuario reportó un problema distinto y hasta entonces
enmascarado por las trabas: un **chirrido** intermitente en la música de
E1M1 ("como cuando un parlante está suelto o haciendo mal contacto"),
inicialmente confundido con las trabas del motor (sección 8.5) pero
identificado como un problema real de la síntesis una vez que el juego
dejó de trabarse.

### 10.1 Metodología: aislar síntesis de tiempo real

Para responder "¿el ruido viene de correr el juego o de la implementación
de instrumentos?" se construyó un **renderizador offline** (no forma parte
del build de Doom, vive solo como herramienta de diagnóstico de sesión):
carga los lumps `GENMIDI` y una canción `MUS` directamente de
`shareware/doom1.wad`, y llama a `HPMusic_Init`/`HPMusic_LoadSong`/
`HPMusic_Generate` - el mismo código que corre en el juego - para escribir
un `.wav`, sin motor de Doom, sin timer `SIGALRM`, sin restricciones de
tiempo real. Cualquier ruido que aparezca ahí es un bug de la síntesis en
sí, no de cómo se integra con el juego.

Con esa herramienta se generaron varios `.wav` de prueba (carpeta
`test_audio/loop/`, archivos `14` a `18`) que el usuario escuchó
directamente y usó para localizar los problemas por oído y, en un caso
clave, inspeccionando la forma de onda en Audacity.

También se usó `opl2test_nuked.c` + `opl2.c` (Nuked-OPL2-Lite, emulador
cycle-accurate validado contra hardware YM3812 real, ver sección 5) como
**referencia de verdad**: renderizar el mismo instrumento/nota con el
sintetizador casero y con Nuked-OPL2 y comparar permitió distinguir "esto
sí sucede en hardware real" de "esto es un bug de nuestra síntesis".

### 10.2 Bug: mapeo de percusión MUS incorrecto

El canal 15 de MUS (percusión) mapeaba el "key" del evento a instrumento
GENMIDI con `128 + (note & 0x3F)`, clampeando al último instrumento
(índice 174) si se pasaba de rango. La fórmula real (verificada contra
`i_oplmusic.c` de Chocolate Doom, función `KeyOnEvent`) es:

```c
if (key < 35 || key > 81) return;      /* fuera de rango: ignorar la nota */
instrument = &percussion_instrs[key - 35];
```

Es decir, rango válido `[35, 81]` (47 sonidos de percusión), índice
`key - 35`, e **ignorar** (no sonar) cualquier nota fuera de ese rango en
vez de clampear a un instrumento arbitrario. El bug hacía sonar
instrumentos de percusión completamente distintos a los que la canción
pedía. Corregido en `hp_music.c`, función `mus_process_tic()`.

### 10.3 Bug: fórmula de feedback OPL2 incorrecta

El feedback del modulador (registro `fb`, usado por varios instrumentos de
percusión con `fb=7`, el máximo) se calculaba como el doble de la única
muestra cruda anterior: `fb_idx = (fb_prev * 2) >> (9 - fb)`. El chip real
(verificado contra `OPL2_SlotCalcFB()` de Nuked-OPL2) usa la **suma** de
las dos últimas muestras: `fbmod = (prout + out) >> (9 - fb)`. Duplicar una
sola muestra amplifica en vez de amortiguar cualquier cambio de signo entre
muestras consecutivas, lo que con feedback alto puede entrar en oscilación
descontrolada. Corregido guardando dos muestras anteriores (`fb_prev`,
`fb_prev2`) y sumándolas. Técnicamente más correcto, aunque en la práctica
no resultó ser la causa dominante del chirrido para el caso puntual
investigado (ver 10.4).

### 10.4 Bug real detrás del "hi-hat con estática": aliasing por Nyquist

Aislando el instrumento de percusión 139 (hi-hat abierto, nota fija de
GENMIDI = 79) se midieron saltos de muestra a muestra de hasta 18516 (57%
del rango completo) al renderizarlo con el sintetizador casero - algo que
Nuked-OPL2, rindiendo el mismo instrumento/nota, no mostraba en absoluto
(máximo 3579). La causa: la nota fija de ese instrumento, con su
multiplicador (`mult`), da una frecuencia de portadora de **~7840 Hz** -
muy por encima del límite de Nyquist a la tasa de muestreo de este
sintetizador (11025 Hz → Nyquist = 5512 Hz). Una frecuencia por encima de
Nyquist se "pliega" (fold-back aliasing) hacia una frecuencia completamente
distinta e inarmónica dentro del rango audible.

Hardware real / Nuked-OPL2 no sufren esto porque sintetizan internamente a
una tasa mucho más alta (~49716 Hz) y remuestrean con un filtro pasa-bajos
adecuado al bajar a la tasa de salida. Implementar sobremuestreo con
filtrado aquí habría sido caro en CPU (justamente lo que esta máquina no
sobra, ver `docs/06-investigacion-rendimiento-cpu.md`), así que se aplicó un
límite más barato: acotar la frecuencia máxima representable justo debajo
de Nyquist en `note_phase_step()` (`freq > SAMPLE_RATE*0.45` se recorta a
ese valor). El costo es precisión de tono en un puñado de instrumentos de
percusión extremos (que ya suenan como ruido/timbre metálico por diseño,
donde el tono exacto es inaudible/intrascendente); el beneficio es
eliminar por completo el aliasing. Con el fix, los saltos bruscos del
instrumento 139 en aislamiento pasaron de 70 a 0.

### 10.5 Bug: corte abrupto de envolvente (release/decay lineales)

Inspeccionando la forma de onda en Audacity, el usuario identificó caídas
verticales duras al final de cada nota - no un problema de instrumento
sino de la forma en que la amplitud termina. La envolvente de este
sintetizador es **lineal** (una escala 0-511 restada a ritmo constante por
muestra), y para tasas de decay/release rápidas (rango 12-15 de la tabla
`rate_to_inc`, hasta 512 por muestra) puede pasar de amplitud casi máxima a
cero en **una sola muestra** - un corte instantáneo y audible como un
"tic". El chip OPL2 real trabaja la envolvente en dominio logarítmico
(dB), donde el mismo "release rápido" da un decaimiento multiplicativo que
naturalmente se suaviza al acercarse a cero, sin el filo duro del modelo
lineal.

En vez de reescribir todo el modelo de envolvente a dominio logarítmico
(cambio mucho más invasivo para un beneficio acotado), se limitó la
velocidad máxima de caída de decay y release para que ningún segmento
pueda completarse en menos de `ENV_REL_MIN_SAMPLES` = 48 muestras (~4.3 ms
a 11025 Hz) - sigue siendo rápido/imperceptible como un fundido, pero ya
no es una discontinuidad de una sola muestra. Nueva función
`rate_to_inc_release()` usada tanto para `env_dec` como `env_rel` del
modulador y la portadora.

### 10.6 Bug: sesgo DC (asimetría de forma de onda)

El usuario notó, mirando la forma de onda en Audacity, que la señal
mezclada estaba mayormente en la mitad positiva del eje Y en vez de
oscilar simétricamente. Varias formas de onda del OPL2 (`opl_wave()` casos
1-3: seno recortado, seno rectificado completo, cuarto de seno) son, **por
diseño del chip**, asimétricas - su promedio no es cero (esto es
intencional, parte del timbre de instrumentos con distorsión/borde, como
la guitarra distorsionada de E1M1). En hardware real esto no es audible
como sesgo porque la salida de audio tiene un capacitor de acoplamiento
(filtro pasa-altos analógico) que remueve cualquier componente DC antes de
llegar al parlante. Este sintetizador software escribe las muestras
crudas directamente, sin ese filtrado - y un sesgo DC no filtrado no es
solo un problema visual: cada vez que una nota con forma de onda
asimétrica empieza o termina, el nivel DC salta de golpe, y un salto de DC
es en sí mismo un transitorio de banda ancha (un "click").

Se agregó un filtro DC-block clásico de un polo
(`y[n] = x[n] - x[n-1] + R*y[n-1]`, R≈0.9986, corte ≈2 Hz) aplicado a la
muestra final ya mezclada en `opl_mix_sample()`. El nivel DC promedio del
primer minuto de E1M1 bajó de 5738 a 72 (prácticamente cero) sobre una
escala de ±32767.

### 10.7 Bug real detrás del chirrido residual: mezcla con divisor dinámico

Con los fixes anteriores aplicados, el chirrido persistía. Otra
inspección de forma de onda del usuario mostró una **caída abrupta a
media envolvente**, sin ningún evento de retrigger de canal cerca (se
descartó instrumentando y registrando cada evento de robo/reutilización
de canal: no hubo ninguno en el primer minuto de la canción). La causa
real: `opl_mix_sample()` sumaba las muestras de todos los canales OPL2
activos y dividía por **la cantidad de canales activos en ese instante**
(`sum /= n`). Cada vez que una nota nueva activaba un canal antes inactivo
- incluso si esa nota recién estaba empezando su ataque y casi no
aportaba volumen todavía - `n` subía de golpe, y eso diluía
instantáneamente el volumen de **todas las demás** notas que ya estaban
sonando. Lo mismo al revés cuando un canal se apagaba. El resultado es un
"bombeo" (ducking) de volumen cada vez que entra o sale una voz - un
defecto de la estrategia de mezcla, no de ningún instrumento puntual, y
consistente con los cientos de eventos de nota por minuto típicos de
cualquier canción.

Fix: dividir por un **divisor fijo**, no por el conteo de canales activos,
de forma que el volumen de un canal ya sonando no dependa de que otros
canales prendan o apaguen. El valor del divisor se ajustó empíricamente
midiendo saltos bruscos (>5000 de diferencia entre muestras consecutivas)
y RMS sobre el primer minuto de E1M1 renderizado offline:

| Divisor | Saltos bruscos (60s) | Pico | RMS |
|---|---|---|---|
| 2 | 816 (mal, similar al bug original) | 32114 | 7716 |
| 3 | 93 | 27932 | 5275 |
| **4 (elegido)** | **1** | **27018** | **3956** |
| 5 | 0 | 21627 | 3163 |
| 9 (=NUM_OPL_CHAN, siempre sin clipping posible) | 0 | ~12045 | ~1758 (demasiado bajo) |

Se probó primero compensar el volumen con una ganancia x2 aplicada en la
mezcla final con SFX (`i_sound.c`), pero por sugerencia del usuario se
prefirió un solo parámetro de volumen en un solo lugar: ajustar
directamente el divisor de `hp_music.c` (de 5 a 4) en vez de mantener dos
controles de volumen en dos archivos distintos. Divisor 4 da un solo
salto brusco residual en 60 segundos (prácticamente inaudible) con volumen
notablemente mayor que 5.

### 10.8 Estado final (chirrido)

Con los seis fixes de la sección 10 combinados (percusión, feedback,
anti-aliasing por Nyquist, envolvente sin corte duro, DC-block, divisor de
mezcla fijo), el usuario confirmó en hardware real: "se siente mucho
mejor" / el ruido molesto ya no se percibe. El volumen de la música quedó
equilibrado respecto a los efectos de sonido tras ajustar el divisor de 5
a 4.

Archivos modificados en esta sección: `src/hp_music.c` únicamente
(`mus_process_tic()`, `note_phase_step()`, `load_instrument()`,
`render_channel()`, `opl_mix_sample()`, nueva función
`rate_to_inc_release()`). `src/i_sound.c` no requirió cambios finales (se
probó y revirtió una ganancia adicional ahí, ver 10.7).
